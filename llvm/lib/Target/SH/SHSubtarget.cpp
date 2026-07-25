//===-- SHSubtarget.cpp - SH subtarget information -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHSubtarget.h"

using namespace llvm;

#define DEBUG_TYPE "sh-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "SHGenSubtargetInfo.inc"

SHSubtarget &SHSubtarget::initializeSubtargetDependencies(StringRef CPU,
                                                          StringRef FS) {
  if (CPU.empty())
    CPU = "sh2";
  ParseSubtargetFeatures(CPU, CPU, FS);
  return *this;
}

SHSubtarget::SHSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
                         const TargetMachine &TM)
    : SHGenSubtargetInfo(TT, CPU, CPU, FS),
      InstrInfo(initializeSubtargetDependencies(CPU, FS)), FrameLowering(),
      TLInfo(TM, *this) {}
