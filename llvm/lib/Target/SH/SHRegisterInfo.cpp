//===-- SHRegisterInfo.cpp - SH register information ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHRegisterInfo.h"
#include "MCTargetDesc/SHMCTargetDesc.h"
#include "SHFrameLowering.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "SHGenRegisterInfo.inc"

SHRegisterInfo::SHRegisterInfo() : SHGenRegisterInfo(SH::PR) {}

const MCPhysReg *
SHRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_SH_SaveList;
}

const uint32_t *SHRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                                     CallingConv::ID CC) const {
  return CSR_SH_RegMask;
}

BitVector SHRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(SH::R15);
  Reserved.set(SH::PC);
  Reserved.set(SH::PR);
  Reserved.set(SH::GBR);
  Reserved.set(SH::VBR);
  Reserved.set(SH::MACH);
  Reserved.set(SH::MACL);
  Reserved.set(SH::SR);
  return Reserved;
}

const TargetRegisterClass *
SHRegisterInfo::getPointerRegClass(unsigned Kind) const {
  return &SH::GPRRegClass;
}

bool SHRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MI,
                                         int SPAdj, unsigned FIOperandNum,
                                         RegScavenger *RS) const {
  report_fatal_error("SH frame indices are not supported");
}

Register SHRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return SH::R15;
}
