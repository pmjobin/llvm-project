//===-- SHSubtarget.h - SH subtarget information ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHSUBTARGET_H
#define LLVM_LIB_TARGET_SH_SHSUBTARGET_H

#include "SHFrameLowering.h"
#include "SHISelLowering.h"
#include "SHInstrInfo.h"
#include "SHSelectionDAGInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "SHGenSubtargetInfo.inc"

namespace llvm {

class SHSubtarget : public SHGenSubtargetInfo {
  SHInstrInfo InstrInfo;
  SHFrameLowering FrameLowering;
  SHTargetLowering TLInfo;
  SHSelectionDAGInfo TSInfo;

  SHSubtarget &initializeSubtargetDependencies(StringRef CPU, StringRef FS);

public:
  SHSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
              const TargetMachine &TM);

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);
  const SHInstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const SHRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
  const SHFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const SHTargetLowering *getTargetLowering() const override { return &TLInfo; }
  const SHSelectionDAGInfo *getSelectionDAGInfo() const override {
    return &TSInfo;
  }
};

} // namespace llvm

#endif
