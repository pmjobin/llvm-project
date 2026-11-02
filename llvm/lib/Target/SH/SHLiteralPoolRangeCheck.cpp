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
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachinePassManager.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Target/TargetMachine.h"
#include <algorithm>
#include <limits>
#include <optional>

using namespace llvm;

#define DEBUG_TYPE "sh-literal-pool-range-check"

namespace {

struct LiteralUse {
  uint64_t Offset;
  unsigned CPI;
  int64_t Instance;
  bool IsWord;
};

struct SymbolLiteralUse {
  uint64_t Offset;
  const MCSymbol *Symbol;
};

static void fail(const MachineFunction &MF, const Twine &Message) {
  reportFatalUsageError(
      Twine("SH literal island validation failed: function ") + MF.getName() +
      ", " + Message);
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
  if (isSHDataIslandBlock(MBB))
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
    if (!isSHDataIslandBlock(*I))
      return &*I;
  }
  return nullptr;
}

static void validateExpandedPICPairs(const MachineFunction &MF) {
  DenseSet<const MCSymbol *> JumpTables;
  for (const MachineBasicBlock &MBB : MF)
    for (const MachineInstr &MI : MBB)
      if (MI.getOpcode() == SH::SH_JUMPTABLE &&
          MI.getNumExplicitOperands() == 4 && MI.getOperand(1).isMCSymbol())
        JumpTables.insert(MI.getOperand(1).getMCSymbol());
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
      if (JumpTables.contains(MOVA.getOperand(1).getMCSymbol()))
        continue;

      auto LoadI = std::next(I);
      if (LoadI != E && LoadI->getOpcode() == SH::FMOV_S_LOAD_POSTINC) {
        for (unsigned Word = 0; Word != 2; ++Word, ++LoadI)
          if (LoadI == E || LoadI->getOpcode() != SH::FMOV_S_LOAD_POSTINC ||
              LoadI->getNumExplicitOperands() != 3 ||
              !LoadI->getOperand(0).isReg() || !LoadI->getOperand(1).isReg() ||
              LoadI->getOperand(1).getReg() != SH::R0 ||
              !LoadI->getOperand(2).isReg() ||
              LoadI->getOperand(2).getReg() != SH::R0)
            fail(MF, "expanded double floating MOVA load is malformed");
        continue;
      }
      if (LoadI != E && LoadI->getOpcode() == SH::FMOV_S_LOAD) {
        if (LoadI->getNumExplicitOperands() != 2 ||
            !LoadI->getOperand(0).isReg() || !LoadI->getOperand(1).isReg() ||
            LoadI->getOperand(1).getReg() != SH::R0)
          fail(MF, "expanded floating MOVA load is malformed");
        continue;
      }
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

static const MachineInstr *getReachingDef(const MachineInstr &Use, Register Reg,
                                          const TargetRegisterInfo &TRI) {
  const MachineBasicBlock *MBB = Use.getParent();
  auto I = Use.getIterator();
  SmallPtrSet<const MachineBasicBlock *, 8> Visited;
  while (Visited.insert(MBB).second) {
    // Inspect raw instructions, including branch delay-slot bundle members.
    for (; I != MBB->instr_begin();) {
      --I;
      if (I->modifiesRegister(Reg, &TRI))
        return &*I;
    }
    // Literal islands can split MOVA and its entry load. Follow only a linear
    // predecessor chain with no alternate entry; never guess at joins or loops.
    if (MBB->isEntryBlock() || MBB->isEHPad() || MBB->hasAddressTaken() ||
        MBB->pred_size() != 1)
      return nullptr;
    const MachineBasicBlock *Pred = *MBB->pred_begin();
    if (Pred->succ_size() != 1)
      return nullptr;
    MBB = Pred;
    I = MBB->instr_end();
  }
  return nullptr;
}

static void validateInlineJumpTableDispatches(const MachineFunction &MF) {
  const MachineJumpTableInfo *MJTI = MF.getJumpTableInfo();
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();
  DenseMap<const MCSymbol *, const MachineInstr *> Tables;
  DenseMap<const MCSymbol *, const MachineInstr *> Bases;
  for (const MachineBasicBlock &MBB : MF) {
    for (const MachineInstr &MI : MBB) {
      if (MI.getOpcode() != SH::SH_JUMPTABLE)
        continue;
      if (MI.getNumExplicitOperands() != 4 || !MI.getOperand(0).isJTI() ||
          !MI.getOperand(1).isMCSymbol() || !MI.getOperand(2).isMCSymbol() ||
          !MI.getOperand(3).isImm())
        fail(MF, "malformed inline jump-table pseudo");
      int JTI = MI.getOperand(0).getIndex();
      int64_t EntrySize = MI.getOperand(3).getImm();
      if (!MJTI || MJTI->getEntryKind() != MachineJumpTableInfo::EK_Inline ||
          JTI < 0 ||
          static_cast<unsigned>(JTI) >= MJTI->getJumpTables().size() ||
          MJTI->getJumpTables()[JTI].MBBs.empty() ||
          (EntrySize != 2 && EntrySize != 4))
        fail(MF, "inline jump table has invalid metadata");
      if (!Tables.try_emplace(MI.getOperand(1).getMCSymbol(), &MI).second)
        fail(MF, "duplicate inline jump-table symbol");
      if (!Bases.try_emplace(MI.getOperand(2).getMCSymbol(), &MI).second)
        fail(MF, "duplicate inline jump-table base symbol");
    }
  }

  DenseSet<const MachineInstr *> MatchedTables;
  for (const MachineBasicBlock &MBB : MF) {
    for (const MachineInstr &Branch : MBB) {
      if (Branch.getOpcode() != SH::BRAF ||
          Branch.getNumExplicitOperands() == 1)
        continue;
      if (Branch.getNumExplicitOperands() != 3 ||
          !Branch.getOperand(0).isReg() || !Branch.getOperand(1).isJTI() ||
          !Branch.getOperand(2).isMCSymbol())
        fail(MF, "inline jump-table BRAF metadata is malformed");
      auto TableIt = Bases.find(Branch.getOperand(2).getMCSymbol());
      if (TableIt == Bases.end())
        fail(MF, "inline jump-table BRAF has no matching table");
      const MachineInstr &Table = *TableIt->second;
      if (!MatchedTables.insert(&Table).second)
        fail(MF, "inline jump table has multiple dispatches");
      int JTI = Branch.getOperand(1).getIndex();
      if (Table.getOperand(0).getIndex() != JTI)
        fail(MF, "inline jump-table dispatch index does not match its table");

      unsigned LoadOpcode = Table.getOperand(3).getImm() == 2
                                ? SH::MOVW_load_indexed
                                : SH::MOVL_load_indexed;
      const MachineInstr *MOVA = nullptr;
      for (const MachineBasicBlock &SearchMBB : MF) {
        for (const MachineInstr &MI : SearchMBB) {
          if (MI.getOpcode() != SH::MOVA || MI.getNumExplicitOperands() != 2 ||
              !MI.getOperand(0).isReg() ||
              MI.getOperand(0).getReg() != SH::R0 ||
              !MI.getOperand(1).isMCSymbol() ||
              MI.getOperand(1).getMCSymbol() !=
                  Table.getOperand(1).getMCSymbol())
            continue;
          if (MOVA)
            fail(MF, "inline jump table has multiple matching MOVAs");
          MOVA = &MI;
        }
      }
      if (!MOVA)
        fail(MF, "inline jump-table dispatch has no matching MOVA");

      const MachineInstr *Load = nullptr;
      for (const MachineBasicBlock &SearchMBB : MF) {
        for (const MachineInstr &MI : SearchMBB) {
          if (MI.getOpcode() != LoadOpcode ||
              getReachingDef(MI, SH::R0, TRI) != MOVA)
            continue;
          if (Load)
            fail(MF, "inline jump table has multiple matching entry loads");
          Load = &MI;
        }
      }
      if (!Load || Load->getNumExplicitOperands() != 2 ||
          !Load->getOperand(0).isReg() || !Load->getOperand(1).isReg())
        fail(MF, "inline jump-table dispatch has an invalid entry load");
      if (Load->memoperands().size() != 1)
        fail(MF, "inline jump-table dispatch has invalid memory metadata");
      const MachineMemOperand *MMO = *Load->memoperands_begin();
      const PseudoSourceValue *PSV = MMO->getPseudoValue();
      if (!MMO->isLoad() || MMO->isStore() ||
          MMO->getSize() !=
              LocationSize::precise(Table.getOperand(3).getImm()) ||
          MMO->getAlign() < Align(4) || !MMO->isInvariant() ||
          !MMO->isDereferenceable() || !PSV || !PSV->isJumpTable())
        fail(MF, "inline jump-table dispatch has invalid memory metadata");
    }
  }
  if (MatchedTables.size() != Tables.size())
    fail(MF, "inline jump table has no matching dispatch");
}

static void validatePICConstants(const MachineFunction &MF) {
  unsigned GOTPCCount = 0;
  bool RequiresPICBase = false;
  bool IsFDPIC = MF.getSubtarget<SHSubtarget>().isFDPIC();
  for (const MachineConstantPoolEntry &Entry :
       MF.getConstantPool()->getConstants()) {
    if (!Entry.isMachineConstantPoolEntry())
      continue;
    const auto *Value =
        static_cast<const SHConstantPoolValue *>(Entry.Val.MachineCPVal);
    SHConstantPoolValue::Modifier Modifier = Value->getModifier();
    if (MF.getTarget().isPositionIndependent() &&
        Modifier == SHConstantPoolValue::Modifier::None)
      fail(MF, "an absolute symbolic word remains in PIC executable code");
    if (!IsFDPIC && (Modifier == SHConstantPoolValue::Modifier::GOT ||
                     Modifier == SHConstantPoolValue::Modifier::GOTOFF ||
                     Modifier == SHConstantPoolValue::Modifier::GOTPC ||
                     Modifier == SHConstantPoolValue::Modifier::PLT ||
                     Modifier == SHConstantPoolValue::Modifier::TLSGD ||
                     Modifier == SHConstantPoolValue::Modifier::TLSLDM ||
                     Modifier == SHConstantPoolValue::Modifier::GOTTPOFF))
      RequiresPICBase = true;
    if (!IsFDPIC &&
        (Modifier == SHConstantPoolValue::Modifier::GOTFUNCDESC ||
         Modifier == SHConstantPoolValue::Modifier::GOTOFFFUNCDESC ||
         Modifier == SHConstantPoolValue::Modifier::FUNCDESC ||
         Modifier == SHConstantPoolValue::Modifier::PCREL))
      fail(MF,
           "an FDPIC-only symbolic modifier was used by an ordinary function");
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
      bool IsTLS = GV->isThreadLocal();
      bool IsTLSModifier =
          Modifier == SHConstantPoolValue::Modifier::TLSGD ||
          Modifier == SHConstantPoolValue::Modifier::TLSLDM ||
          Modifier == SHConstantPoolValue::Modifier::DTPOFF ||
          Modifier == SHConstantPoolValue::Modifier::GOTTPOFF ||
          Modifier == SHConstantPoolValue::Modifier::TPOFF;
      if (IsTLS != IsTLSModifier)
        fail(MF, IsTLS ? "a TLS symbol was lowered as an ordinary global"
                       : "a TLS modifier refers to a non-TLS symbol");
      bool IsNonPreemptible =
          IsFDPIC ? GV->hasLocalLinkage() || GV->hasHiddenVisibility()
                  : GV->isDSOLocal() && !GV->isInterposable();
      if (Modifier == SHConstantPoolValue::Modifier::GOTOFF &&
          !IsNonPreemptible)
        fail(MF, "a preemptible symbol was lowered with GOTOFF");
      if (Modifier == SHConstantPoolValue::Modifier::GOT && IsNonPreemptible) {
        const auto *Variable = dyn_cast<GlobalVariable>(GV);
        if (!IsFDPIC || !Variable ||
            (!Variable->isDeclarationForLinker() &&
             !TargetLoweringObjectFile::getKindForGlobal(Variable,
                                                         MF.getTarget())
                  .isReadOnlyWithRel()))
          fail(MF, "a nonpreemptible symbol was lowered through the GOT");
      }
      if (Modifier == SHConstantPoolValue::Modifier::PLT && IsNonPreemptible)
        fail(MF, "a nonpreemptible function was lowered through the PLT");
      if (Modifier == SHConstantPoolValue::Modifier::GOTOFFFUNCDESC &&
          (!IsNonPreemptible || GV->hasProtectedVisibility()))
        fail(MF, "an interposable or protected function used GOTOFFFUNCDESC");
      if (Modifier == SHConstantPoolValue::Modifier::GOTFUNCDESC &&
          IsNonPreemptible && !GV->hasProtectedVisibility())
        fail(MF, "a local function descriptor was lowered through the GOT");
    }
  }
  if (RequiresPICBase && !MF.getInfo<SHMachineFunctionInfo>()->usesPICBase())
    fail(MF, "PIC symbol materialization is missing GOT setup");
  if (RequiresPICBase && GOTPCCount != 1)
    fail(MF, "PIC function must contain exactly one GOTPC constant");
  if (!RequiresPICBase && GOTPCCount != 0)
    fail(MF, "function has an unused GOTPC constant");
}

static int64_t getDistance(const MachineFunction &MF, uint64_t UseOffset,
                           uint64_t EntryOffset, bool IsWord) {
  uint64_t Base = IsWord ? checkedAdd(MF, UseOffset, SH::PCRelativeBias)
                         : SH::getPCRelativeLongBase(UseOffset);
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
               MI.getOpcode() == SH::MOVW_load_pc_island ||
               MI.getOpcode() == SH::MOVA ||
               MI.getOpcode() == SH::MOVL_load_pc ||
               MI.getOpcode() == SH::SH_CONSTPOOL_ENTRY ||
               MI.getOpcode() == SH::SH_JUMPTABLE;
      });
    });
    if (!HasLiteralContent)
      return;
    validateExpandedPICPairs(MF);
    validateInlineJumpTableDispatches(MF);
    if (MF.getAlignment() < Align(SHLiteralIslandAlignment))
      fail(MF, "function alignment is less than four");

    const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
    DenseMap<uint64_t, uint64_t> EntryOffsets;
    DenseMap<const MCSymbol *, uint64_t> SymbolEntryOffsets;
    DenseMap<const MCSymbol *, uint64_t> JumpTableBaseOffsets;
    DenseMap<const MachineBasicBlock *, uint64_t> BlockOffsets;
    SmallVector<LiteralUse, 16> Uses;
    SmallVector<SymbolLiteralUse, 16> SymbolUses;
    uint64_t Offset = 0;

    for (const MachineBasicBlock &MBB : MF) {
      if (MBB.isBeginSection() && !MBB.isEntryBlock())
        fail(MF, "basic-block sections are not supported");
      Offset = alignBlockOffset(MF, Offset, MBB);
      BlockOffsets.try_emplace(&MBB, Offset);
      bool IsIsland = isSHDataIslandBlock(MBB);
      unsigned Payload = 0;
      unsigned PreviousAlignment = MBB.getAlignment().value();

      if (IsIsland) {
        if (MBB.getAlignment() < Align(SHLiteralIslandMinAlignment) ||
            MBB.getAlignment() > Align(SHDoubleLiteralIslandEntrySize))
          fail(MF, "island alignment is outside the supported range");
        const MachineBasicBlock *Water = getWaterBefore(MBB);
        if (!Water || !isSafeWaterPoint(*Water))
          fail(MF, "execution can fall through into an island");
        if (!MBB.pred_empty() || !MBB.succ_empty())
          fail(MF, "island has a CFG predecessor or successor");
      }

      for (const MachineInstr &MI : MBB) {
        for (const MachineOperand &MO : MI.operands())
          if (MO.isMBB() && isSHDataIslandBlock(*MO.getMBB()))
            fail(MF, "a branch or instruction targets an island");
        if (MI.isBundledWithPred() &&
            (MI.getOpcode() == SH::MOVL_load_pc_island ||
             MI.getOpcode() == SH::MOVW_load_pc_island ||
             MI.getOpcode() == SH::SH_CONSTPOOL_ENTRY ||
             MI.getOpcode() == SH::SH_JUMPTABLE))
          fail(MF, "literal-island content is inside an instruction bundle");
        if (MI.isBundledWithPred())
          continue;

        bool IsLongLiteral = MI.getOpcode() == SH::MOVL_load_pc_island;
        bool IsWordLiteral = MI.getOpcode() == SH::MOVW_load_pc_island;
        if (IsLongLiteral || IsWordLiteral) {
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
          unsigned RequiredSize = IsWordLiteral ? SHWordLiteralIslandEntrySize
                                                : SHLongLiteralIslandEntrySize;
          if (Constants[CPI].getSizeInBytes(MF.getDataLayout()) != RequiredSize)
            fail(MF,
                 "literal load and constant-pool entry widths do not match");
          Uses.push_back({Offset, CPI, Instance, IsWordLiteral});
        }

        if (MI.getOpcode() == SH::MOVA || MI.getOpcode() == SH::MOVL_load_pc) {
          if (MI.getNumExplicitOperands() != 2 || !MI.getOperand(0).isReg() ||
              !MI.getOperand(1).isMCSymbol())
            fail(MF, "expanded PIC literal use has malformed operands");
          if (MI.getOpcode() == SH::MOVA && MI.getOperand(0).getReg() != SH::R0)
            fail(MF, "MOVA does not define r0");
          SymbolUses.push_back({Offset, MI.getOperand(1).getMCSymbol()});
        }

        if (MI.getOpcode() == SH::BRAF && MI.getNumExplicitOperands() == 3 &&
            MI.getOperand(2).isMCSymbol() &&
            !JumpTableBaseOffsets
                 .try_emplace(MI.getOperand(2).getMCSymbol(), Offset)
                 .second)
          fail(MF, "duplicate inline jump-table base definition");

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
          unsigned ConstantSize =
              Constants[CPI].getSizeInBytes(MF.getDataLayout());
          if ((ConstantSize != SHWordLiteralIslandEntrySize &&
               ConstantSize != SHLongLiteralIslandEntrySize &&
               ConstantSize != SHDoubleLiteralIslandEntrySize) ||
              Constants[CPI].getAlign() > Align(ConstantSize))
            fail(MF, "constant-pool entry width or alignment is unsupported");
          if (Size != ConstantSize)
            fail(MF, "island entry width does not match its constant");
          if (Alignment !=
              static_cast<int64_t>(Constants[CPI].getAlign().value()))
            fail(MF, "island entry alignment does not match its constant");
          if (Alignment > PreviousAlignment)
            fail(MF, "island entries are not alignment ordered");
          if (Payload > SHLiteralIslandMaxPayload - Size)
            fail(MF, "island payload exceeds 1020 bytes");
          Payload += Size;
          PreviousAlignment = Alignment;
          uint64_t Key = entryKey(CPI, Instance);
          if (!EntryOffsets.try_emplace(Key, Offset).second)
            fail(MF, Twine("duplicate island entry label for CPI ") +
                         Twine(CPI) + ", instance " + Twine(Instance));
          MCSymbol *Symbol = getSHLiteralIslandSymbol(
              MF.getContext(), MF.getDataLayout(), MF.getFunctionNumber(), CPI,
              static_cast<unsigned>(Instance));
          if (!SymbolEntryOffsets.try_emplace(Symbol, Offset).second)
            fail(MF, "duplicate PIC island entry symbol");
        } else if (MI.getOpcode() == SH::SH_JUMPTABLE) {
          if (!isSHJumpTableBlock(MBB) || MI.getNumExplicitOperands() != 4 ||
              !MI.getOperand(0).isJTI() || !MI.getOperand(1).isMCSymbol() ||
              !MI.getOperand(2).isMCSymbol() || !MI.getOperand(3).isImm())
            fail(MF, "malformed inline jump-table pseudo");
          int JTI = MI.getOperand(0).getIndex();
          int64_t EntrySize = MI.getOperand(3).getImm();
          const MachineJumpTableInfo *MJTI = MF.getJumpTableInfo();
          if (!MJTI ||
              MJTI->getEntryKind() != MachineJumpTableInfo::EK_Inline ||
              JTI < 0 ||
              static_cast<unsigned>(JTI) >= MJTI->getJumpTables().size() ||
              (EntrySize != 2 && EntrySize != 4))
            fail(MF, "inline jump table has invalid metadata");
          if (!SymbolEntryOffsets
                   .try_emplace(MI.getOperand(1).getMCSymbol(), Offset)
                   .second)
            fail(MF, "duplicate inline jump-table symbol");
        } else if (IsIsland && !MI.isMetaInstruction()) {
          fail(MF, "island contains an executable instruction");
        }

        unsigned Size = TII.getInstSizeInBytes(MI);
        if (Size == 0 && !MI.isMetaInstruction() && !MI.isInlineAsm())
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
      int64_t Distance = getDistance(MF, Use.Offset, EntryOffset, Use.IsWord);
      unsigned MaxDistance = Use.IsWord ? SHWordLiteralLoadMaxDistance
                                        : SHLongLiteralLoadMaxDistance;
      unsigned Scale =
          Use.IsWord ? SH::PCRelativeWordScale : SH::PCRelativeLongScale;
      if (Distance < 0 || Distance > MaxDistance || Distance % Scale != 0)
        fail(MF, Twine("literal distance is invalid: CPI ") + Twine(Use.CPI) +
                     ", instance " + Twine(Use.Instance) + ", use offset " +
                     Twine(Use.Offset) + ", entry offset " +
                     Twine(EntryOffset) + ", distance " + Twine(Distance));
    }
    for (const SymbolLiteralUse &Use : SymbolUses) {
      auto Entry = SymbolEntryOffsets.find(Use.Symbol);
      if (Entry == SymbolEntryOffsets.end())
        fail(MF, Twine("expanded PIC literal symbol has no island entry: ") +
                     Use.Symbol->getName());
      int64_t Distance = getDistance(MF, Use.Offset, Entry->second,
                                     /*IsWord=*/false);
      if (Distance < 0 || Distance > SHLongLiteralLoadMaxDistance ||
          (Distance & 3) != 0)
        fail(MF, Twine("expanded PIC literal distance is invalid: symbol ") +
                     Use.Symbol->getName() + ", use offset " +
                     Twine(Use.Offset) + ", entry offset " +
                     Twine(Entry->second) + ", distance " + Twine(Distance) +
                     ", allowed aligned range 0..1020");
    }

    const MachineJumpTableInfo *MJTI = MF.getJumpTableInfo();
    for (const MachineBasicBlock &MBB : MF) {
      for (const MachineInstr &MI : MBB) {
        if (MI.getOpcode() != SH::SH_JUMPTABLE)
          continue;
        const MCSymbol *Base = MI.getOperand(2).getMCSymbol();
        auto BaseIt = JumpTableBaseOffsets.find(Base);
        if (BaseIt == JumpTableBaseOffsets.end())
          fail(MF, "inline jump table has no base definition");
        if (MI.getOperand(3).getImm() != 2)
          continue;
        uint64_t PCBase = checkedAdd(MF, BaseIt->second, 4);
        int JTI = MI.getOperand(0).getIndex();
        for (const MachineBasicBlock *Target :
             MJTI->getJumpTables()[JTI].MBBs) {
          auto TargetIt = BlockOffsets.find(Target);
          if (TargetIt == BlockOffsets.end())
            fail(MF, "inline jump table targets an unknown block");
          if (TargetIt->second >= PCBase) {
            uint64_t Distance = TargetIt->second - PCBase;
            if (Distance > static_cast<uint64_t>(INT16_MAX))
              fail(MF, "16-bit inline jump-table entry is out of range");
          } else {
            uint64_t Distance = PCBase - TargetIt->second;
            if (Distance >
                static_cast<uint64_t>(-static_cast<int64_t>(INT16_MIN)))
              fail(MF, "16-bit inline jump-table entry is out of range");
          }
        }
      }
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
    bool Changed = coalesceSHLongLiteralEntries(MF);
    if (Changed)
      RangeCheckImpl().run(MF);
    return Changed;
  }
};

char SHLiteralPoolRangeCheckLegacy::ID = 0;

} // namespace

INITIALIZE_PASS(SHLiteralPoolRangeCheckLegacy, DEBUG_TYPE,
                "Validate SH literal-island ranges", false, false)

FunctionPass *llvm::createSHLiteralPoolRangeCheckLegacyPass() {
  return new SHLiteralPoolRangeCheckLegacy();
}

PreservedAnalyses
SHLiteralPoolRangeCheckPass::run(MachineFunction &MF,
                                 MachineFunctionAnalysisManager &MFAM) {
  RangeCheckImpl().run(MF);
  if (coalesceSHLongLiteralEntries(MF)) {
    RangeCheckImpl().run(MF);
    return PreservedAnalyses::none();
  }
  return PreservedAnalyses::all();
}
