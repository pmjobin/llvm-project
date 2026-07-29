//===-- SHTargetMachine.cpp - SH target machine --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHTargetMachine.h"
#include "SH.h"
#include "SHMachineFunctionInfo.h"
#include "TargetInfo/SHTargetInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

static StringRef getSHCPU(StringRef CPU) { return CPU.empty() ? "sh2" : CPU; }

static Reloc::Model getSHRelocModel(std::optional<Reloc::Model> RM) {
  Reloc::Model Model = RM.value_or(Reloc::Static);
  if (Model != Reloc::Static)
    reportFatalUsageError("SH only supports static relocation");
  return Model;
}

static CodeModel::Model getSHCodeModel(std::optional<CodeModel::Model> CM) {
  CodeModel::Model Model = CM.value_or(CodeModel::Small);
  if (Model != CodeModel::Small && Model != CodeModel::Large)
    reportFatalUsageError("SH only supports the small and large code models");
  return Model;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeSHTarget() {
  RegisterTargetMachine<SHTargetMachine> X(getTheSHTarget());
  RegisterTargetMachine<SHTargetMachine> Y(getTheSHLETarget());
  PassRegistry &PR = *PassRegistry::getPassRegistry();
  initializeSHAsmPrinterPass(PR);
  initializeSHDAGToDAGISelLegacyPass(PR);
  initializeSHAtomicValidateLegacyPass(PR);
  initializeSHLowerI64StackAlignLegacyPass(PR);
  initializeSHDelaySlotFillerLegacyPass(PR);
  initializeSHLiteralIslandLegacyPass(PR);
  initializeSHLiteralPoolRangeCheckLegacyPass(PR);
}

SHTargetMachine::SHTargetMachine(const Target &T, const Triple &TT,
                                 StringRef CPU, StringRef FS,
                                 const TargetOptions &Options,
                                 std::optional<Reloc::Model> RM,
                                 std::optional<CodeModel::Model> CM,
                                 CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(T, TT.computeDataLayout(), TT, getSHCPU(CPU), FS,
                               Options, getSHRelocModel(RM), getSHCodeModel(CM),
                               OL),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()),
      Subtarget(TT, getSHCPU(CPU), FS, *this) {
  if (JIT)
    reportFatalUsageError("SH JIT code generation is not supported");
  if (!TT.isOSBinFormatELF())
    reportFatalUsageError("SH only supports the ELF object format");
  initAsmInfo();
}

namespace {

class SHPassConfig : public TargetPassConfig {
public:
  SHPassConfig(SHTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}
  SHTargetMachine &getSHTargetMachine() const {
    return getTM<SHTargetMachine>();
  }
  void addIRPasses() override {
    addPass(createSHAtomicValidateLegacyPass());
    addPass(createAtomicExpandLegacyPass());
    addPass(createSHLowerI64StackAlignLegacyPass());
    TargetPassConfig::addIRPasses();
  }
  bool addInstSelector() override {
    addPass(createSHISelDagLegacyPass(getSHTargetMachine()));
    return false;
  }
  void addPreEmitPass() override {
    addPass(createSHLiteralIslandLegacyPass());
    addPass(&BranchRelaxationPassID);
    addPass(createSHDelaySlotFillerLegacyPass());
    addPass(createSHLiteralPoolRangeCheckLegacyPass());
  }
};

} // namespace

TargetPassConfig *SHTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new SHPassConfig(*this, PM);
}

MachineFunctionInfo *SHTargetMachine::createMachineFunctionInfo(
    BumpPtrAllocator &Allocator, const Function &F,
    const TargetSubtargetInfo *STI) const {
  return SHMachineFunctionInfo::create<SHMachineFunctionInfo>(Allocator, F,
                                                              STI);
}
