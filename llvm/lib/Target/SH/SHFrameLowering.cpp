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

void SHFrameLowering::emitPrologue(MachineFunction &MF,
                                   MachineBasicBlock &MBB) const {
  uint64_t StackSize = requireSupportedSHFrame(MF);
  if (StackSize == 0)
    return;

  const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
  BuildMI(MBB, MBB.begin(), DebugLoc(), TII->get(SH::ADDri), SH::R15)
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
  BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::ADDri), SH::R15)
      .addReg(SH::R15)
      .addImm(StackSize)
      .setMIFlag(MachineInstr::FrameDestroy);
}

bool SHFrameLowering::hasFPImpl(const MachineFunction &MF) const {
  return false;
}
