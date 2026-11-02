//===-- SH.h - Top-level interface for SH code generation ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SH_H
#define LLVM_LIB_TARGET_SH_SH_H

#include "llvm/CodeGen/MachineFunctionAnalysisManager.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include <cstdint>

namespace llvm {

inline constexpr uint64_t SHMaxStackFrameSize = 0xfffffffcULL;
inline constexpr uint64_t SHMaxCallFrameSize = 0xfffffffcULL;
inline constexpr uint64_t SHMaxStackAlignment = 0x40000000ULL;
inline constexpr int64_t SHMinFrameAddressOffset =
    -static_cast<int64_t>(SHMaxCallFrameSize);
inline constexpr int64_t SHMaxFrameAddressOffset =
    static_cast<int64_t>(SHMaxCallFrameSize);

class FunctionPass;
class MachineFunction;
class MachineInstr;
class MachineBasicBlock;
class PassRegistry;
class SHTargetMachine;

int createSHPRSpillFrameIndex(MachineFunction &MF);
MachineBasicBlock *emitSHVariableI64Shift(MachineInstr &MI,
                                          MachineBasicBlock *EntryMBB);

class SHDAGToDAGISelPass : public SelectionDAGISelPass {
public:
  explicit SHDAGToDAGISelPass(SHTargetMachine &TM);
};

FunctionPass *createSHISelDagLegacyPass(SHTargetMachine &TM);

class SHPrepareIRPass : public RequiredPassInfoMixin<SHPrepareIRPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

class SHPrepareIRModulePass
    : public RequiredPassInfoMixin<SHPrepareIRModulePass> {
  SHTargetMachine &TM;

public:
  explicit SHPrepareIRModulePass(SHTargetMachine &TM) : TM(TM) {}
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};

FunctionPass *createSHPrepareIRLegacyPass();

class SHPostRAPeepholePass
    : public OptionalPassInfoMixin<SHPostRAPeepholePass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createSHPostRAPeepholeLegacyPass();

class SHInlineJumpTablesPass
    : public RequiredPassInfoMixin<SHInlineJumpTablesPass> {
public:
  MachineFunctionProperties getRequiredProperties() const {
    return MachineFunctionProperties().setNoVRegs();
  }
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createSHInlineJumpTablesLegacyPass();

class SHCompressJumpTablesPass
    : public OptionalPassInfoMixin<SHCompressJumpTablesPass> {
public:
  MachineFunctionProperties getRequiredProperties() const {
    return MachineFunctionProperties().setNoVRegs().setTracksLiveness();
  }
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createSHCompressJumpTablesLegacyPass();

class SHAtomicValidatePass
    : public RequiredPassInfoMixin<SHAtomicValidatePass> {
  SHTargetMachine &TM;

public:
  explicit SHAtomicValidatePass(SHTargetMachine &TM) : TM(TM) {}
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

FunctionPass *createSHAtomicValidateLegacyPass(SHTargetMachine &TM);
ModulePass *createSHIRValidationLegacyPass(SHTargetMachine &TM);

class SHDelaySlotFillerPass
    : public RequiredPassInfoMixin<SHDelaySlotFillerPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createSHDelaySlotFillerLegacyPass();

class SHFPUStatePass : public RequiredPassInfoMixin<SHFPUStatePass> {
  bool PreserveScratch;

public:
  explicit SHFPUStatePass(bool PreserveScratch = false)
      : PreserveScratch(PreserveScratch) {}
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createSHFPUStateLegacyPass(bool PreserveScratch = false);

class SHLiteralIslandPass : public RequiredPassInfoMixin<SHLiteralIslandPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createSHLiteralIslandLegacyPass();

class SHLiteralPoolRangeCheckPass
    : public OptionalPassInfoMixin<SHLiteralPoolRangeCheckPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createSHLiteralPoolRangeCheckLegacyPass();

void initializeSHAsmPrinterPass(PassRegistry &);
void initializeSHDAGToDAGISelLegacyPass(PassRegistry &);
void initializeSHAtomicValidateLegacyPass(PassRegistry &);
void initializeSHIRValidationLegacyPass(PassRegistry &);
void initializeSHPrepareIRLegacyPass(PassRegistry &);
void initializeSHPostRAPeepholeLegacyPass(PassRegistry &);
void initializeSHInlineJumpTablesLegacyPass(PassRegistry &);
void initializeSHCompressJumpTablesLegacyPass(PassRegistry &);
void initializeSHDelaySlotFillerLegacyPass(PassRegistry &);
void initializeSHFPUStateLegacyPass(PassRegistry &);
void initializeSHLiteralIslandLegacyPass(PassRegistry &);
void initializeSHLiteralPoolRangeCheckLegacyPass(PassRegistry &);

} // namespace llvm

#endif
