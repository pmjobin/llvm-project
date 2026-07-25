//===-- SHInstrInfo.cpp - SH instruction information ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHInstrInfo.h"
#include "SHSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "SHGenInstrInfo.inc"

SHInstrInfo::SHInstrInfo(const SHSubtarget &STI)
    : SHGenInstrInfo(STI, RI), RI() {}

void SHInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MI,
                              const DebugLoc &DL, Register DestReg,
                              Register SrcReg, bool KillSrc, bool RenamableDest,
                              bool RenamableSrc) const {
  if (!SH::GPRRegClass.contains(DestReg, SrcReg))
    report_fatal_error("SH cannot copy the requested physical registers");
  BuildMI(MBB, MI, DL, get(SH::MOVrr), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
}

Register SHInstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
                                          int &FrameIndex) const {
  if (MI.getOpcode() == SH::MOVL_load_disp && MI.getOperand(1).isFI() &&
      MI.getOperand(2).isImm() && MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return MI.getOperand(0).getReg();
  }
  return Register();
}

Register SHInstrInfo::isStoreToStackSlot(const MachineInstr &MI,
                                         int &FrameIndex) const {
  if (MI.getOpcode() == SH::MOVL_store_disp && MI.getOperand(1).isFI() &&
      MI.getOperand(2).isImm() && MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return MI.getOperand(0).getReg();
  }
  return Register();
}

void SHInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
    bool IsKill, int FrameIndex, const TargetRegisterClass *RC, Register VReg,
    MachineInstr::MIFlag Flags) const {
  if (!SH::GPRRegClass.hasSubClassEq(RC))
    report_fatal_error("SH only supports 32-bit GPR spills");

  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.getObjectSize(FrameIndex) != 4 ||
      MFI.getObjectAlign(FrameIndex) < Align(4))
    report_fatal_error("SH GPR spill slots must be four-byte aligned words");

  MachineMemOperand *MMO =
      MF.getMachineMemOperand(MachinePointerInfo::getFixedStack(MF, FrameIndex),
                              MachineMemOperand::MOStore, 4, Align(4));
  BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(SH::MOVL_store_disp))
      .addReg(SrcReg, getKillRegState(IsKill))
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO)
      .setMIFlag(Flags);
}

void SHInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator MI,
                                       Register DestReg, int FrameIndex,
                                       const TargetRegisterClass *RC,
                                       Register VReg, unsigned SubReg,
                                       MachineInstr::MIFlag Flags) const {
  if (!SH::GPRRegClass.hasSubClassEq(RC) || SubReg != 0)
    report_fatal_error("SH only supports 32-bit GPR reloads");

  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.getObjectSize(FrameIndex) != 4 ||
      MFI.getObjectAlign(FrameIndex) < Align(4))
    report_fatal_error("SH GPR spill slots must be four-byte aligned words");

  MachineMemOperand *MMO =
      MF.getMachineMemOperand(MachinePointerInfo::getFixedStack(MF, FrameIndex),
                              MachineMemOperand::MOLoad, 4, Align(4));
  BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(SH::MOVL_load_disp), DestReg)
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO)
      .setMIFlag(Flags);
}
