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

namespace llvm {

class FunctionPass;
class PassRegistry;
class SHTargetMachine;

void validateSHIR(const Function &F);
void validateSHAtomics(const Function &F);
bool isSHAtomicCompareExchangeCall(const CallBase &Call);

class SHDAGToDAGISelPass : public SelectionDAGISelPass {
public:
  explicit SHDAGToDAGISelPass(SHTargetMachine &TM);
};

FunctionPass *createSHISelDagLegacyPass(SHTargetMachine &TM);

class SHLowerI64StackAlignPass
    : public PassInfoMixin<SHLowerI64StackAlignPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

FunctionPass *createSHLowerI64StackAlignLegacyPass();

class SHAtomicValidatePass : public PassInfoMixin<SHAtomicValidatePass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

FunctionPass *createSHAtomicValidateLegacyPass();

class SHDelaySlotFillerPass : public PassInfoMixin<SHDelaySlotFillerPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createSHDelaySlotFillerLegacyPass();

class SHLiteralIslandPass : public PassInfoMixin<SHLiteralIslandPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createSHLiteralIslandLegacyPass();

class SHLiteralPoolRangeCheckPass
    : public PassInfoMixin<SHLiteralPoolRangeCheckPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createSHLiteralPoolRangeCheckLegacyPass();

void initializeSHAsmPrinterPass(PassRegistry &);
void initializeSHDAGToDAGISelLegacyPass(PassRegistry &);
void initializeSHAtomicValidateLegacyPass(PassRegistry &);
void initializeSHLowerI64StackAlignLegacyPass(PassRegistry &);
void initializeSHDelaySlotFillerLegacyPass(PassRegistry &);
void initializeSHLiteralIslandLegacyPass(PassRegistry &);
void initializeSHLiteralPoolRangeCheckLegacyPass(PassRegistry &);

} // namespace llvm

#endif
