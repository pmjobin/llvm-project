//===-- SHFrameLowering.cpp - SH frame lowering ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHFrameLowering.h"
#include "SH.h"
#include "SHInstrInfo.h"
#include "SHMachineFunctionInfo.h"
#include "SHSubtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/CFIInstBuilder.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

SHFrameLowering::SHFrameLowering()
    : TargetFrameLowering(StackGrowsDown, Align(4), 0, Align(4), false) {}

static uint64_t requireSupportedSHFrame(const MachineFunction &MF) {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.hasVarSizedObjects())
    report_fatal_error("SH dynamic stack frames are not supported");
  if (MFI.getMaxAlign() > Align(4))
    report_fatal_error("SH stack object alignment cannot exceed 4 bytes");

  uint64_t StackSize = MFI.getStackSize();
  bool IsExtendedAtomicCASFrame = false;
  if (StackSize > 60 && StackSize <= 124)
    for (const BasicBlock &BB : MF.getFunction())
      for (const Instruction &I : BB)
        if (const auto *Call = dyn_cast<CallBase>(&I);
            Call && isSHAtomicCompareExchangeCall(*Call))
          IsExtendedAtomicCASFrame = true;
  if (StackSize > 60 && !IsExtendedAtomicCASFrame)
    report_fatal_error("SH stack frame size cannot exceed 60 bytes");
  if (StackSize % 4 != 0)
    report_fatal_error("SH stack frame size must be four-byte aligned");
  return StackSize;
}

static int getPRSpillFrameIndex(const MachineFunction &MF) {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  for (int FI = MFI.getObjectIndexBegin(); FI != 0; ++FI)
    if (MFI.isSpillSlotObjectIndex(FI) && MFI.getObjectSize(FI) == 4)
      return FI;
  report_fatal_error("SH non-leaf function is missing its PR save area");
}

static void emitCalleeSavedSpillCFI(MachineFunction &MF,
                                    MachineBasicBlock &MBB) {
  if (!MF.needsFrameMoves())
    return;

  const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  for (const CalleeSavedInfo &Info : MFI.getCalleeSavedInfo()) {
    bool Found = false;
    for (MachineInstr &MI : MBB) {
      int FrameIndex;
      if (TII->isStoreToStackSlot(MI, FrameIndex) != Info.getReg() ||
          FrameIndex != Info.getFrameIdx())
        continue;
      CFIInstBuilder(MBB, std::next(MI.getIterator()), MachineInstr::FrameSetup)
          .buildOffset(Info.getReg(), MFI.getObjectOffset(FrameIndex));
      Found = true;
      break;
    }
    if (!Found)
      report_fatal_error("SH callee-saved spill has no frame-setup store");
  }
}

static void emitCalleeSavedRestoreCFI(MachineFunction &MF,
                                      MachineBasicBlock &MBB) {
  if (!MF.needsFrameMoves())
    return;

  const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  for (const CalleeSavedInfo &Info : MFI.getCalleeSavedInfo()) {
    bool Found = false;
    for (MachineInstr &MI : MBB) {
      int FrameIndex;
      if (TII->isLoadFromStackSlot(MI, FrameIndex) != Info.getReg() ||
          FrameIndex != Info.getFrameIdx())
        continue;
      CFIInstBuilder(MBB, std::next(MI.getIterator()),
                     MachineInstr::FrameDestroy)
          .buildRestore(Info.getReg());
      Found = true;
      break;
    }
    if (!Found)
      report_fatal_error("SH callee-saved restore has no frame-destroy load");
  }
}

void SHFrameLowering::emitPrologue(MachineFunction &MF,
                                   MachineBasicBlock &MBB) const {
  uint64_t StackSize = requireSupportedSHFrame(MF);
  if (StackSize == 0)
    return;

  const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
  MachineBasicBlock::iterator Insert = MBB.begin();
  uint64_t CFAOffset = 0;
  if (MF.getFrameInfo().hasCalls()) {
    int FI = getPRSpillFrameIndex(MF);
    int64_t PROffset = MF.getFrameInfo().getObjectOffset(FI);
    unsigned VarArgsSaveSize =
        MF.getInfo<SHMachineFunctionInfo>()->getVarArgsSaveSize();
    bool IsValidOffset =
        MF.getFunction().isVarArg()
            ? PROffset == -static_cast<int64_t>(VarArgsSaveSize + 4)
            : PROffset <= -4 && PROffset % 4 == 0;
    if (!IsValidOffset || static_cast<uint64_t>(-PROffset) > StackSize)
      report_fatal_error("SH PR save area has an invalid frame offset");
    uint64_t BeforePR = static_cast<uint64_t>(-PROffset) - 4;
    if (BeforePR != 0) {
      MachineInstrBuilder Adjust =
          BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::ADDri), SH::R15)
              .addReg(SH::R15)
              .addImm(-static_cast<int64_t>(BeforePR))
              .setMIFlag(MachineInstr::FrameSetup);
      Insert = std::next(Adjust->getIterator());
      CFAOffset += BeforePR;
      if (MF.needsFrameMoves())
        CFIInstBuilder(MBB, Insert, MachineInstr::FrameSetup)
            .buildDefCFAOffset(CFAOffset);
    }
    MachineMemOperand *MMO =
        MF.getMachineMemOperand(MachinePointerInfo::getFixedStack(MF, FI),
                                MachineMemOperand::MOStore, 4, Align(4));
    MachineInstrBuilder Save =
        BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::STS_L_PR), SH::R15)
            .addReg(SH::R15)
            .addMemOperand(MMO)
            .setMIFlag(MachineInstr::FrameSetup);
    Insert = std::next(Save->getIterator());
    CFAOffset += 4;
    if (MF.needsFrameMoves()) {
      CFIInstBuilder CFIBuilder(MBB, Insert, MachineInstr::FrameSetup);
      CFIBuilder.buildDefCFAOffset(CFAOffset);
      CFIBuilder.buildOffset(SH::PR, PROffset);
    }
    StackSize -= static_cast<uint64_t>(-PROffset);
  }
  if (StackSize != 0) {
    MachineInstrBuilder Adjust =
        BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::ADDri), SH::R15)
            .addReg(SH::R15)
            .addImm(-static_cast<int64_t>(StackSize))
            .setMIFlag(MachineInstr::FrameSetup);
    Insert = std::next(Adjust->getIterator());
    CFAOffset += StackSize;
    if (MF.needsFrameMoves())
      CFIInstBuilder(MBB, Insert, MachineInstr::FrameSetup)
          .buildDefCFAOffset(CFAOffset);
  }
  emitCalleeSavedSpillCFI(MF, MBB);
}

void SHFrameLowering::emitEpilogue(MachineFunction &MF,
                                   MachineBasicBlock &MBB) const {
  uint64_t StackSize = requireSupportedSHFrame(MF);
  if (StackSize == 0)
    return;

  MachineBasicBlock::iterator Insert = MBB.getFirstTerminator();
  const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
  emitCalleeSavedRestoreCFI(MF, MBB);
  uint64_t BeforePR = 0;
  if (MF.getFrameInfo().hasCalls()) {
    int FI = getPRSpillFrameIndex(MF);
    int64_t PROffset = MF.getFrameInfo().getObjectOffset(FI);
    unsigned VarArgsSaveSize =
        MF.getInfo<SHMachineFunctionInfo>()->getVarArgsSaveSize();
    bool IsValidOffset =
        MF.getFunction().isVarArg()
            ? PROffset == -static_cast<int64_t>(VarArgsSaveSize + 4)
            : PROffset <= -4 && PROffset % 4 == 0;
    if (!IsValidOffset || static_cast<uint64_t>(-PROffset) > StackSize)
      report_fatal_error("SH PR restore area has an invalid frame offset");
    BeforePR = static_cast<uint64_t>(-PROffset) - 4;
    StackSize -= static_cast<uint64_t>(-PROffset);
  }
  if (StackSize != 0) {
    BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::ADDri), SH::R15)
        .addReg(SH::R15)
        .addImm(StackSize)
        .setMIFlag(MachineInstr::FrameDestroy);
    if (MF.needsFrameMoves())
      CFIInstBuilder(MBB, Insert, MachineInstr::FrameDestroy)
          .buildDefCFAOffset(MF.getFrameInfo().hasCalls() ? BeforePR + 4 : 0);
  }
  if (MF.getFrameInfo().hasCalls()) {
    int FI = getPRSpillFrameIndex(MF);
    MachineMemOperand *MMO =
        MF.getMachineMemOperand(MachinePointerInfo::getFixedStack(MF, FI),
                                MachineMemOperand::MOLoad, 4, Align(4));
    BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::LDS_L_PR), SH::R15)
        .addReg(SH::R15)
        .addMemOperand(MMO)
        .setMIFlag(MachineInstr::FrameDestroy);
    if (MF.needsFrameMoves()) {
      CFIInstBuilder CFIBuilder(MBB, Insert, MachineInstr::FrameDestroy);
      CFIBuilder.buildRestore(SH::PR);
      CFIBuilder.buildDefCFAOffset(BeforePR);
    }
    if (BeforePR != 0) {
      BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::ADDri), SH::R15)
          .addReg(SH::R15)
          .addImm(BeforePR)
          .setMIFlag(MachineInstr::FrameDestroy);
      if (MF.needsFrameMoves())
        CFIInstBuilder(MBB, Insert, MachineInstr::FrameDestroy)
            .buildDefCFAOffset(0);
    }
  }
}

void SHFrameLowering::determineCalleeSaves(MachineFunction &MF,
                                           BitVector &SavedRegs,
                                           RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);
  if (MF.getFrameInfo().hasCalls())
    SavedRegs.set(SH::PR);
}

bool SHFrameLowering::assignCalleeSavedSpillSlots(
    MachineFunction &MF, const TargetRegisterInfo *TRI,
    std::vector<CalleeSavedInfo> &CSI) const {
  if (!MF.getFrameInfo().hasCalls())
    return false;

  MachineFrameInfo &MFI = MF.getFrameInfo();
  unsigned VarArgsSaveSize =
      MF.getInfo<SHMachineFunctionInfo>()->getVarArgsSaveSize();
  int64_t PROffset = -static_cast<int64_t>(VarArgsSaveSize + 4);
  // Nonvariadic byval formals may still use a fixed register/stack bridge.
  if (!MF.getFunction().isVarArg())
    for (int FI = MFI.getObjectIndexBegin(); FI != 0; ++FI)
      PROffset = std::min(PROffset, MFI.getObjectOffset(FI) - 4);
  MFI.CreateFixedSpillStackObject(4, PROffset, true);
  for (auto I = CSI.begin(); I != CSI.end(); ++I) {
    if (I->getReg() != SH::PR)
      continue;
    CSI.erase(I);
    return false;
  }
  report_fatal_error("SH non-leaf function did not reserve PR");
}

MachineBasicBlock::iterator SHFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  if (I->getOpcode() != SH::ADJCALLSTACKDOWN &&
      I->getOpcode() != SH::ADJCALLSTACKUP)
    report_fatal_error("SH encountered an unknown call-frame pseudo");

  int64_t Amount = I->getOperand(0).getImm();
  int64_t CalleePopAmount = I->getOperand(1).getImm();
  if (CalleePopAmount != 0)
    report_fatal_error("SH callee-popped call frames are not supported");
  if (Amount < 0 || Amount > 60 || Amount % 4 != 0)
    report_fatal_error(
        "SH call-frame adjustment must be four-byte aligned and in [0, 60]");

  if (Amount != 0) {
    const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
    int64_t Immediate =
        I->getOpcode() == SH::ADJCALLSTACKDOWN ? -Amount : Amount;
    BuildMI(MBB, I, I->getDebugLoc(), TII->get(SH::ADDri), SH::R15)
        .addReg(SH::R15)
        .addImm(Immediate)
        .setMIFlags(I->getFlags());
    if (MF.needsFrameMoves()) {
      int64_t CFAAdjustment =
          I->getOpcode() == SH::ADJCALLSTACKDOWN ? Amount : -Amount;
      CFIInstBuilder(MBB, I, MachineInstr::NoFlags)
          .buildAdjustCFAOffset(CFAAdjustment);
    }
  }
  return MBB.erase(I);
}

void SHFrameLowering::resetCFIToInitialState(MachineBasicBlock &MBB) const {
  MachineFunction &MF = *MBB.getParent();
  CFIInstBuilder CFIBuilder(MBB, MBB.begin(), MachineInstr::NoFlags);
  CFIBuilder.buildDefCFA(SH::R15, 0);
  if (MF.getFrameInfo().hasCalls())
    CFIBuilder.buildSameValue(SH::PR);
  for (const CalleeSavedInfo &Info : MF.getFrameInfo().getCalleeSavedInfo())
    CFIBuilder.buildSameValue(Info.getReg());
}

bool SHFrameLowering::hasFPImpl(const MachineFunction &MF) const {
  return false;
}
