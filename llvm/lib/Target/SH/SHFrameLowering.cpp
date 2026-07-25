//===-- SHFrameLowering.cpp - SH frame lowering ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHFrameLowering.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

SHFrameLowering::SHFrameLowering()
    : TargetFrameLowering(StackGrowsDown, Align(4), 0, Align(4), false) {}

static void requireEmptySHFrame(const MachineFunction &MF) {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.getStackSize() != 0 || MFI.hasStackObjects() ||
      MFI.hasVarSizedObjects() || MFI.adjustsStack())
    report_fatal_error("SH stack frames are not supported");
}

void SHFrameLowering::emitPrologue(MachineFunction &MF,
                                   MachineBasicBlock &MBB) const {
  requireEmptySHFrame(MF);
}

void SHFrameLowering::emitEpilogue(MachineFunction &MF,
                                   MachineBasicBlock &MBB) const {
  requireEmptySHFrame(MF);
}

bool SHFrameLowering::hasFPImpl(const MachineFunction &MF) const {
  return false;
}
