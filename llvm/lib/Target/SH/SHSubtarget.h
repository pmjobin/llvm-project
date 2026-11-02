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
#include "llvm/TargetParser/SHTargetParser.h"

#define GET_SUBTARGETINFO_HEADER
#include "SHGenSubtargetInfo.inc"

namespace llvm {

class LibcallLoweringInfo;

class SHSubtarget : public SHGenSubtargetInfo {
  bool HasSH2 = false;
  bool HasSH2A = false;
  bool HasDynamicShift = false;
  bool HasSBitControl = false;
  bool HasExceptionState = false;
  bool HasBankedGPRs = false;
  bool HasMMU = false;
  bool HasPrefetch = false;
  bool HasSH4DebugState = false;
  bool HasSH4Cache = false;
  bool HasMOVAIllegalInDelaySlot = false;
  bool HasSH4ACommon = false;
  bool HasSH4A = false;
  bool HasDSP = false;
  bool HasSH4ALDSP = false;
  bool HasST40300 = false;
  bool HasSingleFPU = false;
  bool HasFSQRT = false;
  bool HasFPUSizeMode = false;
  bool HasDoubleFPU = false;
  bool HasFPURegisterBank = false;
  bool HasFPUVector = false;
  bool HasFPUSingleMode = false;
  SH::ABIKind ABI = SH::ABIKind::GNU;
  bool OptSize = false;

  SHInstrInfo InstrInfo;
  SHFrameLowering FrameLowering;
  SHTargetLowering TLInfo;
  SHSelectionDAGInfo TSInfo;

  SHSubtarget &initializeSubtargetDependencies(StringRef CPU, StringRef TuneCPU,
                                               StringRef FS);

public:
  SHSubtarget(const Triple &TT, StringRef CPU, StringRef TuneCPU, StringRef FS,
              SH::ABIKind ABI, bool OptSize, const TargetMachine &TM);

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);
  bool hasSH2() const { return HasSH2; }
  bool hasSH2A() const { return HasSH2A; }
  bool hasDynamicShift() const { return HasDynamicShift; }
  bool hasSBitControl() const { return HasSBitControl; }
  bool hasExceptionState() const { return HasExceptionState; }
  bool hasBankedGPRs() const { return HasBankedGPRs; }
  bool hasMMU() const { return HasMMU; }
  bool hasPrefetch() const { return HasPrefetch; }
  bool hasSH4DebugState() const { return HasSH4DebugState; }
  bool hasSH4Cache() const { return HasSH4Cache; }
  bool isMOVAIllegalInDelaySlot() const { return HasMOVAIllegalInDelaySlot; }
  bool hasSH4ACommon() const { return HasSH4ACommon; }
  bool hasSH4A() const { return HasSH4A; }
  bool hasDSP() const { return HasDSP; }
  bool hasSH4ALDSP() const { return HasSH4ALDSP; }
  bool hasST40300() const { return HasST40300; }
  bool hasSingleFPU() const { return HasSingleFPU; }
  bool hasFSQRT() const { return HasFSQRT; }
  bool hasFPUSizeMode() const { return HasFPUSizeMode; }
  bool hasDoubleFPU() const { return HasDoubleFPU; }
  bool hasFPURegisterBank() const { return HasFPURegisterBank; }
  bool hasFPUVector() const { return HasFPUVector; }
  bool hasFPUSingleMode() const { return HasFPUSingleMode; }
  SH::ABIKind getABI() const { return ABI; }
  bool isFDPIC() const { return SH::isFDPICABI(ABI); }
  bool useHardFloatABI() const { return SH::usesFPUArgumentABI(ABI); }
  bool usesSH4HardFloatABI() const { return SH::usesSH4HardFloatABI(ABI); }
  bool usesEFloatABI() const {
    return SH::getBaseABI(ABI) == SH::ABIKind::GNUEFloat;
  }
  // The SelectionDAG scheduler remains smaller for size-optimized functions.
  bool enableMachineScheduler() const override { return !OptSize; }
  // Coalesce cross-block copies even when the machine scheduler is disabled.
  bool enableJoinGlobalCopies() const override { return true; }
  bool enableMachineSchedDefaultSched() const override { return false; }
  bool enablePostRAMachineScheduler() const override { return false; }
  bool enablePostRAScheduler() const override { return !OptSize; }
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
  void initLibcallLoweringInfo(LibcallLoweringInfo &Info) const override;
};

} // namespace llvm

#endif
