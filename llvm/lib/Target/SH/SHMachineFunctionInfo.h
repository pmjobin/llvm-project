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

public:
  SHMachineFunctionInfo(const Function &, const TargetSubtargetInfo *) {}

  Register getSRetReturnReg() const { return SRetReturnReg; }
  void setSRetReturnReg(Register Reg) { SRetReturnReg = Reg; }
};

} // namespace llvm

#endif
