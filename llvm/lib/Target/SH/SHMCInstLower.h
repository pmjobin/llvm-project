//===-- SHMCInstLower.h - Lower SH MachineInstr to MCInst -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHMCINSTLOWER_H
#define LLVM_LIB_TARGET_SH_SHMCINSTLOWER_H

namespace llvm {

class MCContext;
class MCInst;
class MachineInstr;

class SHMCInstLower {
  MCContext &Ctx;

public:
  explicit SHMCInstLower(MCContext &Ctx) : Ctx(Ctx) {}
  void lower(const MachineInstr *MI, MCInst &OutMI) const;
};

} // namespace llvm

#endif
