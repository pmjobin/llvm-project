//===-- SHLiteralPoolRangeCheck.cpp - Validate SH literal islands --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SH.h"
#include "SHConstantPoolValue.h"
#include "SHInstrInfo.h"
#include "SHLiteralPool.h"
#include "SHMachineFunctionInfo.h"
#include "SHSubtarget.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachinePassManager.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetMachine.h"
#include <limits>
#include <optional>

using namespace llvm;

#define DEBUG_TYPE "sh-literal-pool-range-check"

namespace {

struct LiteralUse {
  uint64_t Offset;
  unsigned CPI;
  int64_t Instance;
};

struct SymbolLiteralUse {
  uint64_t Offset;
  const MCSymbol *Symbol;
};

static void fail(const MachineFunction &MF, const Twine &Message) {
  report_fatal_error(Twine("SH literal island validation failed: function ") +
                     MF.getName() + ", " + Message);
}

static uint64_t entryKey(unsigned CPI, unsigned Instance) {
  return static_cast<uint64_t>(CPI) << 32 | Instance;
}

static uint64_t checkedAdd(const MachineFunction &MF, uint64_t LHS,
                           uint64_t RHS) {
  if (LHS > std::numeric_limits<uint64_t>::max() - RHS)
    fail(MF, "code layout overflow");
  return LHS + RHS;
}

static uint64_t alignBlockOffset(const MachineFunction &MF, uint64_t Offset,
                                 const MachineBasicBlock &MBB) {
  Align Alignment = MBB.getAlignment();
  if (Alignment > MF.getAlignment())
    fail(MF, "basic-block alignment exceeds function alignment");
  uint64_t Aligned = alignTo(Offset, Alignment);
  uint64_t Padding = Aligned - Offset;
  unsigned MaxPadding = MBB.getMaxBytesForAlignment();
  return MaxPadding != 0 && Padding > MaxPadding ? Offset : Aligned;
}

static std::optional<bool> hasBarrier(const MachineBasicBlock &MBB) {
  auto I = MBB.rbegin();
  while (I != MBB.rend() && I->isMetaInstruction())
    ++I;
  if (I == MBB.rend())
    return std::nullopt;

  while (I->isBundledWithPred()) {
    ++I;
    if (I == MBB.rend())
      return false;
  }
  if (!I->isBundle())
    return I->isBarrier();

  bool InBundle = false;
  for (const MachineInstr &MI : MBB) {
    if (&MI == &*I) {
      InBundle = true;
      continue;
    }
    if (!InBundle)
      continue;
    if (!MI.isBundledWithPred())
      break;
    if (MI.isBarrier())
      return true;
  }
  return false;
}

static bool isSafeWaterPoint(const MachineBasicBlock &MBB) {
  if (isSHLiteralIslandBlock(MBB))
    return false;
  if (!MBB.isEntryBlock() && MBB.pred_empty())
    return true;
  std::optional<bool> Barrier = hasBarrier(MBB);
  return Barrier && *Barrier;
}

static const MachineBasicBlock *
getWaterBefore(const MachineBasicBlock &Island) {
  auto I = Island.getIterator();
  while (I != Island.getParent()->begin()) {
    --I;
    if (!isSHLiteralIslandBlock(*I))
      return &*I;
  }
  return nullptr;
}

static void validateExpandedPICPairs(const MachineFunction &MF) {
  for (const MachineBasicBlock &MBB : MF) {
    for (auto I = MBB.begin(), E = MBB.end(); I != E; ++I) {
      const MachineInstr &MOVA = *I;
      if (MOVA.getOpcode() != SH::MOVA)
        continue;
      if (MOVA.isBundledWithPred() || MOVA.isBundledWithSucc())
        fail(MF, "expanded PIC MOVA is inside an instruction bundle");
      if (MOVA.getNumExplicitOperands() != 2 || !MOVA.getOperand(0).isReg() ||
          MOVA.getOperand(0).getReg() != SH::R0 ||
          !MOVA.getOperand(1).isMCSymbol())
        fail(MF, "expanded PIC MOVA has malformed operands");

      auto LoadI = std::next(I);
      if (LoadI == E || LoadI->getOpcode() != SH::MOVL_load_pc ||
          LoadI->getNumExplicitOperands() != 2 ||
          !LoadI->getOperand(0).isReg() ||
          LoadI->getOperand(0).getReg() == SH::R0 ||
          !LoadI->getOperand(1).isMCSymbol() ||
          LoadI->getOperand(1).getMCSymbol() !=
              MOVA.getOperand(1).getMCSymbol())
        fail(MF, "expanded PIC MOVA and MOV.L do not share one island entry");

      Register Destination = LoadI->getOperand(0).getReg();
      auto AddI = std::next(LoadI);
      if (AddI == E || AddI->getOpcode() != SH::ADDrr ||
          AddI->getNumExplicitOperands() != 3 || !AddI->getOperand(0).isReg() ||
          AddI->getOperand(0).getReg() != Destination ||
          !AddI->getOperand(1).isReg() ||
          AddI->getOperand(1).getReg() != Destination ||
          !AddI->getOperand(2).isReg() ||
          AddI->getOperand(2).getReg() != SH::R0)
        fail(MF, "expanded PIC literal load has malformed address addition");
    }
  }
}

static void validatePICConstants(const MachineFunction &MF) {
  if (!MF.getTarget().isPositionIndependent())
    return;
  unsigned GOTPCCount = 0;
  bool HasPICSymbol = false;
  for (const MachineConstantPoolEntry &Entry :
       MF.getConstantPool()->getConstants()) {
    if (!Entry.isMachineConstantPoolEntry())
      continue;
    const auto *Value =
        static_cast<const SHConstantPoolValue *>(Entry.Val.MachineCPVal);
    SHConstantPoolValue::Modifier Modifier = Value->getModifier();
    if (Modifier == SHConstantPoolValue::Modifier::None)
      fail(MF, "an absolute symbolic word remains in PIC executable code");
    HasPICSymbol = true;
    if (Modifier == SHConstantPoolValue::Modifier::GOTPC) {
      ++GOTPCCount;
      if (!Value->isExternalSymbol() ||
          Value->getExternalSymbol() != "_GLOBAL_OFFSET_TABLE_" ||
          Value->getAddend() != 0)
        fail(MF, "PIC GOT setup has an invalid GOTPC expression");
    }
    if (Modifier == SHConstantPoolValue::Modifier::GOT &&
        Value->getAddend() != 0)
      fail(MF, "PIC GOT entry has an unsupported symbol addend");
    if (Value->isGlobalValue()) {
      const GlobalValue *GV = Value->getGlobalValue();
      bool IsNonPreemptible = GV->isDSOLocal() && !GV->isInterposable();
      if (Modifier == SHConstantPoolValue::Modifier::GOTOFF &&
          !IsNonPreemptible)
        fail(MF, "a preemptible symbol was lowered with GOTOFF");
      if (Modifier == SHConstantPoolValue::Modifier::GOT && IsNonPreemptible)
        fail(MF, "a nonpreemptible symbol was lowered through the GOT");
      if (Modifier == SHConstantPoolValue::Modifier::PLT && IsNonPreemptible)
        fail(MF, "a nonpreemptible function was lowered through the PLT");
    }
  }
  if (HasPICSymbol && !MF.getInfo<SHMachineFunctionInfo>()->usesPICBase())
    fail(MF, "PIC symbol materialization is missing GOT setup");
  if (HasPICSymbol && GOTPCCount != 1)
    fail(MF, "PIC function must contain exactly one GOTPC constant");
}

static int64_t getDistance(const MachineFunction &MF, uint64_t UseOffset,
                           uint64_t EntryOffset) {
  uint64_t Base = (UseOffset & ~UINT64_C(3)) + 4;
  if (EntryOffset >= Base) {
    uint64_t Distance = EntryOffset - Base;
    if (Distance > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
      fail(MF, "code layout overflow");
    return Distance;
  }
  uint64_t Distance = Base - EntryOffset;
  if (Distance > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
    fail(MF, "code layout overflow");
  return -static_cast<int64_t>(Distance);
}

class RangeCheckImpl {
public:
  void run(MachineFunction &MF) const {
    const auto &Constants = MF.getConstantPool()->getConstants();
    validatePICConstants(MF);
    bool HasLiteralContent = any_of(MF, [](const MachineBasicBlock &MBB) {
      return any_of(MBB, [](const MachineInstr &MI) {
        return MI.getOpcode() == SH::MOVL_load_pc_island ||
               MI.getOpcode() == SH::MOVA ||
               MI.getOpcode() == SH::MOVL_load_pc ||
               MI.getOpcode() == SH::SH_CONSTPOOL_ENTRY;
      });
    });
    if (!HasLiteralContent)
      return;
    validateExpandedPICPairs(MF);
    if (MF.getAlignment() < Align(SHLiteralIslandAlignment))
      fail(MF, "function alignment is less than four");

    const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
    DenseMap<uint64_t, uint64_t> EntryOffsets;
    DenseMap<const MCSymbol *, uint64_t> SymbolEntryOffsets;
    SmallVector<LiteralUse, 16> Uses;
    SmallVector<SymbolLiteralUse, 16> SymbolUses;
    uint64_t Offset = 0;

    for (const MachineBasicBlock &MBB : MF) {
      if (MBB.isBeginSection() && !MBB.isEntryBlock())
        fail(MF, "basic-block sections are not supported");
      Offset = alignBlockOffset(MF, Offset, MBB);
      bool IsIsland = isSHLiteralIslandBlock(MBB);
      unsigned Payload = 0;

      if (IsIsland) {
        if (MBB.getAlignment() != Align(SHLiteralIslandAlignment))
          fail(MF, "island alignment is not four");
        const MachineBasicBlock *Water = getWaterBefore(MBB);
        if (!Water || !isSafeWaterPoint(*Water))
          fail(MF, "execution can fall through into an island");
        if (!MBB.pred_empty() || !MBB.succ_empty())
          fail(MF, "island has a CFG predecessor or successor");
      }

      for (const MachineInstr &MI : MBB) {
        for (const MachineOperand &MO : MI.operands())
          if (MO.isMBB() && isSHLiteralIslandBlock(*MO.getMBB()))
            fail(MF, "a branch or instruction targets an island");
        if (MI.isBundledWithPred() &&
            (MI.getOpcode() == SH::MOVL_load_pc_island ||
             MI.getOpcode() == SH::SH_CONSTPOOL_ENTRY))
          fail(MF, "literal-island content is inside an instruction bundle");
        if (MI.isBundledWithPred())
          continue;
        if (MI.isInlineAsm())
          fail(MF, "inline assembly prevents exact final layout");

        if (MI.getOpcode() == SH::MOVL_load_pc_island) {
          if (MI.getNumExplicitOperands() != 3 || !MI.getOperand(1).isCPI() ||
              !MI.getOperand(2).isImm())
            fail(MF, "PC-relative literal load has malformed operands");
          int64_t Instance = MI.getOperand(2).getImm();
          if (Instance == SHUnassignedLiteralIsland)
            fail(MF, "literal load has an unassigned island instance");
          if (Instance < 0 || Instance > UINT32_MAX)
            fail(MF, "literal load has an invalid island instance");
          unsigned CPI = MI.getOperand(1).getIndex();
          if (CPI >= Constants.size())
            fail(MF, "literal load has an invalid constant-pool index");
          Uses.push_back({Offset, CPI, Instance});
        }

        if (MI.getOpcode() == SH::MOVA || MI.getOpcode() == SH::MOVL_load_pc) {
          if (MI.getNumExplicitOperands() != 2 || !MI.getOperand(0).isReg() ||
              !MI.getOperand(1).isMCSymbol())
            fail(MF, "expanded PIC literal use has malformed operands");
          if (MI.getOpcode() == SH::MOVA && MI.getOperand(0).getReg() != SH::R0)
            fail(MF, "MOVA does not define r0");
          SymbolUses.push_back({Offset, MI.getOperand(1).getMCSymbol()});
        }

        if (MI.getOpcode() == SH::SH_CONSTPOOL_ENTRY) {
          if (!IsIsland || MI.getNumExplicitOperands() != 4 ||
              !MI.getOperand(0).isCPI() || !MI.getOperand(1).isImm() ||
              !MI.getOperand(2).isImm() || !MI.getOperand(3).isImm())
            fail(MF, "malformed island entry pseudo");
          unsigned CPI = MI.getOperand(0).getIndex();
          int64_t Instance = MI.getOperand(1).getImm();
          int64_t Size = MI.getOperand(2).getImm();
          int64_t Alignment = MI.getOperand(3).getImm();
          if (CPI >= Constants.size() || Instance < 0 || Instance > UINT32_MAX)
            fail(MF, "island entry has invalid identity");
          if (Constants[CPI].getSizeInBytes(MF.getDataLayout()) !=
                  SHLiteralIslandEntrySize ||
              Constants[CPI].getAlign() > Align(SHLiteralIslandAlignment))
            fail(MF, "constant-pool entry width or alignment is unsupported");
          if (Size != SHLiteralIslandEntrySize)
            fail(MF, "island entry width is not four");
          if (Alignment != SHLiteralIslandAlignment)
            fail(MF, "island entry alignment is not four");
          if (Payload > SHLiteralIslandMaxPayload - SHLiteralIslandEntrySize)
            fail(MF, "island payload exceeds 1020 bytes");
          Payload += SHLiteralIslandEntrySize;
          uint64_t Key = entryKey(CPI, Instance);
          if (!EntryOffsets.try_emplace(Key, Offset).second)
            fail(MF, Twine("duplicate island entry label for CPI ") +
                         Twine(CPI) + ", instance " + Twine(Instance));
          MCSymbol *Symbol = getSHLiteralIslandSymbol(
              MF.getContext(), MF.getDataLayout(), MF.getFunctionNumber(), CPI,
              static_cast<unsigned>(Instance));
          if (!SymbolEntryOffsets.try_emplace(Symbol, Offset).second)
            fail(MF, "duplicate PIC island entry symbol");
        } else if (IsIsland && !MI.isMetaInstruction()) {
          fail(MF, "island contains an executable instruction");
        }

        unsigned Size = TII.getInstSizeInBytes(MI);
        if (Size == 0 && !MI.isMetaInstruction())
          fail(MF, "encountered an unexpanded instruction");
        Offset = checkedAdd(MF, Offset, Size);
      }
    }

    for (const LiteralUse &Use : Uses) {
      auto Entry = EntryOffsets.find(entryKey(Use.CPI, Use.Instance));
      if (Entry == EntryOffsets.end())
        fail(MF, Twine("referenced island entry does not exist: CPI ") +
                     Twine(Use.CPI) + ", instance " + Twine(Use.Instance));
      uint64_t EntryOffset = Entry->second;
      int64_t Distance = getDistance(MF, Use.Offset, EntryOffset);
      if (Distance < 0 || Distance > SHLiteralLoadMaxDistance ||
          (Distance & 3) != 0)
        fail(MF, Twine("literal distance is invalid: CPI ") + Twine(Use.CPI) +
                     ", instance " + Twine(Use.Instance) + ", use offset " +
                     Twine(Use.Offset) + ", entry offset " +
                     Twine(EntryOffset) + ", distance " + Twine(Distance) +
                     ", allowed aligned range 0..1020");
    }
    for (const SymbolLiteralUse &Use : SymbolUses) {
      auto Entry = SymbolEntryOffsets.find(Use.Symbol);
      if (Entry == SymbolEntryOffsets.end())
        fail(MF, Twine("expanded PIC literal symbol has no island entry: ") +
                     Use.Symbol->getName());
      int64_t Distance = getDistance(MF, Use.Offset, Entry->second);
      if (Distance < 0 || Distance > SHLiteralLoadMaxDistance ||
          (Distance & 3) != 0)
        fail(MF, Twine("expanded PIC literal distance is invalid: symbol ") +
                     Use.Symbol->getName() + ", use offset " +
                     Twine(Use.Offset) + ", entry offset " +
                     Twine(Entry->second) + ", distance " + Twine(Distance) +
                     ", allowed aligned range 0..1020");
    }
  }
};

class SHLiteralPoolRangeCheckLegacy : public MachineFunctionPass {
public:
  static char ID;
  SHLiteralPoolRangeCheckLegacy() : MachineFunctionPass(ID) {}
  StringRef getPassName() const override {
    return "SH Literal Pool Range Check";
  }
  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().setNoVRegs();
  }
  bool runOnMachineFunction(MachineFunction &MF) override {
    RangeCheckImpl().run(MF);
    return false;
  }
};

char SHLiteralPoolRangeCheckLegacy::ID = 0;

} // namespace

INITIALIZE_PASS(SHLiteralPoolRangeCheckLegacy, DEBUG_TYPE,
                "Validate SH literal-island ranges", false, true)

FunctionPass *llvm::createSHLiteralPoolRangeCheckLegacyPass() {
  return new SHLiteralPoolRangeCheckLegacy();
}

PreservedAnalyses
SHLiteralPoolRangeCheckPass::run(MachineFunction &MF,
                                 MachineFunctionAnalysisManager &MFAM) {
  RangeCheckImpl().run(MF);
  return PreservedAnalyses::all();
}
