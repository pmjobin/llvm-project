//===-- SHFPUState.cpp - Establish SH-4 FPSCR execution modes ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SH.h"
#include "SHInstrInfo.h"
#include "SHMachineFunctionInfo.h"
#include "SHSubtarget.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachinePassManager.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "sh-fpu-state"

namespace {

using SHII::FPURequirement;

enum FPUStateMask : unsigned {
  StateSingle = 1U << 0,
  StateDouble = 1U << 1,
  StateABIBoundary = 1U << 2,
  StatePairedSingle = 1U << 3,
  StateAlternateBank = 1U << 4,
  StatePrimary =
      StateSingle | StateDouble | StateABIBoundary | StatePairedSingle,
  StateUnknown = StatePrimary | StateAlternateBank,
};

struct FPUABIContract {
  bool HasBoundary;
  FPURequirement BoundaryRequirement;
  unsigned BoundaryState;
  bool EFloat;
};

static FPUABIContract getFPUABIContract(const SHSubtarget &Subtarget) {
  SH::ABIKind ABI = SH::getBaseABI(Subtarget.getABI());
  if (ABI == SH::ABIKind::GNUHardFloat)
    return {true, FPURequirement::ABIBoundary, StateABIBoundary, false};
  if (ABI == SH::ABIKind::GNUHardFloatSingle)
    return {true, FPURequirement::Single, StateSingle, false};
  if (ABI == SH::ABIKind::GNUEFloat)
    return {true, FPURequirement::Single, StateSingle, true};
  return {false, FPURequirement::None, StateUnknown, false};
}

static unsigned stateAfterCall(const FPUABIContract &Contract) {
  return Contract.HasBoundary ? Contract.BoundaryState : StateUnknown;
}

static FPURequirement getRequirement(const MachineInstr &MI,
                                     const MachineRegisterInfo &MRI,
                                     const FPUABIContract &Contract,
                                     bool IsHandler) {
  if (Contract.HasBoundary && (MI.isCall() || (MI.isReturn() && !IsHandler)))
    return Contract.BoundaryRequirement;
  if (MI.getOpcode() == TargetOpcode::COPY) {
    for (const MachineOperand &MO : MI.operands()) {
      if (!MO.isReg() || !MO.getReg())
        continue;
      Register Reg = MO.getReg();
      if ((Reg.isPhysical() &&
           (SH::FRRegClass.contains(Reg) || SH::DRRegClass.contains(Reg) ||
            SH::FVRegClass.contains(Reg))) ||
          (Reg.isVirtual() &&
           (SH::FRRegClass.hasSubClassEq(MRI.getRegClass(Reg)) ||
            SH::DRRegClass.hasSubClassEq(MRI.getRegClass(Reg)) ||
            SH::FVRegClass.hasSubClassEq(MRI.getRegClass(Reg)))))
        return FPURequirement::ScalarMove;
    }
  }
  return SHII::getFPURequirement(MI.getDesc());
}

static bool invalidatesState(const MachineInstr &MI) {
  // SET_FPSCR_FIELDS intentionally preserves current PR/SZ/FR. Keep that
  // operation whole until after late mode validation; a raw LDS would lose
  // this knowledge, and a saved full-word restore could erase new status.
  return MI.isCall() || MI.isInlineAsm() || MI.getOpcode() == SH::LDS_FPSCR ||
         MI.getOpcode() == SH::LDS_L_FPSCR || MI.getOpcode() == SH::FRCHG ||
         MI.getOpcode() == SH::FSCHG || MI.getOpcode() == SH::FPCHG;
}

static unsigned toggleSize(unsigned State) {
  // FSCHG is legal only with PR=0. Do not infer a useful state from an
  // externally supplied toggle in a possibly illegal precision mode.
  if (State == 0 || (State & ~(StateSingle | StatePairedSingle)))
    return StateUnknown;
  return ((State & StateSingle) ? StatePairedSingle : 0) |
         ((State & StatePairedSingle) ? StateSingle : 0);
}

static unsigned togglePrecision(unsigned State) {
  if (State == 0 || (State & StateAlternateBank))
    return StateUnknown;
  return ((State & StateSingle) ? StateABIBoundary : 0) |
         ((State & StateABIBoundary) ? StateSingle : 0) |
         ((State & StateDouble) ? StatePairedSingle : 0) |
         ((State & StatePairedSingle) ? StateDouble : 0);
}

static bool satisfies(unsigned State, FPURequirement Requirement) {
  switch (Requirement) {
  case FPURequirement::None:
    return true;
  case FPURequirement::PrimaryBank:
    return (State & StateAlternateBank) == 0;
  case FPURequirement::Single:
    return State == StateSingle;
  case FPURequirement::PairedSingle:
    return State == StatePairedSingle;
  case FPURequirement::SingleArithmetic:
    return State != 0 && (State & ~(StateSingle | StatePairedSingle)) == 0;
  case FPURequirement::Double:
    return State == StateDouble;
  case FPURequirement::DoubleArithmetic:
    return State != 0 && (State & ~(StateDouble | StateABIBoundary)) == 0;
  case FPURequirement::ABIBoundary:
    return State == StateABIBoundary;
  case FPURequirement::ScalarMove:
    return State != 0 && (State & ~(StateSingle | StateABIBoundary)) == 0;
  }
  llvm_unreachable("invalid SH FPU requirement");
}

static unsigned applyRequirement(unsigned State, FPURequirement Requirement) {
  if (satisfies(State, Requirement))
    return State;
  switch (Requirement) {
  case FPURequirement::None:
    return State;
  case FPURequirement::PrimaryBank:
    return StatePrimary;
  case FPURequirement::Single:
    return StateSingle;
  case FPURequirement::PairedSingle:
    return StatePairedSingle;
  case FPURequirement::SingleArithmetic:
    return StateSingle;
  case FPURequirement::Double:
    return StateDouble;
  case FPURequirement::DoubleArithmetic:
  case FPURequirement::ABIBoundary:
    return StateABIBoundary;
  case FPURequirement::ScalarMove:
    llvm_unreachable("scalar move transition must select an exact requirement");
  }
  llvm_unreachable("invalid SH FPU requirement");
}

static std::optional<unsigned> getEstablishedState(const MachineInstr &MI) {
  if (MI.getOpcode() != SH::SET_FPSCR_MODE &&
      MI.getOpcode() != SH::SET_FPSCR_MODE_TOGGLE &&
      MI.getOpcode() != SH::TRANSITION_FPSCR_MODE &&
      MI.getOpcode() != SH::TRANSITION_FPSCR_MODE_REG)
    return std::nullopt;
  switch (MI.getOperand(0).getImm()) {
  case 0:
    return StatePrimary;
  case 1:
    return StateSingle;
  case 2:
    return StateDouble;
  case 3:
    return StateABIBoundary;
  case 4:
    return StatePairedSingle;
  default:
    report_fatal_error("SH FPSCR mode pseudo has an invalid mode");
  }
}

static std::optional<unsigned> getExactFPSCRMode(unsigned State) {
  if (State == StateSingle)
    return 1;
  if (State == StateDouble)
    return 2;
  if (State == StateABIBoundary)
    return 3;
  if (State == StatePairedSingle)
    return 4;
  return std::nullopt;
}

static unsigned getFPSCRModeBits(unsigned Mode) {
  switch (Mode) {
  case 1:
    return 0;
  case 2:
    return 3U << 19;
  case 3:
    return 1U << 19;
  case 4:
    return 1U << 20;
  default:
    llvm_unreachable("FPSCR transition requires an exact mode");
  }
}

static bool modeIsRedundant(unsigned State, int64_t Mode) {
  switch (Mode) {
  case 0:
    return satisfies(State, FPURequirement::PrimaryBank);
  case 1:
    return State == StateSingle;
  case 2:
    return State == StateDouble;
  case 3:
    return State == StateABIBoundary;
  case 4:
    return State == StatePairedSingle;
  default:
    report_fatal_error("SH FPSCR mode pseudo has an invalid mode");
  }
}

static bool hasFPURegClass(Register Reg, const MachineRegisterInfo &MRI) {
  if (!Reg)
    return false;
  if (Reg.isPhysical())
    return SH::FRRegClass.contains(Reg) || SH::DRRegClass.contains(Reg) ||
           SH::FVRegClass.contains(Reg);
  const TargetRegisterClass *RC = MRI.getRegClass(Reg);
  return SH::FRRegClass.hasSubClassEq(RC) || SH::DRRegClass.hasSubClassEq(RC) ||
         SH::FVRegClass.hasSubClassEq(RC);
}

static bool isHardFloatComplexReturn(Type *Ty) {
  const auto *StructTy = dyn_cast<StructType>(Ty);
  return StructTy && !StructTy->isPacked() && StructTy->getNumElements() == 2 &&
         StructTy->getElementType(0) == StructTy->getElementType(1) &&
         (StructTy->getElementType(0)->isFloatTy() ||
          StructTy->getElementType(0)->isDoubleTy());
}

static SmallVector<bool, 16> getFPUPhiSuccessors(MachineFunction &MF) {
  const MachineRegisterInfo &MRI = MF.getRegInfo();
  SmallVector<bool, 16> Result(MF.getNumBlockIDs(), false);
  for (MachineBasicBlock &MBB : MF)
    for (MachineInstr &MI : MBB.phis())
      if (hasFPURegClass(MI.getOperand(0).getReg(), MRI)) {
        Result[MBB.getNumber()] = true;
        break;
      }
  return Result;
}

static FPURequirement getTransitionRequirement(const MachineInstr &MI,
                                               FPURequirement Requirement,
                                               const MachineRegisterInfo &MRI,
                                               const FPUABIContract &Contract,
                                               bool IsHandler) {
  FPURequirement Established = Requirement == FPURequirement::SingleArithmetic
                                   ? FPURequirement::Single
                                   : Requirement;
  if (Requirement == FPURequirement::PrimaryBank ||
      Requirement == FPURequirement::ScalarMove)
    for (auto Later = std::next(MI.getIterator());
         Later != MI.getParent()->instr_end(); ++Later) {
      if (invalidatesState(*Later))
        break;
      FPURequirement LaterRequirement =
          getRequirement(*Later, MRI, Contract, IsHandler);
      if (LaterRequirement == FPURequirement::SingleArithmetic)
        LaterRequirement = FPURequirement::Single;
      if (LaterRequirement == FPURequirement::PairedSingle) {
        Established = Requirement == FPURequirement::ScalarMove
                          ? FPURequirement::Single
                          : LaterRequirement;
        break;
      }
      if (LaterRequirement == FPURequirement::Single ||
          LaterRequirement == FPURequirement::Double ||
          LaterRequirement == FPURequirement::DoubleArithmetic ||
          LaterRequirement == FPURequirement::ABIBoundary) {
        Established = Requirement == FPURequirement::ScalarMove &&
                              LaterRequirement != FPURequirement::Single
                          ? FPURequirement::ABIBoundary
                          : LaterRequirement;
        break;
      }
    }
  return Established == FPURequirement::ScalarMove ? FPURequirement::ABIBoundary
                                                   : Established;
}

static unsigned transferBlock(const MachineBasicBlock &MBB, unsigned State,
                              bool NeedsPhiBank, const MachineRegisterInfo &MRI,
                              const FPUABIContract &Contract, bool IsHandler) {
  for (const MachineInstr &MI : MBB) {
    if (std::optional<unsigned> Established = getEstablishedState(MI))
      State = *Established;
    else {
      FPURequirement Requirement = getRequirement(MI, MRI, Contract, IsHandler);
      if (!satisfies(State, Requirement))
        State = applyRequirement(State,
                                 getTransitionRequirement(MI, Requirement, MRI,
                                                          Contract, IsHandler));
    }
    if (MI.getOpcode() == SH::FSCHG)
      State = toggleSize(State);
    else if (MI.getOpcode() == SH::FPCHG)
      State = togglePrecision(State);
    else if (invalidatesState(MI))
      State = MI.isCall() ? stateAfterCall(Contract) : StateUnknown;
  }
  if (NeedsPhiBank)
    State = applyRequirement(State, FPURequirement::PrimaryBank);
  return State;
}

static void insertMode(MachineFunction &MF, MachineBasicBlock &MBB,
                       MachineBasicBlock::iterator Insert, const DebugLoc &DL,
                       const SHInstrInfo &TII, unsigned State, int64_t Mode,
                       bool PreserveScratch,
                       std::optional<std::pair<int, int>> &ScratchFIs,
                       bool PreserveR0 = false, bool PreserveR1 = false) {
  if ((State == StateSingle && Mode == 4) ||
      (State == StatePairedSingle && Mode == 1)) {
    if (PreserveScratch)
      BuildMI(MBB, Insert, DL, TII.get(SH::FSCHG));
    else
      BuildMI(MBB, Insert, DL, TII.get(SH::SET_FPSCR_MODE_TOGGLE)).addImm(Mode);
    return;
  }
  if (MF.getSubtarget<SHSubtarget>().hasSH4ACommon() &&
      ((State == StateSingle && Mode == 3) ||
       (State == StateABIBoundary && Mode == 1))) {
    if (PreserveScratch)
      BuildMI(MBB, Insert, DL, TII.get(SH::FPCHG));
    else
      BuildMI(MBB, Insert, DL, TII.get(SH::SET_FPSCR_MODE_TOGGLE)).addImm(Mode);
    return;
  }
  if (MF.getSubtarget<SHSubtarget>().hasSH4ACommon() &&
      MF.getSubtarget<SHSubtarget>().hasDoubleFPU() &&
      ((State == StateABIBoundary && Mode == 4) ||
       (State == StatePairedSingle && Mode == 3))) {
    if (!PreserveScratch) {
      BuildMI(MBB, Insert, DL, TII.get(SH::SET_FPSCR_MODE_TOGGLE)).addImm(Mode);
    } else if (State == StateABIBoundary) {
      // FSCHG cannot execute with PR=1. Clear PR before enabling paired moves.
      BuildMI(MBB, Insert, DL, TII.get(SH::FPCHG));
      BuildMI(MBB, Insert, DL, TII.get(SH::FSCHG));
    } else {
      BuildMI(MBB, Insert, DL, TII.get(SH::FSCHG));
      BuildMI(MBB, Insert, DL, TII.get(SH::FPCHG));
    }
    return;
  }
  auto BuildMode = [&]() {
    std::optional<unsigned> SourceMode = getExactFPSCRMode(State);
    if (SourceMode && Mode >= 1 && Mode <= 4 && *SourceMode != Mode) {
      unsigned Mask = getFPSCRModeBits(*SourceMode) ^ getFPSCRModeBits(Mode);
      return BuildMI(MBB, Insert, DL, TII.get(SH::TRANSITION_FPSCR_MODE))
          .addImm(Mode)
          .addImm(Mask >> 16);
    }
    return BuildMI(MBB, Insert, DL, TII.get(SH::SET_FPSCR_MODE)).addImm(Mode);
  };
  if (!PreserveScratch && !PreserveR0 && !PreserveR1) {
    BuildMode();
    return;
  }
  LivePhysRegs LiveRegs(TII.getRegisterInfo());
  LiveRegs.addLiveOuts(MBB);
  for (MachineBasicBlock::reverse_iterator I = MBB.rbegin(), E(Insert); I != E;
       ++I)
    LiveRegs.stepBackward(*I);
  bool R0Live = PreserveR0 || LiveRegs.contains(SH::R0);
  bool R1Live = PreserveR1 || LiveRegs.contains(SH::R1);
  if (!PreserveScratch) {
    MachineRegisterInfo &MRI = MF.getRegInfo();
    Register SavedR0 =
        R0Live ? MRI.createVirtualRegister(&SH::GPRRegClass) : Register();
    Register SavedR1 =
        R1Live ? MRI.createVirtualRegister(&SH::GPRRegClass) : Register();
    if (SavedR0)
      BuildMI(MBB, Insert, DL, TII.get(SH::MOVrr), SavedR0).addReg(SH::R0);
    if (SavedR1)
      BuildMI(MBB, Insert, DL, TII.get(SH::MOVrr), SavedR1).addReg(SH::R1);
    MachineInstrBuilder ModeMI = BuildMode();
    MachineBasicBlock::iterator Restore = std::next(ModeMI->getIterator());
    if (SavedR1)
      BuildMI(MBB, Restore, DL, TII.get(SH::MOVrr), SH::R1)
          .addReg(SavedR1, RegState::Kill);
    if (SavedR0)
      BuildMI(MBB, Restore, DL, TII.get(SH::MOVrr), SH::R0)
          .addReg(SavedR0, RegState::Kill);
    return;
  }
  if ((R0Live || R1Live) && !ScratchFIs) {
    MachineFrameInfo &MFI = MF.getFrameInfo();
    ScratchFIs = std::make_pair(MFI.CreateSpillStackObject(4, Align(4)),
                                MFI.CreateSpillStackObject(4, Align(4)));
  }
  if (R0Live)
    TII.storeRegToStackSlot(MBB, Insert, SH::R0, true, ScratchFIs->first,
                            &SH::GPRRegClass, Register(),
                            MachineInstr::NoFlags);
  if (R1Live)
    TII.storeRegToStackSlot(MBB, Insert, SH::R1, true, ScratchFIs->second,
                            &SH::GPRRegClass, Register(),
                            MachineInstr::NoFlags);
  MachineInstrBuilder ModeMI = BuildMode();
  MachineBasicBlock::iterator Restore = std::next(ModeMI->getIterator());
  if (R1Live)
    TII.loadRegFromStackSlot(MBB, Restore, SH::R1, ScratchFIs->second,
                             &SH::GPRRegClass, Register(), 0,
                             MachineInstr::NoFlags);
  if (R0Live)
    TII.loadRegFromStackSlot(MBB, Restore, SH::R0, ScratchFIs->first,
                             &SH::GPRRegClass, Register(), 0,
                             MachineInstr::NoFlags);
}

static bool
establishHomogeneousLoop(MachineFunction &MF, MachineLoop &Loop,
                         const SHInstrInfo &TII, const MachineRegisterInfo &MRI,
                         const FPUABIContract &Contract, bool IsHandler,
                         unsigned InitialState, bool PreserveScratch,
                         std::optional<std::pair<int, int>> &ScratchFIs) {
  unsigned Requirements = 0;
  bool Invalidated = false;
  for (MachineBasicBlock *MBB : Loop.blocks())
    for (const MachineInstr &MI : *MBB) {
      FPURequirement Requirement = getRequirement(MI, MRI, Contract, IsHandler);
      if (Requirement == FPURequirement::Single)
        Requirements |= StateSingle;
      else if (Requirement == FPURequirement::Double)
        Requirements |= StateDouble;
      else if (Requirement == FPURequirement::DoubleArithmetic)
        Requirements |= StateABIBoundary;
      else if (Requirement == FPURequirement::SingleArithmetic ||
               Requirement == FPURequirement::PairedSingle)
        Invalidated = true; // Do not hoist a conflicting scalar loop mode.
      Invalidated |= invalidatesState(MI);
    }
  if (!Invalidated &&
      (Requirements == StateSingle || Requirements == StateDouble ||
       Requirements == StateABIBoundary))
    if (MachineBasicBlock *Preheader = Loop.getLoopPreheader()) {
      FPURequirement Requirement =
          Requirements == StateSingle   ? FPURequirement::Single
          : Requirements == StateDouble ? FPURequirement::Double
                                        : FPURequirement::DoubleArithmetic;
      // Use the same entry contract as the main dataflow, including
      // hardware handlers that can interrupt an arbitrary FPU mode.
      unsigned IncomingState =
          Preheader == &MF.front() ? InitialState : StateUnknown;
      unsigned PreheaderState = transferBlock(*Preheader, IncomingState, false,
                                              MRI, Contract, IsHandler);
      if (satisfies(PreheaderState, Requirement))
        return false;
      int64_t Mode = Requirements == StateSingle   ? 1
                     : Requirements == StateDouble ? 2
                                                   : 3;
      insertMode(MF, *Preheader, Preheader->getFirstTerminator(), DebugLoc(),
                 TII, PreheaderState, Mode, PreserveScratch, ScratchFIs);
      return true;
    }
  bool Modified = false;
  for (MachineLoop *SubLoop : Loop.getSubLoops())
    Modified |=
        establishHomogeneousLoop(MF, *SubLoop, TII, MRI, Contract, IsHandler,
                                 InitialState, PreserveScratch, ScratchFIs);
  return Modified;
}

static bool reuseTransitionMasks(MachineFunction &MF, MachineLoop &Loop,
                                 const SHInstrInfo &TII) {
  bool Modified = false;
  for (MachineLoop *SubLoop : Loop.getSubLoops())
    Modified |= reuseTransitionMasks(MF, *SubLoop, TII);
  MachineBasicBlock *Preheader = Loop.getLoopPreheader();
  if (!Preheader)
    return Modified;
  MapVector<int64_t, SmallVector<MachineInstr *, 8>> Transitions;
  for (MachineBasicBlock *MBB : Loop.blocks())
    for (MachineInstr &MI : *MBB)
      if (MI.getOpcode() == SH::TRANSITION_FPSCR_MODE)
        Transitions[MI.getOperand(1).getImm()].push_back(&MI);
  MachineRegisterInfo &MRI = MF.getRegInfo();
  for (auto &[Mask, Uses] : Transitions) {
    if (Uses.size() < 2)
      continue;
    MachineBasicBlock::iterator Insert = Preheader->getFirstTerminator();
    Register FullMask = MRI.createVirtualRegister(&SH::GPRNoR0RegClass);
    BuildMI(*Preheader, Insert, DebugLoc(), TII.get(SH::MATERIALIZE_FPSCR_MASK),
            FullMask)
        .addImm(Mask);
    for (MachineInstr *MI : Uses) {
      BuildMI(*MI->getParent(), *MI, MI->getDebugLoc(),
              TII.get(SH::TRANSITION_FPSCR_MODE_REG))
          .add(MI->getOperand(0))
          .addReg(FullMask)
          .addImm(Mask)
          .setMIFlags(MI->getFlags());
      MI->eraseFromParent();
    }
    Modified = true;
  }
  return Modified;
}

class FPUStateImpl {
  bool PreserveScratch;

public:
  explicit FPUStateImpl(bool PreserveScratch)
      : PreserveScratch(PreserveScratch) {}
  bool run(MachineFunction &MF, MachineLoopInfo &MLI) {
    const SHSubtarget &Subtarget = MF.getSubtarget<SHSubtarget>();
    if (!Subtarget.hasSingleFPU())
      return false;
    MF.RenumberBlocks();
    const MachineRegisterInfo &MRI = MF.getRegInfo();
    const SHInstrInfo &TII = *Subtarget.getInstrInfo();
    FPUABIContract Contract = getFPUABIContract(Subtarget);
    bool IsHandler = MF.getInfo<SHMachineFunctionInfo>()->isHandler();
    unsigned InitialState =
        Contract.HasBoundary ? Contract.BoundaryState : StateUnknown;
    if (IsHandler)
      InitialState =
          !Subtarget.hasFPUSizeMode() && !Subtarget.hasFPURegisterBank()
              ? StateSingle
              : StateUnknown;
    std::optional<std::pair<int, int>> ScratchFIs;
    bool Modified = false;
    for (MachineLoop *Loop : MLI)
      Modified |=
          establishHomogeneousLoop(MF, *Loop, TII, MRI, Contract, IsHandler,
                                   InitialState, PreserveScratch, ScratchFIs);
    SmallVector<bool, 16> HasFPUPhis = getFPUPhiSuccessors(MF);
    SmallVector<unsigned, 16> Incoming(MF.getNumBlockIDs(), 0);
    SmallVector<unsigned, 16> Outgoing(MF.getNumBlockIDs(), 0);
    Incoming[MF.front().getNumber()] = InitialState;
    bool Changed;
    do {
      Changed = false;
      for (MachineBasicBlock &MBB : MF) {
        int Number = MBB.getNumber();
        unsigned NewIncoming =
            Number == MF.front().getNumber() ? InitialState : 0U;
        for (MachineBasicBlock *Pred : MBB.predecessors())
          NewIncoming |= Outgoing[Pred->getNumber()];
        if (NewIncoming == 0 && MBB.pred_empty())
          NewIncoming = InitialState;
        bool NeedsPhiBank =
            llvm::any_of(MBB.successors(), [&](MachineBasicBlock *Succ) {
              return HasFPUPhis[Succ->getNumber()];
            });
        unsigned NewOutgoing =
            NewIncoming == 0 ? 0
                             : transferBlock(MBB, NewIncoming, NeedsPhiBank,
                                             MRI, Contract, IsHandler);
        if (Incoming[Number] != NewIncoming ||
            Outgoing[Number] != NewOutgoing) {
          Incoming[Number] = NewIncoming;
          Outgoing[Number] = NewOutgoing;
          Changed = true;
        }
      }
    } while (Changed);

    for (MachineBasicBlock &MBB : MF) {
      unsigned State = Incoming[MBB.getNumber()] == 0
                           ? InitialState
                           : Incoming[MBB.getNumber()];
      for (MachineInstr &MI : make_early_inc_range(MBB)) {
        if (std::optional<unsigned> Established = getEstablishedState(MI)) {
          if (PreserveScratch &&
              modeIsRedundant(State, MI.getOperand(0).getImm())) {
            MI.eraseFromParent();
            Modified = true;
            continue;
          }
          if (PreserveScratch) {
            // Register allocation can insert scalar copies/spills between a
            // planned mode change and its predecessor. Revalidate relative
            // transitions against the actual state; never execute a stale
            // toggle or XOR mask merely because its target mode is known.
            unsigned Opcode = MI.getOpcode();
            int64_t Mode = MI.getOperand(0).getImm();
            bool Repair = Opcode == SH::SET_FPSCR_MODE_TOGGLE;
            if (Opcode == SH::TRANSITION_FPSCR_MODE ||
                Opcode == SH::TRANSITION_FPSCR_MODE_REG) {
              auto Source = getExactFPSCRMode(State);
              int64_t Mask =
                  MI.getOperand(Opcode == SH::TRANSITION_FPSCR_MODE ? 1 : 2)
                      .getImm();
              Repair = !Source || Mask != int64_t((getFPSCRModeBits(*Source) ^
                                                   getFPSCRModeBits(Mode)) >>
                                                  16);
            }
            if (Repair) {
              insertMode(MF, MBB, MI, MI.getDebugLoc(), TII, State, Mode, true,
                         ScratchFIs);
              MI.eraseFromParent();
              Modified = true;
            }
          }
          State = *Established;
          continue;
        }
        FPURequirement Requirement =
            getRequirement(MI, MRI, Contract, IsHandler);
        if (!satisfies(State, Requirement)) {
          FPURequirement Established = getTransitionRequirement(
              MI, Requirement, MRI, Contract, IsHandler);
          int64_t Mode = Established == FPURequirement::PrimaryBank    ? 0
                         : Established == FPURequirement::Single       ? 1
                         : Established == FPURequirement::Double       ? 2
                         : Established == FPURequirement::PairedSingle ? 4
                                                                       : 3;
          bool PreserveR0 = false;
          bool PreserveR1 = false;
          if (Contract.HasBoundary &&
              Requirement == Contract.BoundaryRequirement && MI.isReturn()) {
            const Function &F = MF.getFunction();
            Type *ReturnTy = F.getReturnType();
            bool ComplexFPUResult =
                isHardFloatComplexReturn(ReturnTy) &&
                (!Contract.EFloat ||
                 cast<StructType>(ReturnTy)->getElementType(0)->isFloatTy());
            bool FPUResult = ReturnTy->isFloatTy() || ComplexFPUResult ||
                             (!Contract.EFloat && ReturnTy->isDoubleTy());
            PreserveR0 =
                F.hasStructRetAttr() || (!ReturnTy->isVoidTy() && !FPUResult);
            PreserveR1 = PreserveR0 && !F.hasStructRetAttr() &&
                         MF.getDataLayout().getTypeStoreSize(ReturnTy) > 4;
          }
          // Selected indexed accesses already consume a physical R0.
          // Preserve fixed inputs even before allocation, when ordinary
          // virtual operands can still avoid the mode-setting scratch.
          PreserveR0 |= MI.readsRegister(SH::R0, &TII.getRegisterInfo());
          PreserveR1 |= MI.readsRegister(SH::R1, &TII.getRegisterInfo());
          insertMode(MF, MBB, MI, MI.getDebugLoc(), TII, State, Mode,
                     PreserveScratch, ScratchFIs, PreserveR0, PreserveR1);
          State = applyRequirement(State, Established);
          Modified = true;
        }
        if (MI.getOpcode() == SH::FSCHG)
          State = toggleSize(State);
        else if (MI.getOpcode() == SH::FPCHG)
          State = togglePrecision(State);
        else if (invalidatesState(MI))
          State = MI.isCall() ? stateAfterCall(Contract) : StateUnknown;
      }
      bool NeedsPhiBank =
          llvm::any_of(MBB.successors(), [&](MachineBasicBlock *Succ) {
            return HasFPUPhis[Succ->getNumber()];
          });
      if (NeedsPhiBank && !satisfies(State, FPURequirement::PrimaryBank)) {
        insertMode(MF, MBB, MBB.getFirstTerminator(), DebugLoc(), TII, State, 0,
                   PreserveScratch, ScratchFIs);
        Modified = true;
      }
    }
    if (!PreserveScratch)
      for (MachineLoop *Loop : MLI)
        Modified |= reuseTransitionMasks(MF, *Loop, TII);
    return Modified;
  }
};

class SHFPUStateLegacy : public MachineFunctionPass {
  bool PreserveScratch;

public:
  static char ID;
  explicit SHFPUStateLegacy(bool PreserveScratch = false)
      : MachineFunctionPass(ID), PreserveScratch(PreserveScratch) {}
  StringRef getPassName() const override { return "SH FPU State"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineLoopInfoWrapperPass>();
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }
  bool runOnMachineFunction(MachineFunction &MF) override {
    FPUStateImpl Impl(PreserveScratch);
    return Impl.run(MF, getAnalysis<MachineLoopInfoWrapperPass>().getLI());
  }
};

char SHFPUStateLegacy::ID = 0;

} // namespace

INITIALIZE_PASS_BEGIN(SHFPUStateLegacy, DEBUG_TYPE,
                      "Establish SH FPU execution modes", false, false)
INITIALIZE_PASS_DEPENDENCY(MachineLoopInfoWrapperPass)
INITIALIZE_PASS_END(SHFPUStateLegacy, DEBUG_TYPE,
                    "Establish SH FPU execution modes", false, false)

FunctionPass *llvm::createSHFPUStateLegacyPass(bool PreserveScratch) {
  return new SHFPUStateLegacy(PreserveScratch);
}

PreservedAnalyses SHFPUStatePass::run(MachineFunction &MF,
                                      MachineFunctionAnalysisManager &MFAM) {
  FPUStateImpl Impl(PreserveScratch);
  MachineLoopInfo &MLI = MFAM.getResult<MachineLoopAnalysis>(MF);
  return Impl.run(MF, MLI) ? getMachineFunctionPassPreservedAnalyses()
                                 .preserveSet<CFGAnalyses>()
                           : PreservedAnalyses::all();
}
