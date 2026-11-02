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
#include "llvm/ADT/SmallPtrSet.h"
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
#include <algorithm>
#include <limits>
#include <optional>

using namespace llvm;

#define DEBUG_TYPE "sh-literal-islands"

namespace {

// Branch relaxation can replace a direct branch with an indirect sequence
// after literal islands have been placed.  Account for the largest source
// sequence (two saved registers and SH_LONG_BRANCH), and for the matching
// restores plus a branch around each restore block at its destination.
constexpr unsigned ConservativeLongBranchSourceSize = 24;
constexpr unsigned ConservativeLongBranchTargetSize = 12;

struct ConservativeLayout {
  DenseMap<const MachineBasicBlock *, uint64_t> BlockOffsets;
  DenseMap<const MachineBasicBlock *, uint64_t> BlockEnds;
  DenseMap<const MachineInstr *, uint64_t> InstrOffsets;
};

static bool isPICPair(const MachineInstr &MI) {
  return MI.getOpcode() == SH::SH_PIC_SETUP ||
         MI.getOpcode() == SH::SH_PIC_ADDRESS ||
         MI.getOpcode() == SH::SH_PIC_GOTOFF;
}

static bool isLiteralUse(const MachineInstr &MI) {
  return MI.getOpcode() == SH::MOVL_load_pc_island ||
         MI.getOpcode() == SH::MOVW_load_pc_island ||
         MI.getOpcode() == SH::FMOV_S_LOAD_PC_ISLAND ||
         MI.getOpcode() == SH::FMOV_D_LOAD_PC_ISLAND || isPICPair(MI) ||
         MI.getOpcode() == SH::SH_TLS_CALL || MI.getOpcode() == SH::SH_TLS_IE;
}

struct LiteralUse {
  MachineInstr *MI;
  unsigned CPIIndex;
  unsigned InstanceIndex;

  unsigned getCPI() const { return MI->getOperand(CPIIndex).getIndex(); }
  MachineOperand &getInstance() const { return MI->getOperand(InstanceIndex); }
};

static void appendLiteralUses(MachineInstr &MI,
                              SmallVectorImpl<LiteralUse> &Uses) {
  switch (MI.getOpcode()) {
  case SH::SH_PIC_SETUP:
    Uses.push_back({&MI, 0, 1});
    return;
  case SH::MOVL_load_pc_island:
  case SH::MOVW_load_pc_island:
  case SH::FMOV_S_LOAD_PC_ISLAND:
  case SH::FMOV_D_LOAD_PC_ISLAND:
  case SH::SH_PIC_ADDRESS:
  case SH::SH_PIC_GOTOFF:
  case SH::SH_TLS_IE:
    Uses.push_back({&MI, 1, 2});
    return;
  case SH::SH_TLS_CALL:
    Uses.push_back({&MI, 2, 3});
    Uses.push_back({&MI, 0, 1});
    return;
  default:
    return;
  }
}

static unsigned getLiteralEntrySize(const MachineFunction &MF, unsigned CPI) {
  const MachineConstantPoolEntry &Entry =
      MF.getConstantPool()->getConstants()[CPI];
  return Entry.getSizeInBytes(MF.getDataLayout());
}

static bool literalEntryComesBefore(const MachineFunction &MF, unsigned LHS,
                                    unsigned RHS) {
  unsigned LHSSize = getLiteralEntrySize(MF, LHS);
  unsigned RHSSize = getLiteralEntrySize(MF, RHS);
  return LHSSize != RHSSize ? LHSSize > RHSSize : LHS < RHS;
}

static unsigned getLiteralLoadMaxDistance(const MachineInstr &MI) {
  return MI.getOpcode() == SH::MOVW_load_pc_island
             ? SHWordLiteralLoadMaxDistance
             : SHLongLiteralLoadMaxDistance;
}

static unsigned getConservativeLiteralSpan(const MachineInstr &MI) {
  return getLiteralLoadMaxDistance(MI) + SHLiteralIslandAlignment / 2;
}

static void fail(const MachineFunction &MF, const Twine &Message) {
  reportFatalUsageError(Twine("SH literal island placement failed: function ") +
                        MF.getName() + ", " + Message);
}

static uint64_t checkedAdd(const MachineFunction &MF, uint64_t LHS,
                           uint64_t RHS) {
  if (LHS > std::numeric_limits<uint64_t>::max() - RHS)
    fail(MF, "code layout overflow");
  return LHS + RHS;
}

static unsigned getConservativeInstSize(
    const MachineFunction &MF, const SHInstrInfo &TII, const MachineInstr &MI,
    const SmallPtrSetImpl<const MachineInstr *>
        &PotentialRelaxedConditionalBranches,
    const SmallPtrSetImpl<const MachineInstr *> &PotentialLongBranches) {
  if (MI.getOpcode() == SH::BT || MI.getOpcode() == SH::BF) {
    unsigned BranchSize = TII.get(MI.getOpcode()).getSize();
    return PotentialLongBranches.contains(&MI)
               ? BranchSize + ConservativeLongBranchSourceSize
           : PotentialRelaxedConditionalBranches.contains(&MI)
               ? BranchSize + TII.get(SH::BRA).getSize() +
                     TII.get(SH::NOP).getSize()
               : BranchSize;
  }
  if (MI.getOpcode() == SH::BRA)
    return PotentialLongBranches.contains(&MI)
               ? ConservativeLongBranchSourceSize
               : 4;
  if (isPICPair(MI))
    return MI.getOpcode() == SH::SH_PIC_GOTOFF ? 4 : 6;
  if (MI.getOpcode() == SH::FMOV_S_LOAD_PC_ISLAND ||
      MI.getOpcode() == SH::FMOV_D_LOAD_PC_ISLAND)
    return MI.getOpcode() == SH::FMOV_S_LOAD_PC_ISLAND ? 4 : 6;
  if (MI.getOpcode() == SH::SH_TLS_CALL)
    return 12;
  if (MI.getOpcode() == SH::SH_TLS_IE)
    return 8;
  unsigned Size = TII.getInstSizeInBytes(MI);
  if (Size == 0 && !MI.isMetaInstruction() && !MI.isInlineAsm())
    fail(MF, "encountered an unexpanded instruction");
  return Size;
}

static ConservativeLayout computeConservativeLayout(
    MachineFunction &MF, const SHInstrInfo &TII,
    const SmallPtrSetImpl<const MachineInstr *>
        &PotentialRelaxedConditionalBranches,
    const SmallPtrSetImpl<const MachineInstr *> &PotentialLongBranches) {
  ConservativeLayout Layout;
  DenseMap<const MachineBasicBlock *, unsigned> PotentialRestoreCounts;
  for (const MachineBasicBlock &MBB : MF)
    for (const MachineInstr &MI : MBB)
      if (PotentialLongBranches.contains(&MI))
        for (const MachineOperand &MO : MI.operands())
          if (MO.isMBB()) {
            ++PotentialRestoreCounts[MO.getMBB()];
            break;
          }

  uint64_t Offset = 0;
  bool IsFirstBlock = true;

  for (MachineBasicBlock &MBB : MF) {
    Offset =
        checkedAdd(MF, Offset,
                   static_cast<uint64_t>(PotentialRestoreCounts.lookup(&MBB)) *
                       ConservativeLongBranchTargetSize);
    // The function entry has already been aligned for every block, so the
    // running offset determines the exact padding required here.
    if (!IsFirstBlock) {
      uint64_t Alignment = MBB.getAlignment().value();
      uint64_t Misalignment = Offset % Alignment;
      if (Misalignment)
        Offset = checkedAdd(MF, Offset, Alignment - Misalignment);
    }
    IsFirstBlock = false;
    Layout.BlockOffsets[&MBB] = Offset;

    for (MachineInstr &MI : MBB) {
      if (MI.isBundledWithPred())
        continue;
      Layout.InstrOffsets[&MI] = Offset;
      Offset = checkedAdd(MF, Offset,
                          getConservativeInstSize(
                              MF, TII, MI, PotentialRelaxedConditionalBranches,
                              PotentialLongBranches));
    }
    Layout.BlockEnds[&MBB] = Offset;
  }
  return Layout;
}

static bool isConservativeBranchInRange(const SHInstrInfo &TII, unsigned Opcode,
                                        uint64_t BranchOffset,
                                        uint64_t DestinationOffset) {
  if (DestinationOffset >= BranchOffset) {
    uint64_t Distance = DestinationOffset - BranchOffset;
    return Distance <=
               static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) &&
           TII.isBranchOffsetInRange(Opcode, static_cast<int64_t>(Distance));
  }
  uint64_t Distance = BranchOffset - DestinationOffset;
  return Distance <=
             static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) &&
         TII.isBranchOffsetInRange(Opcode, -static_cast<int64_t>(Distance));
}

static ConservativeLayout
computeStableConservativeLayout(MachineFunction &MF, const SHInstrInfo &TII) {
  SmallPtrSet<const MachineInstr *, 8> PotentialRelaxedConditionalBranches;
  SmallPtrSet<const MachineInstr *, 8> PotentialLongBranches;
  while (true) {
    ConservativeLayout Layout = computeConservativeLayout(
        MF, TII, PotentialRelaxedConditionalBranches, PotentialLongBranches);
    bool Changed = false;
    for (const MachineBasicBlock &MBB : MF)
      for (const MachineInstr &MI : MBB) {
        if (MI.getOpcode() != SH::BT && MI.getOpcode() != SH::BF &&
            MI.getOpcode() != SH::BRA)
          continue;
        const MachineBasicBlock *Destination = nullptr;
        for (const MachineOperand &MO : MI.operands())
          if (MO.isMBB()) {
            Destination = MO.getMBB();
            break;
          }
        if (!Destination)
          fail(MF, "direct branch is missing its destination");
        uint64_t BranchOffset = Layout.InstrOffsets.lookup(&MI);
        uint64_t DestinationOffset = Layout.BlockOffsets.lookup(Destination);
        if (MI.getOpcode() == SH::BRA) {
          if (MBB.getSectionID() != Destination->getSectionID() ||
              !isConservativeBranchInRange(TII, SH::BRA, BranchOffset,
                                           DestinationOffset))
            Changed |= PotentialLongBranches.insert(&MI).second;
          continue;
        }
        if (MBB.getSectionID() == Destination->getSectionID() &&
            isConservativeBranchInRange(TII, MI.getOpcode(), BranchOffset,
                                        DestinationOffset))
          continue;
        uint64_t RelaxedBranchOffset =
            checkedAdd(MF, BranchOffset, TII.get(MI.getOpcode()).getSize());
        if (MBB.getSectionID() != Destination->getSectionID() ||
            !isConservativeBranchInRange(TII, SH::BRA, RelaxedBranchOffset,
                                         DestinationOffset))
          Changed |= PotentialLongBranches.insert(&MI).second;
        else
          Changed |= PotentialRelaxedConditionalBranches.insert(&MI).second;
      }
    if (!Changed)
      return Layout;
  }
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

static unsigned getIslandPayload(const MachineFunction &MF,
                                 const MachineBasicBlock &Island) {
  unsigned Payload = 0;
  unsigned PreviousAlignment = Island.getAlignment().value();
  for (const MachineInstr &MI : Island) {
    if (MI.isMetaInstruction())
      continue;
    if (MI.getOpcode() != SH::SH_CONSTPOOL_ENTRY ||
        MI.getNumExplicitOperands() != 4 || !MI.getOperand(0).isCPI() ||
        !MI.getOperand(1).isImm() || !MI.getOperand(2).isImm() ||
        !MI.getOperand(3).isImm())
      fail(MF, "malformed literal-island block");
    unsigned CPI = MI.getOperand(0).getIndex();
    int64_t Size = MI.getOperand(2).getImm();
    int64_t Alignment = MI.getOperand(3).getImm();
    if (CPI >= MF.getConstantPool()->getConstants().size())
      fail(MF, "literal-island entry has an invalid constant-pool index");
    const MachineConstantPoolEntry &Entry =
        MF.getConstantPool()->getConstants()[CPI];
    if ((Size != SHWordLiteralIslandEntrySize &&
         Size != SHLongLiteralIslandEntrySize &&
         Size != SHDoubleLiteralIslandEntrySize) ||
        Size != Entry.getSizeInBytes(MF.getDataLayout()) ||
        Alignment != static_cast<int64_t>(Entry.getAlign().value()))
      fail(MF, "literal-island entry width or alignment is unsupported");
    if (Alignment > PreviousAlignment)
      fail(MF, "literal-island entries are not alignment ordered");
    if (Payload > SHLiteralIslandMaxPayload - Size)
      fail(MF, "island payload exceeds 1020 bytes");
    Payload += Size;
    PreviousAlignment = Alignment;
  }
  return Payload;
}

static bool isConservativelyInRange(const LiteralUse &Use, uint64_t UseOffset,
                                    uint64_t EntryOffset) {
  return EntryOffset >= UseOffset &&
         EntryOffset - UseOffset <= getConservativeLiteralSpan(*Use.MI);
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

static bool assignedUseIsConservativelyInRange(MachineFunction &MF,
                                               const ConservativeLayout &Layout,
                                               const LiteralUse &Use) {
  MachineInstr *Entry = findEntry(MF, Use.getCPI(), Use.getInstance().getImm());
  if (!Entry)
    fail(MF, "assigned literal use has no island entry");
  return isConservativelyInRange(Use, Layout.InstrOffsets.lookup(Use.MI),
                                 Layout.InstrOffsets.lookup(Entry));
}

static bool insertionInvalidatesAssigned(MachineFunction &MF,
                                         const ConservativeLayout &Layout,
                                         ArrayRef<LiteralUse> AssignedUses,
                                         uint64_t InsertOffset,
                                         uint64_t InsertSize) {
  for (const LiteralUse &Use : AssignedUses) {
    unsigned CPI = Use.getCPI();
    unsigned Instance = Use.getInstance().getImm();
    MachineInstr *Entry = findEntry(MF, CPI, Instance);
    if (!Entry)
      fail(MF, "assigned literal use has no island entry");
    uint64_t UseOffset = Layout.InstrOffsets.lookup(Use.MI);
    uint64_t EntryOffset = Layout.InstrOffsets.lookup(Entry);
    if (InsertOffset > UseOffset && InsertOffset <= EntryOffset &&
        !isConservativelyInRange(Use, UseOffset,
                                 checkedAdd(MF, EntryOffset, InsertSize)))
      return true;
  }
  return false;
}

static MachineInstr *
findReusableEntry(MachineFunction &MF, const ConservativeLayout &Layout,
                  const LiteralUse &Use, uint64_t UseOffset,
                  const SmallPtrSetImpl<MachineBasicBlock *> &ExcludedIslands) {
  unsigned CPI = Use.getCPI();
  MachineInstr *Best = nullptr;
  uint64_t BestOffset = 0;
  for (MachineBasicBlock &MBB : MF) {
    if (!isSHLiteralIslandBlock(MBB))
      continue;
    if (ExcludedIslands.contains(&MBB))
      continue;
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() != SH::SH_CONSTPOOL_ENTRY ||
          MI.getOperand(0).getIndex() != static_cast<int>(CPI))
        continue;
      uint64_t EntryOffset = Layout.InstrOffsets.lookup(&MI);
      if (isConservativelyInRange(Use, UseOffset, EntryOffset) &&
          (!Best || EntryOffset > BestOffset)) {
        Best = &MI;
        BestOffset = EntryOffset;
      }
    }
  }
  return Best;
}

static MachineBasicBlock *findReusableIsland(
    MachineFunction &MF, const ConservativeLayout &Layout,
    const LiteralUse &Use, uint64_t UseOffset,
    ArrayRef<LiteralUse> AssignedUses,
    const SmallPtrSetImpl<MachineBasicBlock *> &ExcludedIslands) {
  unsigned CPI = Use.getCPI();
  unsigned EntrySize = getLiteralEntrySize(MF, CPI);
  unsigned EntryAlignment = std::max<unsigned>(
      SHLiteralIslandAlignment,
      MF.getConstantPool()->getConstants()[CPI].getAlign().value());
  MachineBasicBlock *Best = nullptr;
  uint64_t BestOffset = 0;
  for (MachineBasicBlock &MBB : MF) {
    if (!isSHLiteralIslandBlock(MBB))
      continue;
    if (ExcludedIslands.contains(&MBB))
      continue;
    unsigned Payload = getIslandPayload(MF, MBB);
    if (Payload > SHLiteralIslandMaxPayload - EntrySize)
      continue;
    unsigned AlignmentGrowth = EntryAlignment > MBB.getAlignment().value()
                                   ? EntryAlignment - MBB.getAlignment().value()
                                   : 0;
    uint64_t IslandOffset = Layout.BlockOffsets.lookup(&MBB);
    uint64_t EntryOffset = IslandOffset;
    for (const MachineInstr &MI : MBB) {
      if (MI.getOpcode() != SH::SH_CONSTPOOL_ENTRY ||
          !literalEntryComesBefore(MF, MI.getOperand(0).getIndex(), CPI))
        break;
      EntryOffset += MI.getOperand(2).getImm();
    }
    EntryOffset += AlignmentGrowth;
    if (!isConservativelyInRange(Use, UseOffset, EntryOffset) ||
        insertionInvalidatesAssigned(MF, Layout, AssignedUses, IslandOffset,
                                     AlignmentGrowth + EntrySize))
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
  if (Next == MBB.getParent()->end() || !isSHDataIslandBlock(*Next))
    return nullptr;
  return &*Next;
}

static MachineBasicBlock *findWater(MachineFunction &MF,
                                    const ConservativeLayout &Layout,
                                    const LiteralUse &Use, uint64_t UseOffset,
                                    ArrayRef<LiteralUse> AssignedUses) {
  unsigned EntrySize = getLiteralEntrySize(MF, Use.getCPI());
  unsigned EntryAlignment =
      MF.getConstantPool()->getConstants()[Use.getCPI()].getAlign().value();
  EntryAlignment = std::max(SHLiteralIslandAlignment, EntryAlignment);
  MachineBasicBlock *Best = nullptr;
  uint64_t BestOffset = 0;
  for (MachineBasicBlock &MBB : MF) {
    if (!isSafeWaterPoint(MBB) || getFollowingIsland(MBB))
      continue;
    uint64_t InsertOffset = Layout.BlockEnds.lookup(&MBB);
    uint64_t EntryOffset = InsertOffset + EntryAlignment - 1;
    if (InsertOffset <= UseOffset ||
        !isConservativelyInRange(Use, UseOffset, EntryOffset) ||
        insertionInvalidatesAssigned(MF, Layout, AssignedUses, InsertOffset,
                                     EntryAlignment - 1 + EntrySize))
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
    if (Next == MF.end() || isSHDataIslandBlock(*Next))
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
  DenseMap<MachineInstr *, MachineBasicBlock *> AdjacentIslands;
  SmallPtrSet<MachineBasicBlock *, 8> AtomicIslandBlocks;
  SmallVector<LiteralUse, 16> AssignedUses;

  unsigned addEntry(MachineBasicBlock &Island, unsigned CPI) {
    unsigned Instance = NextInstance[CPI]++;
    unsigned EntrySize = getLiteralEntrySize(MF, CPI);
    Align EntryAlignment = MF.getConstantPool()->getConstants()[CPI].getAlign();
    Island.setAlignment(std::max(Island.getAlignment(), EntryAlignment));
    MF.ensureAlignment(EntryAlignment);
    auto Insert = Island.end();
    for (auto I = Island.begin(); I != Island.end(); ++I)
      if (I->getOpcode() == SH::SH_CONSTPOOL_ENTRY &&
          !literalEntryComesBefore(MF, I->getOperand(0).getIndex(), CPI)) {
        Insert = I;
        break;
      }
    BuildMI(Island, Insert, DebugLoc(), TII.get(SH::SH_CONSTPOOL_ENTRY))
        .addConstantPoolIndex(CPI)
        .addImm(Instance)
        .addImm(EntrySize)
        .addImm(EntryAlignment.value());
    return Instance;
  }

  unsigned appendEntry(MachineBasicBlock &Island, unsigned CPI) {
    unsigned Instance = NextInstance[CPI]++;
    unsigned EntrySize = getLiteralEntrySize(MF, CPI);
    Align EntryAlignment = MF.getConstantPool()->getConstants()[CPI].getAlign();
    Island.setAlignment(std::max(Island.getAlignment(), EntryAlignment));
    MF.ensureAlignment(EntryAlignment);
    BuildMI(Island, Island.end(), DebugLoc(), TII.get(SH::SH_CONSTPOOL_ENTRY))
        .addConstantPoolIndex(CPI)
        .addImm(Instance)
        .addImm(EntrySize)
        .addImm(EntryAlignment.value());
    return Instance;
  }

  void expandPICPair(MachineInstr &MI) {
    if (!isPICPair(MI))
      return;
    unsigned CPIIndex = MI.getOpcode() == SH::SH_PIC_SETUP ? 0 : 1;
    unsigned InstanceIndex = MI.getOpcode() == SH::SH_PIC_SETUP ? 1 : 2;
    unsigned CPI = MI.getOperand(CPIIndex).getIndex();
    int64_t Instance = MI.getOperand(InstanceIndex).getImm();
    if (Instance < 0 || Instance > UINT32_MAX)
      fail(MF, "PIC materialization has an invalid island instance");
    if (MI.memoperands().size() != 1)
      fail(MF, "PIC materialization requires one literal-load memory operand");
    MachineMemOperand *MMO = *MI.memoperands_begin();
    const PseudoSourceValue *PSV = MMO->getPseudoValue();
    if (!MMO->isLoad() || MMO->isStore() ||
        MMO->getSize() != LocationSize::precise(4) ||
        MMO->getAlign() < Align(4) || !MMO->isInvariant() ||
        !MMO->isDereferenceable() || !PSV || !PSV->isConstantPool())
      fail(MF, "PIC materialization has an invalid literal-load memory "
               "operand");

    MCSymbol *Symbol = getSHLiteralIslandSymbol(
        MF.getContext(), MF.getDataLayout(), MF.getFunctionNumber(), CPI,
        static_cast<unsigned>(Instance));
    MachineBasicBlock &MBB = *MI.getParent();
    MachineBasicBlock::iterator Insert = MI.getIterator();
    const DebugLoc &DL = MI.getDebugLoc();
    bool IsGOTOFF = MI.getOpcode() == SH::SH_PIC_GOTOFF;
    if (!IsGOTOFF)
      BuildMI(MBB, Insert, DL, TII.get(SH::MOVA), SH::R0).addSym(Symbol);

    Register Destination = MI.getOpcode() == SH::SH_PIC_SETUP
                               ? Register(SH::R12)
                               : MI.getOperand(0).getReg();
    if (!Destination.isPhysical() || (!IsGOTOFF && Destination == SH::R0))
      fail(MF, "PIC address has an invalid physical destination register");
    BuildMI(MBB, Insert, DL, TII.get(SH::MOVL_load_pc), Destination)
        .addSym(Symbol)
        .addMemOperand(MMO);
    BuildMI(MBB, Insert, DL, TII.get(SH::ADDrr), Destination)
        .addReg(Destination)
        .addReg(IsGOTOFF ? SH::R12 : SH::R0,
                IsGOTOFF ? static_cast<RegState>(0) : RegState::Kill);
    MI.eraseFromParent();
  }

  void expandFPConstant(MachineInstr &MI) {
    bool IsDouble = MI.getOpcode() == SH::FMOV_D_LOAD_PC_ISLAND;
    if (!IsDouble && MI.getOpcode() != SH::FMOV_S_LOAD_PC_ISLAND)
      return;
    if (MI.getNumExplicitOperands() != 3 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isCPI() || !MI.getOperand(2).isImm() ||
        MI.getOperand(2).getImm() < 0 ||
        MI.getOperand(2).getImm() > UINT32_MAX || MI.memoperands().size() != 1)
      fail(MF, "floating literal load has malformed operands");
    MachineMemOperand *MMO = *MI.memoperands_begin();
    const PseudoSourceValue *PSV = MMO->getPseudoValue();
    unsigned LoadSize = IsDouble ? 8 : 4;
    if (!MMO->isLoad() || MMO->isStore() ||
        MMO->getSize() != LocationSize::precise(LoadSize) ||
        MMO->getAlign() < Align(4) || !MMO->isInvariant() ||
        !MMO->isDereferenceable() || !PSV || !PSV->isConstantPool())
      fail(MF, "floating literal load has an invalid memory operand");
    unsigned CPI = MI.getOperand(1).getIndex();
    MCSymbol *Symbol = getSHLiteralIslandSymbol(
        MF.getContext(), MF.getDataLayout(), MF.getFunctionNumber(), CPI,
        static_cast<unsigned>(MI.getOperand(2).getImm()));
    MachineBasicBlock &MBB = *MI.getParent();
    MachineBasicBlock::iterator Insert = MI.getIterator();
    const DebugLoc &DL = MI.getDebugLoc();
    BuildMI(MBB, Insert, DL, TII.get(SH::MOVA), SH::R0).addSym(Symbol);
    if (!IsDouble) {
      MachineInstrBuilder Load =
          BuildMI(MBB, Insert, DL, TII.get(SH::FMOV_S_LOAD),
                  MI.getOperand(0).getReg())
              .addReg(SH::R0, RegState::Kill)
              .setMIFlags(MI.getFlags())
              .addMemOperand(MMO);
      Load->getOperand(0).setIsDead(MI.getOperand(0).isDead());
    } else {
      Register Dest = MI.getOperand(0).getReg();
      bool IsBigEndian = MF.getDataLayout().isBigEndian();
      for (unsigned MemoryWord = 0; MemoryWord != 2; ++MemoryWord) {
        unsigned PhysicalWord = IsBigEndian ? MemoryWord : 1 - MemoryWord;
        Register WordDest = TII.getRegisterInfo().getSubReg(
            Dest, PhysicalWord == 0 ? SH::sub_fpu32_0 : SH::sub_fpu32_1);
        MachineInstrBuilder Load =
            BuildMI(MBB, Insert, DL, TII.get(SH::FMOV_S_LOAD_POSTINC), WordDest)
                .addDef(SH::R0)
                .addReg(SH::R0, RegState::Kill)
                .setMIFlags(MI.getFlags())
                .addMemOperand(MF.getMachineMemOperand(MMO, 4 * MemoryWord, 4));
        Load->getOperand(0).setIsDead(MI.getOperand(0).isDead());
        if (MemoryWord == 1)
          Load->getOperand(1).setIsDead();
      }
    }
    MI.eraseFromParent();
  }

  MachineMemOperand *requireLoadMMO(MachineInstr &MI, unsigned Index,
                                    bool IsGOT) {
    if (Index >= MI.memoperands().size())
      fail(MF, "TLS materialization is missing a memory operand");
    MachineMemOperand *MMO = *std::next(MI.memoperands_begin(), Index);
    const PseudoSourceValue *PSV = MMO->getPseudoValue();
    if (!MMO->isLoad() || MMO->isStore() ||
        MMO->getSize() != LocationSize::precise(4) ||
        MMO->getAlign() < Align(4) || !MMO->isInvariant() ||
        !MMO->isDereferenceable() || !PSV ||
        (IsGOT ? !PSV->isGOT() : !PSV->isConstantPool()))
      fail(MF, IsGOT ? "initial-exec TLS has an invalid GOT-load memory "
                       "operand"
                     : "TLS materialization has an invalid literal-load "
                       "memory operand");
    return MMO;
  }

  MCSymbol *getAssignedSymbol(MachineInstr &MI, unsigned CPIIndex,
                              unsigned InstanceIndex) {
    if (!MI.getOperand(CPIIndex).isCPI() ||
        !MI.getOperand(InstanceIndex).isImm())
      fail(MF, "TLS materialization has malformed literal operands");
    int64_t Instance = MI.getOperand(InstanceIndex).getImm();
    if (Instance < 0 || Instance > UINT32_MAX)
      fail(MF, "TLS materialization has an invalid island instance");
    return getSHLiteralIslandSymbol(
        MF.getContext(), MF.getDataLayout(), MF.getFunctionNumber(),
        MI.getOperand(CPIIndex).getIndex(), static_cast<unsigned>(Instance));
  }

  void expandTLSCall(MachineInstr &MI) {
    if (MI.getOpcode() != SH::SH_TLS_CALL)
      return;
    if (MI.memoperands().size() != 2)
      fail(MF, "TLS resolver call requires two literal-load memory operands");
    MachineMemOperand *DescriptorMMO = requireLoadMMO(MI, 0, false);
    MachineMemOperand *ResolverMMO = requireLoadMMO(MI, 1, false);
    MCSymbol *Descriptor = getAssignedSymbol(MI, 0, 1);
    MCSymbol *Resolver = getAssignedSymbol(MI, 2, 3);
    const MachineOperand *RegMask = nullptr;
    for (const MachineOperand &MO : MI.operands())
      if (MO.isRegMask()) {
        if (RegMask)
          fail(MF, "TLS resolver call has more than one register mask");
        RegMask = &MO;
      }
    if (!RegMask)
      fail(MF, "TLS resolver call is missing its register mask");

    MachineBasicBlock &MBB = *MI.getParent();
    MachineBasicBlock::iterator Insert = MI.getIterator();
    const DebugLoc &DL = MI.getDebugLoc();
    BuildMI(MBB, Insert, DL, TII.get(SH::MOVL_load_pc), SH::R4)
        .addSym(Descriptor)
        .addMemOperand(DescriptorMMO);
    BuildMI(MBB, Insert, DL, TII.get(SH::MOVA), SH::R0).addSym(Resolver);
    BuildMI(MBB, Insert, DL, TII.get(SH::MOVL_load_pc), SH::R1)
        .addSym(Resolver)
        .addMemOperand(ResolverMMO);
    BuildMI(MBB, Insert, DL, TII.get(SH::ADDrr), SH::R1)
        .addReg(SH::R1)
        .addReg(SH::R0, RegState::Kill);
    MachineInstrBuilder Call =
        BuildMI(MBB, Insert, DL, TII.get(SH::JSR))
            .addReg(SH::R1, RegState::Kill)
            .addRegMask(RegMask->getRegMask())
            .addReg(SH::R4, RegState::Implicit | RegState::Kill)
            .addReg(SH::R0, RegState::ImplicitDefine);
    MachineInstrBuilder Slot =
        BuildMI(MBB, Insert, DL, TII.get(SH::ADDrr), SH::R4)
            .addReg(SH::R4)
            .addReg(SH::R12);
    MIBundleBuilder(MBB, Call->getIterator(), std::next(Slot->getIterator()));
    MI.eraseFromParent();
  }

  void expandTLSIE(MachineInstr &MI) {
    if (MI.getOpcode() != SH::SH_TLS_IE)
      return;
    if (MI.memoperands().size() != 2)
      fail(MF, "initial-exec TLS requires literal and GOT memory operands");
    MachineMemOperand *LiteralMMO = requireLoadMMO(MI, 0, false);
    MachineMemOperand *GOTMMO = requireLoadMMO(MI, 1, true);
    MCSymbol *Literal = getAssignedSymbol(MI, 1, 2);
    Register Destination = MI.getOperand(0).getReg();
    if (!Destination.isPhysical() || Destination == SH::R0)
      fail(MF, "initial-exec TLS destination must be a physical register "
               "other than r0");

    MachineBasicBlock &MBB = *MI.getParent();
    MachineBasicBlock::iterator Insert = MI.getIterator();
    const DebugLoc &DL = MI.getDebugLoc();
    BuildMI(MBB, Insert, DL, TII.get(SH::MOVL_load_pc), SH::R0)
        .addSym(Literal)
        .addMemOperand(LiteralMMO);
    BuildMI(MBB, Insert, DL, TII.get(SH::STC_GBR), Destination);
    BuildMI(MBB, Insert, DL, TII.get(SH::MOVL_load_indexed), SH::R0)
        .addReg(SH::R12)
        .addMemOperand(GOTMMO);
    MachineBasicBlock::iterator Branch = std::next(MI.getIterator());
    if (Branch == MBB.end() || Branch->getOpcode() != SH::BRA ||
        Branch->isBundled())
      fail(MF, "initial-exec TLS is not followed by its island branch");
    MachineInstrBuilder Slot =
        BuildMI(MBB, std::next(Branch), DL, TII.get(SH::ADDrr), Destination)
            .addReg(Destination)
            .addReg(SH::R0, RegState::Kill);
    MIBundleBuilder(MBB, Branch, std::next(Slot->getIterator()));
    MI.eraseFromParent();
  }

public:
  explicit IslandPlacement(MachineFunction &MF)
      : MF(MF), TII(*MF.getSubtarget<SHSubtarget>().getInstrInfo()) {}

  bool run() {
    SmallVector<LiteralUse, 16> Uses;
    SmallVector<MachineInstr *, 16> Instructions;
    for (MachineBasicBlock &MBB : MF) {
      if (MBB.isBeginSection() && !MBB.isEntryBlock())
        fail(MF, "basic-block sections are not supported");
      for (MachineInstr &MI : MBB) {
        if (!isLiteralUse(MI))
          continue;
        Instructions.push_back(&MI);
        SmallVector<LiteralUse, 2> MIUses;
        appendLiteralUses(MI, MIUses);
        if (MIUses.empty())
          fail(MF, "literal use has no literal operands");
        for (const LiteralUse &Use : MIUses) {
          if (!MI.getOperand(Use.CPIIndex).isCPI() ||
              !Use.getInstance().isImm())
            fail(MF, "literal use has malformed operands");
          if (Use.getInstance().getImm() != SHUnassignedLiteralIsland)
            fail(MF, "literal load already has an assigned island instance");
          unsigned CPI = Use.getCPI();
          if (CPI >= MF.getConstantPool()->getConstants().size())
            fail(MF, "literal load has an invalid constant-pool index");
          const MachineConstantPoolEntry &Entry =
              MF.getConstantPool()->getConstants()[CPI];
          unsigned EntrySize = Entry.getSizeInBytes(MF.getDataLayout());
          unsigned RequiredSize =
              Use.MI->getOpcode() == SH::MOVW_load_pc_island
                  ? SHWordLiteralIslandEntrySize
              : Use.MI->getOpcode() == SH::FMOV_D_LOAD_PC_ISLAND
                  ? SHDoubleLiteralIslandEntrySize
                  : SHLongLiteralIslandEntrySize;
          if (EntrySize != RequiredSize || Entry.getAlign() > Align(EntrySize))
            fail(MF,
                 "literal load and constant-pool entry widths do not match");
          Uses.push_back(Use);
        }
      }
    }
    if (Uses.empty())
      return false;

    MF.ensureAlignment(Align(SHLiteralIslandAlignment));
    for (MachineBasicBlock &MBB : MF)
      MF.ensureAlignment(MBB.getAlignment());

    for (LiteralUse &Use : llvm::reverse(Uses)) {
      ConservativeLayout Layout = computeStableConservativeLayout(MF, TII);
      uint64_t UseOffset = Layout.InstrOffsets.lookup(Use.MI);
      unsigned CPI = Use.getCPI();
      unsigned Instance;

      bool NeedsAdjacentIsland = Use.MI->getOpcode() == SH::SH_TLS_CALL ||
                                 Use.MI->getOpcode() == SH::SH_TLS_IE;
      if (NeedsAdjacentIsland) {
        MachineBasicBlock *&Island = AdjacentIslands[Use.MI];
        if (!Island)
          Island = createWaterAfterUse(MF, TII, *Use.MI);
        AtomicIslandBlocks.insert(Island);
        Instance = appendEntry(*Island, CPI);
      } else if (MachineInstr *Entry = findReusableEntry(
                     MF, Layout, Use, UseOffset, AtomicIslandBlocks)) {
        Instance = Entry->getOperand(1).getImm();
      } else {
        MachineBasicBlock *Island = findReusableIsland(
            MF, Layout, Use, UseOffset, AssignedUses, AtomicIslandBlocks);
        if (!Island) {
          if (MachineBasicBlock *Water =
                  findWater(MF, Layout, Use, UseOffset, AssignedUses))
            Island = insertIslandAfter(MF, *Water);
          else
            Island = createWaterAfterUse(MF, TII, *Use.MI);
        }
        Instance = addEntry(*Island, CPI);
      }

      Use.getInstance().setImm(Instance);
      AssignedUses.push_back(Use);
    }

    // Island insertion can push an otherwise short conditional branch over its
    // range and make later branch relaxation grow it. Recompute the completed
    // layout and give any affected literal use a guaranteed-nearby entry.
    while (true) {
      ConservativeLayout Layout = computeStableConservativeLayout(MF, TII);
      auto Invalid = llvm::find_if(AssignedUses, [&](const LiteralUse &Use) {
        return !assignedUseIsConservativelyInRange(MF, Layout, Use);
      });
      if (Invalid == AssignedUses.end())
        break;
      MachineBasicBlock *Island = createWaterAfterUse(MF, TII, *Invalid->MI);
      Invalid->getInstance().setImm(addEntry(*Island, Invalid->getCPI()));
    }

    for (MachineBasicBlock &MBB : MF) {
      if (!isSHLiteralIslandBlock(MBB))
        continue;
      Align IslandAlignment(SHLiteralIslandMinAlignment);
      for (const MachineInstr &MI : MBB)
        if (MI.getOpcode() == SH::SH_CONSTPOOL_ENTRY)
          IslandAlignment =
              std::max(IslandAlignment, Align(MI.getOperand(3).getImm()));
      MBB.setAlignment(IslandAlignment);
    }

    for (MachineInstr *MI : Instructions) {
      if (isPICPair(*MI))
        expandPICPair(*MI);
      else if (MI->getOpcode() == SH::FMOV_S_LOAD_PC_ISLAND ||
               MI->getOpcode() == SH::FMOV_D_LOAD_PC_ISLAND)
        expandFPConstant(*MI);
      else if (MI->getOpcode() == SH::SH_TLS_CALL)
        expandTLSCall(*MI);
      else if (MI->getOpcode() == SH::SH_TLS_IE)
        expandTLSIE(*MI);
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
