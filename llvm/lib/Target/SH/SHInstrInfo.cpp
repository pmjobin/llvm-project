//===-- SHInstrInfo.cpp - SH instruction information ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHInstrInfo.h"
#include "SHSubtarget.h"
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

void SHInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
    bool IsKill, int FrameIndex, const TargetRegisterClass *RC, Register VReg,
    MachineInstr::MIFlag Flags) const {
  report_fatal_error("SH register spills are not supported");
}

void SHInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator MI,
                                       Register DestReg, int FrameIndex,
                                       const TargetRegisterClass *RC,
                                       Register VReg, unsigned SubReg,
                                       MachineInstr::MIFlag Flags) const {
  report_fatal_error("SH register reloads are not supported");
}
