//===-- SHMachineFunctionInfo.h - SH machine function info ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHMACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_SH_SHMACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {

class SHMachineFunctionInfo : public MachineFunctionInfo {
  Register SRetReturnReg;
  int VarArgsFrameIndex = 0;
  unsigned VarArgsSaveSize = 0;
  bool HasVarArgsSaveArea = false;
  bool UsesPICBase = false;
  int PICBaseCPI = -1;

public:
  SHMachineFunctionInfo(const Function &, const TargetSubtargetInfo *) {}

  Register getSRetReturnReg() const { return SRetReturnReg; }
  void setSRetReturnReg(Register Reg) { SRetReturnReg = Reg; }

  bool hasVarArgsSaveArea() const { return HasVarArgsSaveArea; }
  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }
  unsigned getVarArgsSaveSize() const { return VarArgsSaveSize; }
  void setVarArgsSaveArea(int FrameIndex, unsigned SaveSize) {
    VarArgsFrameIndex = FrameIndex;
    VarArgsSaveSize = SaveSize;
    HasVarArgsSaveArea = true;
  }

  bool usesPICBase() const { return UsesPICBase; }
  int getPICBaseCPI() const { return PICBaseCPI; }
  void setPICBaseCPI(unsigned CPI) {
    PICBaseCPI = CPI;
    UsesPICBase = true;
  }
};

} // namespace llvm

#endif
