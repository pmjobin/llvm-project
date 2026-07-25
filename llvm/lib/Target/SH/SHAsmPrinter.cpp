//===-- SHAsmPrinter.cpp - SH assembly printer ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHAsmPrinter.h"
#include "MCTargetDesc/SHMCTargetDesc.h"
#include "SH.h"
#include "SHMCInstLower.h"
#include "TargetInfo/SHTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/AsmPrinterAnalysis.h"
#include "llvm/CodeGen/MachineFunctionAnalysisManager.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachinePassManager.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {

class SHAsmPrinter : public AsmPrinter {
public:
  static char ID;
  SHAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}
  StringRef getPassName() const override { return "SH Assembly Printer"; }

  void emitInstruction(const MachineInstr *MI) override {
    SHMCInstLower Lowering(OutContext);
    MachineBasicBlock::const_instr_iterator I = MI->getIterator();
    MachineBasicBlock::const_instr_iterator E = MI->getParent()->instr_end();
    do {
      MCInst OutMI;
      Lowering.lower(&*I, OutMI);
      EmitToStreamer(*OutStreamer, OutMI);
    } while (++I != E && I->isInsideBundle());
  }
};

char SHAsmPrinter::ID = 0;

} // namespace

INITIALIZE_PASS(SHAsmPrinter, "sh-asm-printer", "SH Assembly Printer", false,
                false)

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeSHAsmPrinter() {
  RegisterAsmPrinter<SHAsmPrinter> X(getTheSHTarget());
  RegisterAsmPrinter<SHAsmPrinter> Y(getTheSHLETarget());
}

PreservedAnalyses SHAsmPrinterBeginPass::run(Module &M,
                                             ModuleAnalysisManager &MAM) {
  SHAsmPrinter &Printer = static_cast<SHAsmPrinter &>(
      MAM.getResult<AsmPrinterAnalysis>(M).getPrinter());
  setupModuleAsmPrinter(M, MAM, Printer);
  Printer.doInitialization(M);
  return PreservedAnalyses::all();
}

PreservedAnalyses SHAsmPrinterPass::run(MachineFunction &MF,
                                        MachineFunctionAnalysisManager &MFAM) {
  SHAsmPrinter &Printer = static_cast<SHAsmPrinter &>(
      MFAM.getResult<ModuleAnalysisManagerMachineFunctionProxy>(MF)
          .getCachedResult<AsmPrinterAnalysis>(*MF.getFunction().getParent())
          ->getPrinter());
  setupMachineFunctionAsmPrinter(MFAM, MF, Printer);
  Printer.runOnMachineFunction(MF);
  return PreservedAnalyses::all();
}

PreservedAnalyses SHAsmPrinterEndPass::run(Module &M,
                                           ModuleAnalysisManager &MAM) {
  SHAsmPrinter &Printer = static_cast<SHAsmPrinter &>(
      MAM.getResult<AsmPrinterAnalysis>(M).getPrinter());
  setupModuleAsmPrinter(M, MAM, Printer);
  Printer.doFinalization(M);
  return PreservedAnalyses::all();
}
