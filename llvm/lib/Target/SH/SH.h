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

class SHDAGToDAGISelPass : public SelectionDAGISelPass {
public:
  explicit SHDAGToDAGISelPass(SHTargetMachine &TM);
};

FunctionPass *createSHISelDagLegacyPass(SHTargetMachine &TM);

class SHDelaySlotFillerPass : public PassInfoMixin<SHDelaySlotFillerPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createSHDelaySlotFillerLegacyPass();

void initializeSHAsmPrinterPass(PassRegistry &);
void initializeSHDAGToDAGISelLegacyPass(PassRegistry &);
void initializeSHDelaySlotFillerLegacyPass(PassRegistry &);

} // namespace llvm

#endif
