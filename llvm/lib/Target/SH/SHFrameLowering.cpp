//===-- SHFrameLowering.cpp - SH frame lowering ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHFrameLowering.h"
#include "SHInstrInfo.h"
#include "SHSubtarget.h"
#include "llvm/ADT/BitVector.h"
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
  if (StackSize > 60)
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

void SHFrameLowering::emitPrologue(MachineFunction &MF,
                                   MachineBasicBlock &MBB) const {
  uint64_t StackSize = requireSupportedSHFrame(MF);
  if (StackSize == 0)
    return;

  const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
  MachineBasicBlock::iterator Insert = MBB.begin();
  if (MF.getFrameInfo().hasCalls()) {
    int FI = getPRSpillFrameIndex(MF);
    int64_t PROffset = MF.getFrameInfo().getObjectOffset(FI);
    if (PROffset > -4 || PROffset % 4 != 0 ||
        static_cast<uint64_t>(-PROffset) > StackSize)
      report_fatal_error("SH PR save area has an invalid frame offset");
    uint64_t BeforePR = static_cast<uint64_t>(-PROffset) - 4;
    if (BeforePR != 0) {
      MachineInstrBuilder Adjust =
          BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::ADDri), SH::R15)
              .addReg(SH::R15)
              .addImm(-static_cast<int64_t>(BeforePR))
              .setMIFlag(MachineInstr::FrameSetup);
      Insert = std::next(Adjust->getIterator());
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
    StackSize -= static_cast<uint64_t>(-PROffset);
  }
  if (StackSize != 0)
    BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::ADDri), SH::R15)
        .addReg(SH::R15)
        .addImm(-static_cast<int64_t>(StackSize))
        .setMIFlag(MachineInstr::FrameSetup);
}

void SHFrameLowering::emitEpilogue(MachineFunction &MF,
                                   MachineBasicBlock &MBB) const {
  uint64_t StackSize = requireSupportedSHFrame(MF);
  if (StackSize == 0)
    return;

  MachineBasicBlock::iterator Insert = MBB.getFirstTerminator();
  const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
  uint64_t BeforePR = 0;
  if (MF.getFrameInfo().hasCalls()) {
    int FI = getPRSpillFrameIndex(MF);
    int64_t PROffset = MF.getFrameInfo().getObjectOffset(FI);
    if (PROffset > -4 || PROffset % 4 != 0 ||
        static_cast<uint64_t>(-PROffset) > StackSize)
      report_fatal_error("SH PR restore area has an invalid frame offset");
    BeforePR = static_cast<uint64_t>(-PROffset) - 4;
    StackSize -= static_cast<uint64_t>(-PROffset);
  }
  if (StackSize != 0)
    BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::ADDri), SH::R15)
        .addReg(SH::R15)
        .addImm(StackSize)
        .setMIFlag(MachineInstr::FrameDestroy);
  if (MF.getFrameInfo().hasCalls()) {
    int FI = getPRSpillFrameIndex(MF);
    MachineMemOperand *MMO =
        MF.getMachineMemOperand(MachinePointerInfo::getFixedStack(MF, FI),
                                MachineMemOperand::MOLoad, 4, Align(4));
    BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::LDS_L_PR), SH::R15)
        .addReg(SH::R15)
        .addMemOperand(MMO)
        .setMIFlag(MachineInstr::FrameDestroy);
    if (BeforePR != 0)
      BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::ADDri), SH::R15)
          .addReg(SH::R15)
          .addImm(BeforePR)
          .setMIFlag(MachineInstr::FrameDestroy);
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
  int64_t LowestFixedOffset = 0;
  for (int FI = MFI.getObjectIndexBegin(); FI != 0; ++FI)
    LowestFixedOffset = std::min(LowestFixedOffset, MFI.getObjectOffset(FI));
  MFI.CreateFixedSpillStackObject(4, LowestFixedOffset - 4, true);
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
  }
  return MBB.erase(I);
}

bool SHFrameLowering::hasFPImpl(const MachineFunction &MF) const {
  return false;
}
