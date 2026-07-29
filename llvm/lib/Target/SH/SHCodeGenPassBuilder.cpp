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
#include "llvm/CodeGen/BranchRelaxation.h"
#include "llvm/IR/PassInstrumentation.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Passes/CodeGenPassBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Target/CGPassBuilderOption.h"

using namespace llvm;

namespace {

class SHCodeGenPassBuilder
    : public CodeGenPassBuilder<SHCodeGenPassBuilder, SHTargetMachine> {
  using Base = CodeGenPassBuilder<SHCodeGenPassBuilder, SHTargetMachine>;

public:
  SHCodeGenPassBuilder(SHTargetMachine &TM, const CGPassBuilderOption &Opts,
                       PassInstrumentationCallbacks *PIC)
      : Base(TM, Opts, PIC) {}
  void addIRPasses(PassManagerWrapper &PMW) const {
    addFunctionPass(SHLowerI64StackAlignPass(), PMW);
    Base::addIRPasses(PMW);
  }
  Error addInstSelector(PassManagerWrapper &PMW) const {
    addMachineFunctionPass(SHDAGToDAGISelPass(TM), PMW);
    return Error::success();
  }
  void addPreEmitPass(PassManagerWrapper &PMW) const {
    addMachineFunctionPass(BranchRelaxationPass(), PMW);
    addMachineFunctionPass(SHDelaySlotFillerPass(), PMW);
    addMachineFunctionPass(SHLiteralPoolRangeCheckPass(), PMW);
  }
  void addAsmPrinterBegin(PassManagerWrapper &PMW) const {
    addModulePass(SHAsmPrinterBeginPass(), PMW, true);
  }
  void addAsmPrinter(PassManagerWrapper &PMW) const {
    addMachineFunctionPass(SHAsmPrinterPass(), PMW);
  }
  void addAsmPrinterEnd(PassManagerWrapper &PMW) const {
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
    PIC->addClassToPassName(SHLowerI64StackAlignPass::name(),
                            "sh-lower-i64-stack-align");
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
