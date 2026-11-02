//===-- SHCodeGenPassBuilder.cpp - SH new-PM CodeGen pipeline -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SH.h"
#include "SHAsmPrinter.h"
#include "SHTargetMachine.h"
#include "llvm/CodeGen/AtomicExpand.h"
#include "llvm/CodeGen/BranchRelaxation.h"
#include "llvm/CodeGen/MachineCopyPropagation.h"
#include "llvm/IR/PassInstrumentation.h"
#include "llvm/IR/PassManager.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Passes/CodeGenPassBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Target/CGPassBuilderOption.h"

using namespace llvm;

namespace {

class SHCodeGenPassBuilder : public CodeGenPassBuilder {
  using Base = CodeGenPassBuilder;

  SHTargetMachine &getTM() const { return static_cast<SHTargetMachine &>(TM); }

public:
  explicit SHCodeGenPassBuilder(SHTargetMachine &TM,
                                const CGPassBuilderOption &Opts,
                                PassInstrumentationCallbacks *PIC)
      : Base(TM, Opts, PIC) {}
  void addIRPasses(PassManagerWrapper &PMW) override {
    // Reject unsupported atomic forms before generic expansion creates runtime
    // calls. Their desired values use ordinary SelectionDAG legalization.
    addFunctionPass(SHAtomicValidatePass(getTM()), PMW);
    addFunctionPass(AtomicExpandPass(TM), PMW);
    flushFPMsToMPM(PMW);
    addModulePass(SHPrepareIRModulePass(getTM()), PMW);
    Base::addIRPasses(PMW);
  }
  Error addInstSelector(PassManagerWrapper &PMW) override {
    addMachineFunctionPass(SHDAGToDAGISelPass(getTM()), PMW);
    return Error::success();
  }
  void addPreRegAlloc(PassManagerWrapper &PMW) override {
    addMachineFunctionPass(SHFPUStatePass(), PMW);
  }
  void addPostRegAlloc(PassManagerWrapper &PMW) override {
    addMachineFunctionPass(SHFPUStatePass(/*PreserveScratch=*/true), PMW);
    addMachineFunctionPass(SHPostRAPeepholePass(), PMW);
  }
  void addPreEmitPass(PassManagerWrapper &PMW) override {
    // Fold PEI adjustments and late allocated-register patterns.
    addMachineFunctionPass(SHPostRAPeepholePass(), PMW);
    if (getOptLevel() != CodeGenOptLevel::None)
      addMachineFunctionPass(MachineCopyPropagationPass(/*UseCopyInstr=*/true),
                             PMW);
    // Inline 32-bit tables make branch relaxation conservative. Compression
    // and delay filling must preserve the relaxed branch ranges, including
    // changes in alignment padding.
    addMachineFunctionPass(SHInlineJumpTablesPass(), PMW);
    addMachineFunctionPass(SHLiteralIslandPass(), PMW);
    addMachineFunctionPass(BranchRelaxationPass(), PMW);
    addMachineFunctionPass(SHCompressJumpTablesPass(), PMW);
    addMachineFunctionPass(SHDelaySlotFillerPass(), PMW);
    addMachineFunctionPass(SHLiteralPoolRangeCheckPass(), PMW);
  }
  void addAsmPrinterBegin(PassManagerWrapper &PMW) override {
    addModulePass(SHAsmPrinterBeginPass(), PMW, true);
  }
  void addAsmPrinter(PassManagerWrapper &PMW) override {
    addMachineFunctionPass(SHAsmPrinterPass(), PMW);
  }
  void addAsmPrinterEnd(PassManagerWrapper &PMW) override {
    addModulePass(SHAsmPrinterEndPass(), PMW, true);
  }
};

} // namespace

void SHTargetMachine::registerPassBuilderCallbacks(PassBuilder &PB) {
#define GET_PASS_REGISTRY "SHPassRegistry.def"
#include "llvm/Passes/TargetPassRegistry.inc"
  if (PIC) {
    PIC->addClassToPassName(SHAsmPrinterBeginPass::name(),
                            "sh-asm-printer-begin");
    PIC->addClassToPassName(SHAsmPrinterPass::name(), "sh-asm-printer");
    PIC->addClassToPassName(SHAsmPrinterEndPass::name(), "sh-asm-printer-end");
    PIC->addClassToPassName(SHAtomicValidatePass::name(), "sh-atomic-validate");
    PIC->addClassToPassName(SHPrepareIRPass::name(), "sh-prepare-ir");
    PIC->addClassToPassName(SHPrepareIRModulePass::name(),
                            "sh-prepare-ir-module");
    PIC->addClassToPassName(SHPostRAPeepholePass::name(), "sh-postra-peephole");
    PIC->addClassToPassName(SHInlineJumpTablesPass::name(),
                            "sh-inline-jump-tables");
    PIC->addClassToPassName(SHCompressJumpTablesPass::name(),
                            "sh-compress-jump-tables");
    PIC->addClassToPassName(SHFPUStatePass::name(), "sh-fpu-state");
    PIC->addClassToPassName(SHLiteralIslandPass::name(), "sh-literal-islands");
    PIC->addClassToPassName(SHLiteralPoolRangeCheckPass::name(),
                            "sh-literal-pool-range-check");
  }
}

Error SHTargetMachine::buildCodeGenPipeline(
    ModulePassManager &MPM, ModuleAnalysisManager &MAM, raw_pwrite_stream &Out,
    raw_pwrite_stream *DwoOut, CodeGenFileType FileType,
    const CGPassBuilderOption &Opt, MCContext &Ctx,
    PassInstrumentationCallbacks *PIC) {
  SHCodeGenPassBuilder CGPB(*this, Opt, PIC);
  return CGPB.buildPipeline(MPM, MAM, Out, DwoOut, FileType, Ctx);
}
