//===-- SHLiteralIsland.cpp - Place inline SH literal islands ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SH.h"
#include "SHInstrInfo.h"
#include "SHLiteralPool.h"
#include "SHSubtarget.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachinePassManager.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/ErrorHandling.h"
#include <limits>
#include <optional>

using namespace llvm;

#define DEBUG_TYPE "sh-literal-islands"

namespace {

// P may be two bytes past a four-byte boundary.  In that case the furthest
// valid entry is 1022 bytes after P, rather than 1024 bytes after P.
constexpr uint64_t ConservativeLiteralSpan =
    SHLiteralLoadMaxDistance + SHLiteralIslandAlignment / 2;

struct ConservativeLayout {
  DenseMap<const MachineBasicBlock *, uint64_t> BlockOffsets;
  DenseMap<const MachineBasicBlock *, uint64_t> BlockEnds;
  DenseMap<const MachineInstr *, uint64_t> InstrOffsets;
};

static void fail(const MachineFunction &MF, const Twine &Message) {
  report_fatal_error(Twine("SH literal island placement failed: function ") +
                     MF.getName() + ", " + Message);
}

static uint64_t checkedAdd(const MachineFunction &MF, uint64_t LHS,
                           uint64_t RHS) {
  if (LHS > std::numeric_limits<uint64_t>::max() - RHS)
    fail(MF, "code layout overflow");
  return LHS + RHS;
}

static unsigned getConservativeInstSize(const MachineFunction &MF,
                                        const SHInstrInfo &TII,
                                        const MachineInstr &MI) {
  if (MI.getOpcode() == SH::BT || MI.getOpcode() == SH::BF)
    return 6;
  unsigned Size = TII.getInstSizeInBytes(MI);
  if (Size == 0 && !MI.isMetaInstruction())
    fail(MF, "encountered an unexpanded instruction");
  return Size;
}

static ConservativeLayout computeConservativeLayout(MachineFunction &MF,
                                                    const SHInstrInfo &TII) {
  ConservativeLayout Layout;
  uint64_t Offset = 0;
  bool IsFirstBlock = true;

  for (MachineBasicBlock &MBB : MF) {
    if (!IsFirstBlock)
      Offset = checkedAdd(MF, Offset, MBB.getAlignment().value() - 1);
    IsFirstBlock = false;
    Layout.BlockOffsets[&MBB] = Offset;

    for (MachineInstr &MI : MBB) {
      if (MI.isBundledWithPred())
        continue;
      Layout.InstrOffsets[&MI] = Offset;
      Offset = checkedAdd(MF, Offset, getConservativeInstSize(MF, TII, MI));
    }
    Layout.BlockEnds[&MBB] = Offset;
  }
  return Layout;
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

static unsigned getIslandPayload(const MachineFunction &MF,
                                 const MachineBasicBlock &Island) {
  unsigned Payload = 0;
  for (const MachineInstr &MI : Island) {
    if (MI.isMetaInstruction())
      continue;
    if (MI.getOpcode() != SH::SH_CONSTPOOL_ENTRY ||
        MI.getNumExplicitOperands() != 4 || !MI.getOperand(0).isCPI() ||
        !MI.getOperand(1).isImm() || !MI.getOperand(2).isImm() ||
        !MI.getOperand(3).isImm())
      fail(MF, "malformed literal-island block");
    int64_t Size = MI.getOperand(2).getImm();
    int64_t Alignment = MI.getOperand(3).getImm();
    if (Size != SHLiteralIslandEntrySize ||
        Alignment != SHLiteralIslandAlignment)
      fail(MF, "only four-byte, four-byte-aligned island entries are "
               "supported");
    if (Payload > SHLiteralIslandMaxPayload - Size)
      fail(MF, "island payload exceeds 1020 bytes");
    Payload += Size;
  }
  return Payload;
}

static bool isConservativelyInRange(uint64_t UseOffset, uint64_t EntryOffset) {
  return EntryOffset >= UseOffset &&
         EntryOffset - UseOffset <= ConservativeLiteralSpan;
}

static MachineInstr *findEntry(MachineFunction &MF, unsigned CPI,
                               unsigned Instance) {
  for (MachineBasicBlock &MBB : MF) {
    if (!isSHLiteralIslandBlock(MBB))
      continue;
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == SH::SH_CONSTPOOL_ENTRY &&
          MI.getOperand(0).getIndex() == static_cast<int>(CPI) &&
          MI.getOperand(1).getImm() == Instance)
        return &MI;
    }
  }
  return nullptr;
}

static bool insertionInvalidatesAssigned(MachineFunction &MF,
                                         const ConservativeLayout &Layout,
                                         ArrayRef<MachineInstr *> AssignedUses,
                                         uint64_t InsertOffset,
                                         uint64_t InsertSize) {
  for (MachineInstr *Use : AssignedUses) {
    unsigned CPI = Use->getOperand(1).getIndex();
    unsigned Instance = Use->getOperand(2).getImm();
    MachineInstr *Entry = findEntry(MF, CPI, Instance);
    if (!Entry)
      fail(MF, "assigned literal use has no island entry");
    uint64_t UseOffset = Layout.InstrOffsets.lookup(Use);
    uint64_t EntryOffset = Layout.InstrOffsets.lookup(Entry);
    if (InsertOffset > UseOffset && InsertOffset <= EntryOffset &&
        !isConservativelyInRange(UseOffset,
                                 checkedAdd(MF, EntryOffset, InsertSize)))
      return true;
  }
  return false;
}

static MachineInstr *findReusableEntry(MachineFunction &MF,
                                       const ConservativeLayout &Layout,
                                       unsigned CPI, uint64_t UseOffset) {
  MachineInstr *Best = nullptr;
  uint64_t BestOffset = 0;
  for (MachineBasicBlock &MBB : MF) {
    if (!isSHLiteralIslandBlock(MBB))
      continue;
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() != SH::SH_CONSTPOOL_ENTRY ||
          MI.getOperand(0).getIndex() != static_cast<int>(CPI))
        continue;
      uint64_t EntryOffset = Layout.InstrOffsets.lookup(&MI);
      if (isConservativelyInRange(UseOffset, EntryOffset) &&
          (!Best || EntryOffset > BestOffset)) {
        Best = &MI;
        BestOffset = EntryOffset;
      }
    }
  }
  return Best;
}

static MachineBasicBlock *
findReusableIsland(MachineFunction &MF, const ConservativeLayout &Layout,
                   unsigned CPI, uint64_t UseOffset,
                   ArrayRef<MachineInstr *> AssignedUses) {
  MachineBasicBlock *Best = nullptr;
  uint64_t BestOffset = 0;
  for (MachineBasicBlock &MBB : MF) {
    if (!isSHLiteralIslandBlock(MBB))
      continue;
    unsigned Payload = getIslandPayload(MF, MBB);
    if (Payload > SHLiteralIslandMaxPayload - SHLiteralIslandEntrySize)
      continue;
    uint64_t EntryOffset = Layout.BlockOffsets.lookup(&MBB);
    for (const MachineInstr &MI : MBB) {
      if (MI.getOpcode() != SH::SH_CONSTPOOL_ENTRY ||
          MI.getOperand(0).getIndex() >= static_cast<int>(CPI))
        break;
      EntryOffset += SHLiteralIslandEntrySize;
    }
    if (!isConservativelyInRange(UseOffset, EntryOffset) ||
        insertionInvalidatesAssigned(MF, Layout, AssignedUses, EntryOffset,
                                     SHLiteralIslandEntrySize))
      continue;
    if (!Best || EntryOffset > BestOffset) {
      Best = &MBB;
      BestOffset = EntryOffset;
    }
  }
  return Best;
}

static MachineBasicBlock *getFollowingIsland(MachineBasicBlock &MBB) {
  auto Next = std::next(MBB.getIterator());
  if (Next == MBB.getParent()->end() || !isSHLiteralIslandBlock(*Next))
    return nullptr;
  return &*Next;
}

static MachineBasicBlock *findWater(MachineFunction &MF,
                                    const ConservativeLayout &Layout,
                                    uint64_t UseOffset,
                                    ArrayRef<MachineInstr *> AssignedUses) {
  MachineBasicBlock *Best = nullptr;
  uint64_t BestOffset = 0;
  for (MachineBasicBlock &MBB : MF) {
    if (!isSafeWaterPoint(MBB) || getFollowingIsland(MBB))
      continue;
    uint64_t InsertOffset = Layout.BlockEnds.lookup(&MBB);
    uint64_t EntryOffset = InsertOffset + SHLiteralIslandAlignment - 1;
    if (InsertOffset <= UseOffset ||
        !isConservativelyInRange(UseOffset, EntryOffset) ||
        insertionInvalidatesAssigned(MF, Layout, AssignedUses, InsertOffset,
                                     SHLiteralIslandAlignment - 1 +
                                         SHLiteralIslandEntrySize))
      continue;
    if (!Best || EntryOffset > BestOffset) {
      Best = &MBB;
      BestOffset = EntryOffset;
    }
  }
  return Best;
}

static MachineBasicBlock *insertIslandBefore(MachineFunction &MF,
                                             MachineBasicBlock &Before) {
  MachineBasicBlock *Island = MF.CreateMachineBasicBlock();
  Island->setAlignment(Align(SHLiteralIslandAlignment));
  Island->setSectionID(Before.getSectionID());
  MF.insert(Before.getIterator(), Island);
  return Island;
}

static MachineBasicBlock *insertIslandAfter(MachineFunction &MF,
                                            MachineBasicBlock &Water) {
  MachineBasicBlock *Island = MF.CreateMachineBasicBlock();
  Island->setAlignment(Align(SHLiteralIslandAlignment));
  Island->setSectionID(Water.getSectionID());
  if (Water.isEndSection()) {
    Water.setIsEndSection(false);
    Island->setIsEndSection();
  }
  MF.insert(std::next(Water.getIterator()), Island);
  return Island;
}

static void addSpecialStateLiveIns(MachineBasicBlock &MBB) {
  LivePhysRegs LiveRegs;
  computeLiveIns(LiveRegs, MBB);
  static constexpr MCPhysReg SpecialRegs[] = {SH::TBit, SH::MBit, SH::QBit,
                                              SH::MACL, SH::MACH, SH::PR};
  for (MCPhysReg Reg : SpecialRegs)
    if (LiveRegs.contains(Reg) && !MBB.isLiveIn(Reg))
      MBB.addLiveIn(Reg);
  MBB.sortUniqueLiveIns();
}

static MachineBasicBlock *createWaterAfterUse(MachineFunction &MF,
                                              const SHInstrInfo &TII,
                                              MachineInstr &Use) {
  MachineBasicBlock *Prefix = Use.getParent();
  if (Use.isBundled())
    fail(MF, "no safe split point is available inside an instruction bundle");

  auto AfterUse = std::next(Use.getIterator());
  MachineBasicBlock *Continuation = nullptr;
  if (AfterUse != Prefix->end()) {
    MBBSectionID SectionID = Prefix->getSectionID();
    bool WasEndSection = Prefix->isEndSection();
    Continuation = Prefix->splitAt(Use, true);
    Continuation->setSectionID(SectionID);
    Continuation->setIsEndSection(WasEndSection);
    Prefix->setIsEndSection(false);
    addSpecialStateLiveIns(*Continuation);
  } else {
    auto Next = std::next(Prefix->getIterator());
    if (Next == MF.end() || isSHLiteralIslandBlock(*Next))
      fail(MF, "no safe split point is available after the literal load");
    MBBSectionID SectionID = Prefix->getSectionID();
    bool WasEndSection = Prefix->isEndSection();
    Continuation = MF.CreateMachineBasicBlock(Prefix->getBasicBlock());
    Continuation->setSectionID(SectionID);
    Continuation->setIsEndSection(WasEndSection);
    Prefix->setIsEndSection(false);
    MF.insert(Next, Continuation);
    Continuation->transferSuccessorsAndUpdatePHIs(Prefix);
    if (Continuation->succ_empty())
      Continuation->addSuccessor(&*Next);
    Prefix->addSuccessor(Continuation);
    LivePhysRegs LiveRegs;
    computeAndAddLiveIns(LiveRegs, *Continuation);
    addSpecialStateLiveIns(*Continuation);
  }

  BuildMI(*Prefix, Prefix->end(), Use.getDebugLoc(), TII.get(SH::BRA))
      .addMBB(Continuation);
  return insertIslandBefore(MF, *Continuation);
}

class IslandPlacement {
  MachineFunction &MF;
  const SHInstrInfo &TII;
  DenseMap<unsigned, unsigned> NextInstance;
  SmallVector<MachineInstr *, 16> AssignedUses;

  unsigned addEntry(MachineBasicBlock &Island, unsigned CPI) {
    unsigned Instance = NextInstance[CPI]++;
    auto Insert = Island.end();
    for (auto I = Island.begin(); I != Island.end(); ++I)
      if (I->getOpcode() == SH::SH_CONSTPOOL_ENTRY &&
          I->getOperand(0).getIndex() > static_cast<int>(CPI)) {
        Insert = I;
        break;
      }
    BuildMI(Island, Insert, DebugLoc(), TII.get(SH::SH_CONSTPOOL_ENTRY))
        .addConstantPoolIndex(CPI)
        .addImm(Instance)
        .addImm(SHLiteralIslandEntrySize)
        .addImm(SHLiteralIslandAlignment);
    return Instance;
  }

public:
  explicit IslandPlacement(MachineFunction &MF)
      : MF(MF), TII(*MF.getSubtarget<SHSubtarget>().getInstrInfo()) {}

  bool run() {
    SmallVector<MachineInstr *, 16> Uses;
    bool HasInlineAsm = false;
    for (MachineBasicBlock &MBB : MF) {
      if (MBB.isBeginSection() && !MBB.isEntryBlock())
        fail(MF, "basic-block sections are not supported");
      for (MachineInstr &MI : MBB) {
        if (MI.isInlineAsm())
          HasInlineAsm = true;
        if (MI.getOpcode() != SH::MOVL_load_pc_island)
          continue;
        if (MI.getNumExplicitOperands() != 3 || !MI.getOperand(1).isCPI() ||
            !MI.getOperand(2).isImm())
          fail(MF, "PC-relative literal load has malformed operands");
        if (MI.getOperand(2).getImm() != SHUnassignedLiteralIsland)
          fail(MF, "literal load already has an assigned island instance");
        unsigned CPI = MI.getOperand(1).getIndex();
        if (CPI >= MF.getConstantPool()->getConstants().size())
          fail(MF, "literal load has an invalid constant-pool index");
        const MachineConstantPoolEntry &Entry =
            MF.getConstantPool()->getConstants()[CPI];
        if (Entry.getSizeInBytes(MF.getDataLayout()) !=
                SHLiteralIslandEntrySize ||
            Entry.getAlign() > Align(SHLiteralIslandAlignment))
          fail(MF, "only four-byte literal-island entries are supported");
        Uses.push_back(&MI);
      }
    }
    if (Uses.empty())
      return false;
    if (HasInlineAsm)
      fail(MF, "no safe split point is available around inline assembly");

    MF.ensureAlignment(Align(SHLiteralIslandAlignment));
    for (MachineBasicBlock &MBB : MF)
      MF.ensureAlignment(MBB.getAlignment());

    for (MachineInstr *Use : llvm::reverse(Uses)) {
      ConservativeLayout Layout = computeConservativeLayout(MF, TII);
      uint64_t UseOffset = Layout.InstrOffsets.lookup(Use);
      unsigned CPI = Use->getOperand(1).getIndex();
      unsigned Instance;

      if (MachineInstr *Entry = findReusableEntry(MF, Layout, CPI, UseOffset)) {
        Instance = Entry->getOperand(1).getImm();
      } else {
        MachineBasicBlock *Island =
            findReusableIsland(MF, Layout, CPI, UseOffset, AssignedUses);
        if (!Island) {
          if (MachineBasicBlock *Water =
                  findWater(MF, Layout, UseOffset, AssignedUses))
            Island = insertIslandAfter(MF, *Water);
          else
            Island = createWaterAfterUse(MF, TII, *Use);
        }
        Instance = addEntry(*Island, CPI);
      }

      Use->getOperand(2).setImm(Instance);
      AssignedUses.push_back(Use);
    }

    MF.RenumberBlocks();
    verifyMachineFunction("After SH literal islands", MF);
    return true;
  }
};

class SHLiteralIslandLegacy : public MachineFunctionPass {
public:
  static char ID;
  SHLiteralIslandLegacy() : MachineFunctionPass(ID) {}
  StringRef getPassName() const override { return "SH Literal Islands"; }
  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().setNoVRegs();
  }
  bool runOnMachineFunction(MachineFunction &MF) override {
    return IslandPlacement(MF).run();
  }
};

char SHLiteralIslandLegacy::ID = 0;

} // namespace

INITIALIZE_PASS(SHLiteralIslandLegacy, DEBUG_TYPE, "Place SH literal islands",
                false, false)

FunctionPass *llvm::createSHLiteralIslandLegacyPass() {
  return new SHLiteralIslandLegacy();
}

PreservedAnalyses
SHLiteralIslandPass::run(MachineFunction &MF,
                         MachineFunctionAnalysisManager &MFAM) {
  return IslandPlacement(MF).run() ? PreservedAnalyses::none()
                                   : PreservedAnalyses::all();
}
