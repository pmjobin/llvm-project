//===-- SHDelaySlotFiller.cpp - Fill SH delay slots ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/SHBaseInfo.h"
#include "SH.h"
#include "SHBranchRange.h"
#include "SHInstrInfo.h"
#include "SHSubtarget.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/LiveRegUnits.h"
#include "llvm/CodeGen/MachineBranchProbabilityInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachinePassManager.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "sh-delay-slot-filler"

STATISTIC(DelayedSlots, "Number of SH delayed instructions seen");
STATISTIC(AlreadyFilledSlots, "Number of SH delay slots already filled");
STATISTIC(NewlyFilledSlots, "Number of SH delay slots newly filled");
STATISTIC(NopSlots, "Number of SH delay slots left as NOPs");
STATISTIC(NoCandidateSlots,
          "Number of SH delay slots without a legal candidate");
STATISTIC(CallSlots, "Number of SH delayed calls seen");
STATISTIC(FilledCallSlots, "Number of SH delayed calls newly filled");
STATISTIC(AdjacentCallLoadSlots,
          "Number of SH call slots filled by adjacent loads");
STATISTIC(NonAdjacentCallLoadSlots,
          "Number of SH call slots filled by non-adjacent loads");
STATISTIC(AdjacentCallStoreSlots,
          "Number of SH call slots filled by adjacent stores");
STATISTIC(AdjacentUnconditionalMemorySlots,
          "Number of SH unconditional branch slots filled by adjacent memory "
          "operations");
STATISTIC(
    NonAdjacentUnconditionalLoadSlots,
    "Number of SH unconditional branch slots filled by non-adjacent loads");
STATISTIC(
    SuccessorSlots,
    "Number of SH unconditional branch slots filled from their successor");
STATISTIC(
    ConditionalSuccessorSlots,
    "Number of SH conditional branch slots filled by a taken-successor copy");
STATISTIC(LexicographicCompareSlots, "Number of SH conditional branch slots "
                                     "filled by a lexicographic comparison");
STATISTIC(RawRejections, "Number of SH delay-slot RAW rejections");
STATISTIC(WarRejections, "Number of SH delay-slot WAR rejections");
STATISTIC(WawRejections, "Number of SH delay-slot WAW rejections");
STATISTIC(MemoryRejections, "Number of SH delay-slot memory rejections");
STATISTIC(SideEffectRejections,
          "Number of SH delay-slot side-effect rejections");
STATISTIC(ControlRejections, "Number of SH delay-slot control rejections");
STATISTIC(SizeRejections, "Number of SH delay-slot size rejections");
STATISTIC(SlotIllegalRejections,
          "Number of SH delay-slot architectural rejections");
STATISTIC(SpecialStateRejections,
          "Number of SH delay-slot special-register rejections");
STATISTIC(SearchBoundStops,
          "Number of SH delay-slot searches stopped at the bound");

namespace {

// Keep branch and return searches local, with room for scheduling hazards.
static constexpr unsigned MaxSearchInstructions = 4;
// Argument computation can precede several dependent virtual-target loads.
// Allow a longer bounded search at calls when optimizing for speed.
static constexpr unsigned MaxCallSearchInstructions = 12;

enum class RejectionReason {
  None,
  RAW,
  WAR,
  WAW,
  Memory,
  SideEffect,
  Control,
  Size,
  SlotIllegal,
};

enum class CandidateAction { Accept, Reject, Boundary, Skip };

struct CandidateDecision {
  CandidateAction Action;
  RejectionReason Reason;
};

static bool isExpandedPICLoadSequenceMember(const MachineInstr &MI) {
  const MachineInstr *Previous = MI.getPrevNode();
  if (!Previous)
    return false;
  if (Previous->getOpcode() == SH::MOVA)
    return true;
  const MachineInstr *BeforePrevious = Previous->getPrevNode();
  return BeforePrevious && BeforePrevious->getOpcode() == SH::MOVA &&
         Previous->getOpcode() == SH::FMOV_S_LOAD_POSTINC &&
         MI.getOpcode() == SH::FMOV_S_LOAD_POSTINC;
}

static StringRef getReasonName(RejectionReason Reason) {
  switch (Reason) {
  case RejectionReason::None:
    return "none";
  case RejectionReason::RAW:
    return "raw";
  case RejectionReason::WAR:
    return "war";
  case RejectionReason::WAW:
    return "waw";
  case RejectionReason::Memory:
    return "memory";
  case RejectionReason::SideEffect:
    return "side-effect";
  case RejectionReason::Control:
    return "control";
  case RejectionReason::Size:
    return "size";
  case RejectionReason::SlotIllegal:
    return "slot-illegal";
  }
  llvm_unreachable("unknown SH delay-slot rejection reason");
}

static bool isLateCallArgument(const MachineInstr &MI,
                               const MachineOperand &MO) {
  return MO.isImplicit() ||
         (MI.getOpcode() == SH::TAIL_JMP && MO.getOperandNo() != 0);
}

class RegisterDependencies {
  LiveRegUnits Defs;
  LiveRegUnits Uses;

public:
  explicit RegisterDependencies(const TargetRegisterInfo &TRI)
      : Defs(TRI), Uses(TRI) {}

  void addDef(Register Reg) { Defs.addReg(Reg); }
  void addUse(Register Reg) { Uses.addReg(Reg); }

  void addInstruction(const MachineInstr &MI) {
    for (const MachineOperand &MO : MI.operands()) {
      if (!MO.isReg() || !MO.getReg())
        continue;
      if (MO.isDef())
        addDef(MO.getReg());
      else
        addUse(MO.getReg());
    }
  }

  void addDelayedInstruction(const MachineInstr &MI) {
    if (MI.isCall()) {
      // The target and PR are consumed/defined when the delayed call is
      // issued. Argument uses and regmask clobbers occur after its slot.
      addDef(SH::PR);
      addUse(SH::R15);
      for (const MachineOperand &MO : MI.operands())
        if (MO.isReg() && MO.getReg() && MO.isUse() &&
            !isLateCallArgument(MI, MO))
          addUse(MO.getReg());
      return;
    }
    if (MI.isReturn()) {
      // The return target is captured before the slot. Return-value uses
      // describe the state observed after it and intentionally stay late.
      addUse(SH::PR);
      addUse(SH::R15);
      return;
    }
    addInstruction(MI);
  }

  RejectionReason check(const MachineInstr &MI, Register &HazardReg) const {
    for (const MachineOperand &MO : MI.operands()) {
      if (!MO.isReg() || !MO.getReg() || !MO.isUse())
        continue;
      if (!Defs.available(MO.getReg())) {
        HazardReg = MO.getReg();
        return RejectionReason::RAW;
      }
    }
    for (const MachineOperand &MO : MI.operands()) {
      if (!MO.isReg() || !MO.getReg() || !MO.isDef())
        continue;
      if (!Uses.available(MO.getReg())) {
        HazardReg = MO.getReg();
        return RejectionReason::WAR;
      }
      if (!Defs.available(MO.getReg())) {
        HazardReg = MO.getReg();
        return RejectionReason::WAW;
      }
    }
    return RejectionReason::None;
  }
};

class FillerImpl {
  const SHSubtarget *STI = nullptr;
  const SHInstrInfo *TII = nullptr;
  const TargetRegisterInfo *TRI = nullptr;

  void recordRejection(RejectionReason Reason, Register HazardReg) const {
    switch (Reason) {
    case RejectionReason::None:
      return;
    case RejectionReason::RAW:
      ++RawRejections;
      break;
    case RejectionReason::WAR:
      ++WarRejections;
      break;
    case RejectionReason::WAW:
      ++WawRejections;
      break;
    case RejectionReason::Memory:
      ++MemoryRejections;
      break;
    case RejectionReason::SideEffect:
      ++SideEffectRejections;
      break;
    case RejectionReason::Control:
      ++ControlRejections;
      break;
    case RejectionReason::Size:
      ++SizeRejections;
      break;
    case RejectionReason::SlotIllegal:
      ++SlotIllegalRejections;
      break;
    }
    if (HazardReg && HazardReg.isPhysical() &&
        !TRI->isInAllocatableClass(HazardReg.asMCReg()))
      ++SpecialStateRejections;
  }

  // Moving MI from before Delayed into its slot is legal only when MI can move
  // down across every accumulated instruction, remains legal on this CPU, and
  // has no control, memory, exception, size, or unmodeled-state hazard.
  CandidateDecision evaluateCandidate(const MachineInstr &MI,
                                      const MachineInstr &Delayed,
                                      const RegisterDependencies &Dependencies,
                                      Register &HazardReg,
                                      bool IsAdjacent) const {
    if (MI.isDebugInstr() || MI.isPseudoProbe())
      return {CandidateAction::Skip, RejectionReason::None};
    if (MI.isCFIInstruction() || MI.isPosition() || MI.isMetaInstruction() ||
        MI.isInlineAsm() || MI.isBundle() || MI.isBundledWithPred() ||
        MI.isBundledWithSucc())
      return {CandidateAction::Boundary, RejectionReason::SideEffect};
    if (MI.isCall() || MI.isReturn() || MI.isBranch() || MI.isTerminator() ||
        MI.isBarrier())
      return {CandidateAction::Boundary, RejectionReason::Control};
    if (MI.hasUnmodeledSideEffects() || MI.mayRaiseFPException())
      return {CandidateAction::Boundary, RejectionReason::SideEffect};
    // An unordered load can move across other loads and non-memory instructions
    // into a call or unconditional branch slot. Stores remain adjacent-only so
    // this pass needs no alias analysis. An adjacent memory operation can also
    // move into a return slot. The control target is captured before the slot,
    // and call arguments and return values are consumed after it.
    bool IsControlLoad =
        (Delayed.isCall() || Delayed.isUnconditionalBranch()) && MI.mayLoad() &&
        !MI.mayStore() && !MI.hasOrderedMemoryRef() &&
        !isExpandedPICLoadSequenceMember(MI);
    bool IsAdjacentCallStore = Delayed.isCall() && IsAdjacent &&
                               MI.mayStore() && !MI.mayLoad() &&
                               !MI.hasOrderedMemoryRef();
    bool IsAdjacentUnconditionalStore =
        Delayed.isUnconditionalBranch() && IsAdjacent && MI.mayStore() &&
        !MI.mayLoad() && !MI.hasOrderedMemoryRef();
    bool IsAdjacentReturnMemory =
        Delayed.isReturn() && IsAdjacent && MI.mayLoadOrStore() &&
        !MI.hasOrderedMemoryRef() && !isExpandedPICLoadSequenceMember(MI);
    bool IsAdjacentReturnRestore = Delayed.isReturn() && IsAdjacent &&
                                   MI.getOpcode() == SH::MOVL_load_postinc &&
                                   MI.getFlag(MachineInstr::FrameDestroy) &&
                                   !MI.hasOrderedMemoryRef();
    bool IsMovableControlMemory = IsControlLoad || IsAdjacentCallStore ||
                                  IsAdjacentUnconditionalStore ||
                                  IsAdjacentReturnMemory;
    bool IsLiteralLoad = MI.getOpcode() == SH::MOVL_load_pc_island ||
                         MI.getOpcode() == SH::MOVW_load_pc_island;
    if ((MI.mayLoadOrStore() || MI.hasOrderedMemoryRef()) &&
        !IsMovableControlMemory)
      return {CandidateAction::Boundary, RejectionReason::Memory};
    if (MI.isPseudo())
      // Literal loads expand after this pass. They cannot move into the slot,
      // but dependency tracking can safely scan behind them.
      return {IsLiteralLoad ? CandidateAction::Reject
                            : CandidateAction::Boundary,
              RejectionReason::Size};
    if (MI.getOpcode() == SH::NOP)
      return {CandidateAction::Reject, RejectionReason::None};
    if (!TII->isDelaySlotCandidate(MI, *STI))
      return {CandidateAction::Reject, RejectionReason::SlotIllegal};
    if (IsAdjacentReturnRestore)
      return {CandidateAction::Accept, RejectionReason::None};
    RejectionReason Reason = Dependencies.check(MI, HazardReg);
    if (Reason != RejectionReason::None)
      // A rejected load may be crossed by an older independent candidate.
      // Stores remain boundaries because this pass does not perform alias
      // analysis.
      return {MI.mayStore() ? CandidateAction::Boundary
                            : CandidateAction::Reject,
              Reason};
    if (Delayed.isCall())
      for (const MachineOperand &MO : MI.operands())
        if (MO.isReg() && MO.getReg().isPhysical() && MO.isDef() &&
            !TRI->isInAllocatableClass(MO.getReg().asMCReg())) {
          HazardReg = MO.getReg();
          return {CandidateAction::Reject, RejectionReason::SideEffect};
        }
    return {CandidateAction::Accept, RejectionReason::None};
  }

  bool findCandidate(MachineBasicBlock &MBB,
                     MachineBasicBlock::iterator Delayed,
                     MachineBasicBlock::iterator &Candidate,
                     unsigned &Distance) const {
    const MachineFunction &MF = *MBB.getParent();
    unsigned SearchLimit = MaxSearchInstructions;
    if (Delayed->isCall() &&
        MF.getTarget().getOptLevel() != CodeGenOptLevel::None &&
        !MF.getFunction().hasOptNone() && !MF.getFunction().hasOptSize() &&
        !MF.getFunction().hasMinSize())
      SearchLimit = MaxCallSearchInstructions;
    RegisterDependencies Dependencies(*TRI);
    Dependencies.addDelayedInstruction(*Delayed);
    Candidate = Delayed;
    Distance = 0;
    while (Candidate != MBB.begin()) {
      --Candidate;
      Register HazardReg;
      CandidateDecision Decision = evaluateCandidate(
          *Candidate, *Delayed, Dependencies, HazardReg, Distance == 0);
      if (Decision.Action == CandidateAction::Skip)
        continue;
      if (Distance == SearchLimit) {
        ++SearchBoundStops;
        LLVM_DEBUG(dbgs() << DEBUG_TYPE
                          << ": function=" << MBB.getParent()->getName()
                          << " result=search-bound\n");
        return false;
      }
      ++Distance;
      if (Decision.Action == CandidateAction::Accept) {
        SmallVector<MachineInstr *, 2> DbgValues;
        if (Candidate->getNumOperands() != 0)
          Candidate->collectDebugValues(DbgValues);
        if (DbgValues.empty())
          return true;
        Decision = {CandidateAction::Reject, RejectionReason::SideEffect};
      }
      recordRejection(Decision.Reason, HazardReg);
      LLVM_DEBUG(dbgs() << DEBUG_TYPE
                        << ": function=" << MBB.getParent()->getName()
                        << " distance=" << Distance << " result=reject reason="
                        << getReasonName(Decision.Reason)
                        << " instruction=" << *Candidate);
      if (Decision.Action == CandidateAction::Boundary)
        return false;
      Dependencies.addInstruction(*Candidate);
    }
    return false;
  }

  bool findSuccessorCandidate(MachineBasicBlock &Source,
                              MachineBasicBlock::iterator Branch,
                              MachineBasicBlock::iterator &Candidate) const {
    if (Branch->getOpcode() != SH::BRA ||
        Branch->getNumExplicitOperands() != 1 || !Branch->getOperand(0).isMBB())
      return false;
    MachineBasicBlock *Target = Branch->getOperand(0).getMBB();
    if (Target == &Source || Target->getSectionID() != Source.getSectionID() ||
        Target->pred_size() != 1 || *Target->pred_begin() != &Source ||
        Target->hasAddressTaken() || Target->isEHPad())
      return false;
    Candidate = Target->getFirstNonDebugInstr();
    if (Candidate == Target->end() ||
        Candidate->getFlag(MachineInstr::FrameSetup) ||
        Candidate->getFlag(MachineInstr::FrameDestroy) ||
        Candidate->getFlag(MachineInstr::NoMerge))
      return false;
    // PC-relative instructions must retain the address at which they execute.
    if (Candidate->getOpcode() == SH::MOVA ||
        Candidate->getOpcode() == SH::MOVL_load_pc ||
        Candidate->getOpcode() == SH::MOVW_load_pc ||
        Candidate->getOpcode() == SH::MOVL_load_pc_island ||
        Candidate->getOpcode() == SH::MOVW_load_pc_island)
      return false;
    RegisterDependencies Dependencies(*TRI);
    Dependencies.addDelayedInstruction(*Branch);
    Register HazardReg;
    CandidateDecision Decision =
        evaluateCandidate(*Candidate, *Branch, Dependencies, HazardReg, true);
    if (Decision.Action != CandidateAction::Accept) {
      recordRejection(Decision.Reason, HazardReg);
      return false;
    }
    SmallVector<MachineInstr *, 2> DbgValues;
    if (Candidate->getNumOperands() != 0)
      Candidate->collectDebugValues(DbgValues);
    return DbgValues.empty();
  }

  bool
  findConditionalSuccessorCopy(MachineBasicBlock &Source,
                               MachineBasicBlock::iterator Branch,
                               MachineBasicBlock::iterator &Candidate) const {
    const MachineFunction &MF = *Source.getParent();
    const Function &F = MF.getFunction();
    if (STI->getSchedModel().IssueWidth != 1 || F.hasOptSize() ||
        F.hasMinSize() || F.hasOptNone() ||
        MF.getTarget().getOptLevel() == CodeGenOptLevel::None ||
        !MF.getRegInfo().tracksLiveness() || Source.succ_size() != 2 ||
        std::next(Branch) != Source.end())
      return false;
    MachineBasicBlock *Target = Branch->getOperand(0).getMBB();
    if (Target == &Source || Target->pred_size() != 1 ||
        *Target->pred_begin() != &Source || Target->hasAddressTaken() ||
        Target->isEHPad() || Target->getSectionID() != Source.getSectionID())
      return false;
    MachineBasicBlock *Other = *Source.succ_begin() == Target
                                   ? *std::next(Source.succ_begin())
                                   : *Source.succ_begin();
    if (Other != Source.getNextNode())
      return false;
    // A useful slot saves two taken-transfer cycles on a single-issue CPU,
    // at the cost of one additional copy on the other edge. An unbiased
    // or more likely taken edge has positive expected benefit.
    MachineBranchProbabilityInfo MBPI;
    if (MBPI.getEdgeProbability(&Source, Target) < BranchProbability(1, 2))
      return false;
    Candidate = Target->getFirstNonDebugInstr();
    if (Candidate == Target->end() || Candidate->getOpcode() != SH::MOVrr ||
        Candidate->getNumOperands() != 2 || !Candidate->memoperands_empty() ||
        Candidate->getFlag(MachineInstr::FrameSetup) ||
        Candidate->getFlag(MachineInstr::FrameDestroy) ||
        Candidate->getFlag(MachineInstr::NoMerge))
      return false;
    const MachineOperand &Def = Candidate->getOperand(0);
    const MachineOperand &Use = Candidate->getOperand(1);
    if (!Def.isReg() || !Use.isReg() || Def.isDead() || Use.isUndef() ||
        Def.getSubReg() || Use.getSubReg())
      return false;
    Register Dst = Def.getReg(), Src = Use.getReg();
    if (!SH::GPRRegClass.contains(Dst) || !SH::GPRRegClass.contains(Src) ||
        TRI->regsOverlap(Dst, Src))
      return false;
    auto Reserved = TRI->getReservedRegs(MF);
    if (Reserved.test(Dst) || Reserved.test(Src) ||
        llvm::any_of(Other->liveins(), [&](const auto &LiveIn) {
          return TRI->regsOverlap(Dst, LiveIn.PhysReg);
        }))
      return false;
    // Do not take a copy that could fill the target's own transfer slot.
    // A local consumer before any transfer needs this definition in place.
    bool HasLocalUse = false;
    for (auto I = std::next(Candidate); I != Target->end(); ++I) {
      if (I->isDebugInstr())
        continue;
      if (I->isTerminator() || I->isCall())
        break;
      if (I->readsRegister(Dst, TRI)) {
        HasLocalUse = true;
        break;
      }
      if (I->modifiesRegister(Dst, TRI))
        break;
    }
    if (!HasLocalUse)
      return false;
    RegisterDependencies Dependencies(*TRI);
    Dependencies.addDelayedInstruction(*Branch);
    Register HazardReg;
    if (evaluateCandidate(*Candidate, *Branch, Dependencies, HazardReg, true)
            .Action != CandidateAction::Accept)
      return false;
    SmallVector<MachineInstr *, 2> DbgValues;
    Candidate->collectDebugValues(DbgValues);
    if (!DbgValues.empty())
      return false;

    return canMoveForwardWithoutPCRelativeUses(Source, Branch, Candidate);
  }

  bool canMoveForwardWithoutPCRelativeUses(
      MachineBasicBlock &Source, MachineBasicBlock::iterator Branch,
      MachineBasicBlock::iterator Candidate) const {
    const MachineFunction &MF = *Source.getParent();
    // Move only forward across an unaligned interval without PC-relative
    // uses or data. Its instructions shift by two bytes, while every literal
    // use and definition keeps its address. Check all branch ranges below.
    for (auto MBB = Source.getIterator(); MBB != MF.end(); ++MBB) {
      if (MBB->getSectionID() != Source.getSectionID() ||
          (&*MBB != &Source && MBB->getAlignment() > Align(2)))
        return false;
      auto Begin = &*MBB == &Source ? std::next(Branch) : MBB->begin();
      for (auto I = Begin; I != MBB->end(); ++I) {
        if (&*I == &*Candidate)
          return true;
        if (I->isInlineAsm() ||
            llvm::any_of(I->operands(), [](const MachineOperand &MO) {
              return MO.isCPI() || MO.isJTI() || MO.isMCSymbol() ||
                     MO.isGlobal() || MO.isSymbol() || MO.isBlockAddress();
            }))
          return false;
      }
    }
    return false;
  }

  bool isPureGPRCompare(const MachineInstr &MI) const {
    switch (MI.getOpcode()) {
    case SH::CMP_EQ:
    case SH::CMP_HI:
    case SH::CMP_HS:
    case SH::CMP_GT:
    case SH::CMP_GE:
      break;
    default:
      return false;
    }
    if (MI.getNumOperands() != 3 || MI.isBundled() || !MI.memoperands_empty() ||
        !TII->isDelaySlotCandidate(MI, *STI) ||
        MI.getFlag(MachineInstr::FrameSetup) ||
        MI.getFlag(MachineInstr::FrameDestroy) ||
        MI.getFlag(MachineInstr::NoMerge))
      return false;
    auto Reserved = TRI->getReservedRegs(*MI.getMF());
    for (unsigned I = 0; I != 2; ++I) {
      const MachineOperand &MO = MI.getOperand(I);
      if (!MO.isReg() || !MO.isUse() || MO.isImplicit() || MO.isUndef() ||
          MO.getSubReg() || !SH::GPRRegClass.contains(MO.getReg()) ||
          Reserved.test(MO.getReg()))
        return false;
    }
    const MachineOperand &T = MI.getOperand(2);
    return T.isReg() && T.isDef() && T.isImplicit() && T.getReg() == SH::TBit &&
           !T.isDead();
  }

  bool findLexicographicSuccessorCompare(
      MachineBasicBlock &Source, MachineBasicBlock::iterator Branch,
      MachineBasicBlock::iterator &Candidate) const {
    const MachineFunction &MF = *Source.getParent();
    const Function &F = MF.getFunction();
    if (STI->getSchedModel().IssueWidth != 1 || F.hasOptSize() ||
        F.hasMinSize() || F.hasOptNone() ||
        MF.getTarget().getOptLevel() == CodeGenOptLevel::None ||
        !MF.getRegInfo().tracksLiveness() || Source.succ_size() != 2 ||
        Branch->getOpcode() != SH::BF || Branch->isBundled() ||
        std::next(Branch) != Source.end())
      return false;
    MachineInstr *Equality = Branch->getPrevNode();
    while (Equality && Equality->isDebugInstr())
      Equality = Equality->getPrevNode();
    if (!Equality || Equality->getOpcode() != SH::CMP_EQ ||
        !isPureGPRCompare(*Equality))
      return false;
    MachineBasicBlock *Target = Branch->getOperand(0).getMBB();
    if (Target == &Source || Target->pred_size() != 1 ||
        *Target->pred_begin() != &Source || Target->hasAddressTaken() ||
        Target->isEHPad() || Target->getSectionID() != Source.getSectionID() ||
        Target->isLiveIn(SH::TBit))
      return false;
    MachineBasicBlock *Other = *Source.succ_begin() == Target
                                   ? *std::next(Source.succ_begin())
                                   : *Source.succ_begin();
    if (Other != Source.getNextNode() || Other->isLiveIn(SH::TBit))
      return false;
    // Equality selects the low-word comparison. That path must overwrite T
    // before any use; the high-word ordering result is only live on Target.
    auto LowCompare = Other->getFirstNonDebugInstr();
    if (LowCompare == Other->end() ||
        (LowCompare->getOpcode() != SH::CMP_HI &&
         LowCompare->getOpcode() != SH::CMP_HS) ||
        !isPureGPRCompare(*LowCompare))
      return false;
    Candidate = Target->getFirstNonDebugInstr();
    if (Candidate == Target->end() || Candidate->getOpcode() == SH::CMP_EQ ||
        !isPureGPRCompare(*Candidate) ||
        Candidate->getOperand(0).getReg() != Equality->getOperand(0).getReg() ||
        Candidate->getOperand(1).getReg() != Equality->getOperand(1).getReg())
      return false;
    auto Consumer = std::next(Candidate);
    while (Consumer != Target->end() && Consumer->isDebugInstr())
      ++Consumer;
    if (Consumer == Target->end() || Consumer->isBundled() ||
        (Consumer->getOpcode() != SH::BT && Consumer->getOpcode() != SH::BF))
      return false;
    // BF/S captures the equality T before the slot writes the ordering T.
    // Moving this pure compare forward preserves every Target T consumer,
    // not just its first branch. The ordinary backward WAR veto stays intact.
    // On a single-issue CPU, the taken edge saves two transfer cycles and
    // the other edge executes one extra compare. Require an unbiased edge.
    MachineBranchProbabilityInfo MBPI;
    if (MBPI.getEdgeProbability(&Source, Target) < BranchProbability(1, 2))
      return false;
    return canMoveForwardWithoutPCRelativeUses(Source, Branch, Candidate);
  }

  bool isConditionalConversionInRange(const MachineInstr &Branch,
                                      const MachineInstr &Candidate,
                                      const MachineBasicBlock &Target) const {
    const MachineBasicBlock &Source = *Branch.getParent();
    if (Source.getSectionID() != Target.getSectionID())
      return false;
    if (&Source == &Target)
      return true;

    const MachineFunction &MF = *Source.getParent();
    auto TargetBlock = std::next(Source.getIterator());
    while (TargetBlock != MF.end() && &*TargetBlock != &Target)
      ++TargetBlock;
    if (TargetBlock == MF.end())
      return true;
    // The corrected SH4 model regresses when forward slots are exposed. The
    // MOVA slot restriction marks that scheduling boundary; backward slots
    // remain profitable and enabled.
    if (STI->isMOVAIllegalInDelaySlot())
      return false;

    // Moving Candidate after Branch shifts Branch earlier by Candidate's size.
    // Use maximum possible block-alignment padding so a forward branch accepted
    // here remains in range for every valid final address.
    constexpr uint64_t MaxForwardOffset =
        SH::PCRelativeBias +
        ((UINT64_C(1) << (SH::ConditionalBranchDisplacementBits - 1)) - 1) *
            SH::BranchDisplacementScale;
    uint64_t Offset = 0;
    auto AddToOffset = [&](uint64_t Size) {
      if (Size > MaxForwardOffset || Offset > MaxForwardOffset - Size)
        return false;
      Offset += Size;
      return true;
    };

    for (auto I = Branch.getIterator(); I != Source.end(); ++I)
      if (!I->isBundledWithPred() && !AddToOffset(TII->getInstSizeInBytes(*I)))
        return false;
    for (auto MBB = std::next(Source.getIterator());; ++MBB) {
      if (MBB->getSectionID() != Source.getSectionID() ||
          !AddToOffset(MBB->getAlignment().value() - 1))
        return false;
      if (&*MBB == &Target)
        break;
      for (const MachineInstr &MI : *MBB)
        if (!MI.isBundledWithPred() &&
            !AddToOffset(TII->getInstSizeInBytes(MI)))
          return false;
    }

    return AddToOffset(TII->getInstSizeInBytes(Candidate));
  }

  bool preservesBranchRanges(const MachineInstr &Delayed,
                             const MachineInstr *Candidate,
                             const MachineInstr *RemovedNop,
                             bool IsConditional) const {
    return preservesSHBranchRanges(
        *Delayed.getMF(), *TII, [&](const MachineInstr &MI, unsigned Size) {
          if (&MI == Candidate || &MI == RemovedNop)
            return 0u;
          if (&MI == &Delayed && IsConditional)
            return Size + TII->getInstSizeInBytes(*Candidate);
          return Size;
        });
  }

  void markLateUsesInternal(MachineInstr &Delayed,
                            const MachineInstr &Candidate) const {
    if (!Delayed.isCall() && !Delayed.isReturn())
      return;
    for (MachineOperand &DelayedMO : Delayed.operands()) {
      if (!DelayedMO.isReg() || !DelayedMO.getReg() || !DelayedMO.isUse())
        continue;
      if (Delayed.isCall() && !isLateCallArgument(Delayed, DelayedMO))
        continue;
      if (Delayed.isReturn() && (TRI->regsOverlap(DelayedMO.getReg(), SH::PR) ||
                                 TRI->regsOverlap(DelayedMO.getReg(), SH::R15)))
        continue;
      for (const MachineOperand &CandidateMO : Candidate.operands())
        if (CandidateMO.isReg() && CandidateMO.getReg() &&
            CandidateMO.isDef() &&
            TRI->regsOverlap(DelayedMO.getReg(), CandidateMO.getReg())) {
          DelayedMO.setIsInternalRead();
          break;
        }
    }
  }

  void transferCrossedKillFlags(MachineBasicBlock::iterator Delayed,
                                MachineBasicBlock::iterator Candidate) const {
    for (auto I = std::next(Candidate); I != Delayed; ++I) {
      for (MachineOperand &MO : I->operands()) {
        if (!MO.isReg() || !MO.getReg() || !MO.isUse() || !MO.isKill())
          continue;
        for (MachineOperand &CandidateMO : Candidate->operands()) {
          if (!CandidateMO.isReg() || !CandidateMO.getReg() ||
              !CandidateMO.isUse() ||
              !TRI->regsOverlap(MO.getReg(), CandidateMO.getReg()))
            continue;
          MO.setIsKill(false);
          CandidateMO.setIsKill(true);
          break;
        }
      }
    }
  }

  void moveCandidate(MachineBasicBlock &MBB,
                     MachineBasicBlock::iterator Delayed,
                     MachineBasicBlock::iterator Candidate) const {
    markLateUsesInternal(*Delayed, *Candidate);
    transferCrossedKillFlags(Delayed, Candidate);
    MBB.splice(std::next(Delayed), &MBB, Candidate);
  }

  void moveSuccessorCandidate(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator Delayed,
                              MachineBasicBlock::iterator Candidate) const {
    MachineBasicBlock *Target = Candidate->getParent();
    for (const MachineOperand &MO : Candidate->operands()) {
      if (!MO.isReg() || !MO.getReg().isPhysical())
        continue;
      MCRegister Reg = MO.getReg().asMCReg();
      if (MO.isUse() && !MBB.isLiveIn(Reg))
        MBB.addLiveIn(Reg);
      if (MO.isDef() && !MO.isDead() && !Target->isLiveIn(Reg))
        Target->addLiveIn(Reg);
    }
    MBB.sortUniqueLiveIns();
    Target->sortUniqueLiveIns();
    MBB.splice(std::next(Delayed), Target, Candidate);
  }

  void
  moveConditionalSuccessorCopy(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator Delayed,
                               MachineBasicBlock::iterator Candidate) const {
    MachineBasicBlock *Target = Candidate->getParent();
    Target->addLiveIn(Candidate->getOperand(0).getReg().asMCReg());
    Target->sortUniqueLiveIns();
    // The source value may still be needed on either outgoing edge.
    Candidate->getOperand(1).setIsKill(false);
    MBB.splice(std::next(Delayed), Target, Candidate);
  }

  void moveLexicographicSuccessorCompare(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator Delayed,
      MachineBasicBlock::iterator Candidate) const {
    MachineBasicBlock *Target = Candidate->getParent();
    Target->addLiveIn(SH::TBit);
    Target->sortUniqueLiveIns();
    // The same inputs were already read by the equality test, but remain
    // needed until the slot and may also be live on either outgoing edge.
    MachineInstr *Equality = Delayed->getPrevNode();
    while (Equality->isDebugInstr())
      Equality = Equality->getPrevNode();
    for (unsigned I = 0; I != 2; ++I) {
      Equality->getOperand(I).setIsKill(false);
      Candidate->getOperand(I).setIsKill(false);
    }
    MBB.splice(std::next(Delayed), Target, Candidate);
  }

public:
  bool run(MachineFunction &MF) {
    STI = &MF.getSubtarget<SHSubtarget>();
    TII = STI->getInstrInfo();
    TRI = STI->getRegisterInfo();
    bool Changed = false;
    for (MachineBasicBlock &MBB : MF) {
      for (MachineBasicBlock::iterator I = MBB.begin(); I != MBB.end(); ++I) {
        bool IsUndelayedConditional =
            I->getOpcode() == SH::BT || I->getOpcode() == SH::BF;
        MachineBasicBlock *Target =
            IsUndelayedConditional && I->getOperand(0).isMBB()
                ? I->getOperand(0).getMBB()
                : nullptr;
        bool IsConvertibleConditional =
            STI->hasSH2() && IsUndelayedConditional && Target &&
            Target->getSectionID() == MBB.getSectionID();
        if (!I->getDesc().hasDelaySlot() && !IsConvertibleConditional)
          continue;
        if (I->isBundledWithSucc()) {
          ++DelayedSlots;
          if (I->isCall())
            ++CallSlots;
          auto Slot = std::next(I);
          if (Slot != MBB.end() && Slot->getOpcode() != SH::NOP)
            ++AlreadyFilledSlots;
          else
            ++NopSlots;
          continue;
        }

        auto ExistingSlot = std::next(I);
        bool HasReusableNop = !IsConvertibleConditional &&
                              ExistingSlot != MBB.end() &&
                              ExistingSlot->getOpcode() == SH::NOP &&
                              !ExistingSlot->isBundledWithPred();
        MachineBasicBlock::iterator Candidate;
        unsigned Distance;
        bool Found = I->getOpcode() != SH::RTE &&
                     findCandidate(MBB, I, Candidate, Distance);
        bool FoundInSuccessor = MF.getFunction().hasOptSize() && !Found &&
                                !IsConvertibleConditional &&
                                findSuccessorCandidate(MBB, I, Candidate);
        if (FoundInSuccessor)
          Distance = 1;
        Found |= FoundInSuccessor;
        bool FoundConditionalCopy =
            !Found && IsConvertibleConditional &&
            findConditionalSuccessorCopy(MBB, I, Candidate);
        if (FoundConditionalCopy)
          Distance = 1;
        Found |= FoundConditionalCopy;
        bool FoundLexicographicCompare =
            !Found && IsConvertibleConditional &&
            findLexicographicSuccessorCompare(MBB, I, Candidate);
        if (FoundLexicographicCompare)
          Distance = 1;
        Found |= FoundLexicographicCompare;
        if (IsConvertibleConditional && Found &&
            !isConditionalConversionInRange(*I, *Candidate, *Target))
          Found = false;
        if (Found &&
            !preservesBranchRanges(*I, &*Candidate,
                                   HasReusableNop ? &*ExistingSlot : nullptr,
                                   IsConvertibleConditional))
          Found = false;
        if (IsConvertibleConditional && !Found)
          continue;
        if (IsConvertibleConditional)
          I->setDesc(TII->get(I->getOpcode() == SH::BT ? SH::BT_S : SH::BF_S));
        ++DelayedSlots;
        if (I->isCall())
          ++CallSlots;
        if (Found) {
          if (HasReusableNop)
            ExistingSlot->eraseFromParent();
          if (FoundLexicographicCompare) {
            moveLexicographicSuccessorCompare(MBB, I, Candidate);
            ++LexicographicCompareSlots;
          } else if (FoundConditionalCopy) {
            moveConditionalSuccessorCopy(MBB, I, Candidate);
            ++ConditionalSuccessorSlots;
          } else if (FoundInSuccessor) {
            moveSuccessorCandidate(MBB, I, Candidate);
            ++SuccessorSlots;
          } else {
            moveCandidate(MBB, I, Candidate);
          }
          ++NewlyFilledSlots;
          if (I->isCall())
            ++FilledCallSlots;
          if (I->isCall() && std::next(I)->mayLoad()) {
            if (Distance == 1)
              ++AdjacentCallLoadSlots;
            else
              ++NonAdjacentCallLoadSlots;
          }
          if (I->isCall() && std::next(I)->mayStore())
            ++AdjacentCallStoreSlots;
          if (I->isUnconditionalBranch() && std::next(I)->mayLoadOrStore()) {
            if (Distance == 1)
              ++AdjacentUnconditionalMemorySlots;
            else
              ++NonAdjacentUnconditionalLoadSlots;
          }
          LLVM_DEBUG(dbgs() << DEBUG_TYPE << ": function=" << MF.getName()
                            << " distance=" << Distance
                            << " result=filled instruction=" << *std::next(I));
        } else {
          if (HasReusableNop &&
              !preservesBranchRanges(*I, nullptr, &*ExistingSlot, false))
            HasReusableNop = false;
          if (!HasReusableNop)
            BuildMI(MBB, ExistingSlot, DebugLoc(), TII->get(SH::NOP));
          ++NopSlots;
          ++NoCandidateSlots;
          LLVM_DEBUG(dbgs() << DEBUG_TYPE << ": function=" << MF.getName()
                            << " result=nop delayed=" << *I);
        }
        auto Filler = std::next(I);
        MIBundleBuilder(MBB, I, std::next(Filler));
        I = Filler;
        Changed = true;
      }
    }
    return Changed;
  }
};

class SHDelaySlotFillerLegacy : public MachineFunctionPass {
public:
  static char ID;
  SHDelaySlotFillerLegacy() : MachineFunctionPass(ID) {}
  StringRef getPassName() const override { return "SH Delay Slot Filler"; }
  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().setNoVRegs();
  }
  bool runOnMachineFunction(MachineFunction &MF) override {
    FillerImpl Impl;
    return Impl.run(MF);
  }
};

char SHDelaySlotFillerLegacy::ID = 0;

} // namespace

INITIALIZE_PASS(SHDelaySlotFillerLegacy, DEBUG_TYPE, "Fill SH delay slots",
                false, false)

FunctionPass *llvm::createSHDelaySlotFillerLegacyPass() {
  return new SHDelaySlotFillerLegacy();
}

PreservedAnalyses
SHDelaySlotFillerPass::run(MachineFunction &MF,
                           MachineFunctionAnalysisManager &MFAM) {
  FillerImpl Impl;
  return Impl.run(MF) ? getMachineFunctionPassPreservedAnalyses()
                            .preserveSet<CFGAnalyses>()
                      : PreservedAnalyses::all();
}
