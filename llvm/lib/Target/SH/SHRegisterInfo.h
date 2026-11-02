//===-- SHRegisterInfo.h - SH register information -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHREGISTERINFO_H
#define LLVM_LIB_TARGET_SH_SHREGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "SHGenRegisterInfo.inc"

namespace llvm {

class SHRegisterInfo : public SHGenRegisterInfo {
public:
  SHRegisterInfo();

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;
  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID CC) const override;
  const uint32_t *getShiftLibcallPreservedMask(bool IsArithmetic) const;
  BitVector getReservedRegs(const MachineFunction &MF) const override;
  bool
  getRegAllocationHints(Register VirtReg, ArrayRef<MCPhysReg> Order,
                        SmallSetVector<MCPhysReg, 16> &Hints,
                        const MachineFunction &MF,
                        const VirtRegMap *VRM = nullptr,
                        const LiveRegMatrix *Matrix = nullptr) const override;
  bool reverseLocalAssignment() const override { return true; }
  bool eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;
  Register getFrameRegister(const MachineFunction &MF) const override;
  bool requiresRegisterScavenging(const MachineFunction &MF) const override {
    return true;
  }
};

} // namespace llvm

#endif
