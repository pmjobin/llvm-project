//===-- SHTargetMachine.cpp - SH target machine --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHTargetMachine.h"
#include "MCTargetDesc/SHMCAsmInfo.h"
#include "SH.h"
#include "SHMachineFunctionInfo.h"
#include "SHTargetTransformInfo.h"
#include "TargetInfo/SHTargetInfo.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/TargetParser/SHTargetParser.h"
#include "llvm/TargetParser/SubtargetFeature.h"
#include "llvm/Transforms/Utils/SplitModule.h"

using namespace llvm;

namespace {

class SHTargetObjectFile : public TargetLoweringObjectFileELF {
  bool IsFDPIC = false;

public:
  void initialize(MCContext &Ctx, const TargetMachine &TM,
                  const Module &M) override {
    if (M.getExceptionModel() != ExceptionHandling::None &&
        M.getExceptionModel() != ExceptionHandling::DwarfCFI)
      reportFatalUsageError("SH only supports DWARF CFI exception handling");
    TargetLoweringObjectFileELF::initialize(Ctx, TM, M);
    IsFDPIC = static_cast<const SHTargetMachine &>(TM).isFDPIC();
    FDECFIEncoding = dwarf::DW_EH_PE_pcrel | dwarf::DW_EH_PE_sdata4;
    if (TM.isPositionIndependent()) {
      PersonalityEncoding =
          dwarf::DW_EH_PE_indirect |
          (IsFDPIC ? dwarf::DW_EH_PE_datarel : dwarf::DW_EH_PE_pcrel);
      LSDAEncoding = dwarf::DW_EH_PE_pcrel | dwarf::DW_EH_PE_sdata4;
      TTypeEncoding =
          dwarf::DW_EH_PE_indirect |
          (IsFDPIC ? dwarf::DW_EH_PE_datarel : dwarf::DW_EH_PE_pcrel) |
          dwarf::DW_EH_PE_sdata4;
      return;
    }
    PersonalityEncoding = dwarf::DW_EH_PE_absptr;
    LSDAEncoding = dwarf::DW_EH_PE_sdata4;
    TTypeEncoding = dwarf::DW_EH_PE_sdata4;
  }

  MCSymbol *getCFIPersonalitySymbol(const GlobalValue *GV,
                                    const TargetMachine &TM,
                                    MachineModuleInfo *MMI) const override {
    if (IsFDPIC)
      return TM.getSymbol(GV);
    return TargetLoweringObjectFileELF::getCFIPersonalitySymbol(GV, TM, MMI);
  }

  void emitPersonalityValue(MCStreamer &Streamer, const DataLayout &DL,
                            const MCSymbol *Sym,
                            const MachineModuleInfo *MMI) const override {
    if (!IsFDPIC)
      TargetLoweringObjectFileELF::emitPersonalityValue(Streamer, DL, Sym, MMI);
  }

  const MCExpr *getTTypeGlobalReference(const GlobalValue *GV,
                                        unsigned Encoding,
                                        const TargetMachine &TM,
                                        MachineModuleInfo *MMI,
                                        MCStreamer &Streamer) const override {
    if (IsFDPIC)
      return MCSymbolRefExpr::create(TM.getSymbol(GV), SH::S_GOT, getContext());
    return TargetLoweringObjectFileELF::getTTypeGlobalReference(
        GV, Encoding, TM, MMI, Streamer);
  }

  const MCExpr *getDebugThreadLocalSymbol(const MCSymbol *Sym) const override {
    return MCSymbolRefExpr::create(Sym, SH::S_DTPOFF, getContext());
  }
};

} // namespace

static StringRef getSHCPU(StringRef CPU) {
  if (CPU.empty())
    return SH::DefaultCPU;
  if (!SH::isValidCPUName(CPU))
    reportFatalUsageError(Twine("SH does not support CPU '") + CPU + "'");
  return CPU;
}

static StringRef getSHTuneCPU(StringRef TuneCPU, StringRef CPU) {
  if (TuneCPU.empty())
    return CPU;
  SH::TuneCPUKind TuneCPUKind = SH::parseTuneCPU(TuneCPU);
  if (TuneCPUKind == SH::TuneCPUKind::Invalid)
    reportFatalUsageError(Twine("SH does not support tune CPU '") + TuneCPU +
                          "'");
  if (!SH::isTuneCompatible(SH::parseCPU(CPU), TuneCPUKind))
    reportFatalUsageError(Twine("SH tune CPU '") + TuneCPU +
                          "' is incompatible with architecture CPU '" + CPU +
                          "'");
  return TuneCPU;
}

static std::string getSHFeatureString(StringRef FS) {
  SubtargetFeatures Features(FS);
  SubtargetFeatures FilteredFeatures;
  static bool PrintedCPUHelp = false;
  for (const std::string &Feature : Features.getFeatures()) {
    if (Feature == "+cpuhelp") {
      if (!PrintedCPUHelp) {
        errs() << "Available CPUs for this target:\n\n";
        for (StringRef CPU : SH::ValidCPUNames)
          errs() << '\t' << CPU << '\n';
        errs() << "\nUse -mcpu to specify the target architecture.\n";
        PrintedCPUHelp = true;
      }
    } else
      FilteredFeatures.AddFeature(Feature);
  }
  return FilteredFeatures.getString();
}

static SH::ABIKind getSHABI(const Triple &TT, const TargetOptions &Options,
                            StringRef CPU, const Module *M = nullptr) {
  StringRef ABIName = Options.MCOptions.getABIName();
  if (ABIName.empty()) {
    SH::CPUKind CPUKind = SH::parseCPU(CPU);
    FloatABI::ABIType FloatABI = FloatABI::Default;
    if (M && M->getModuleFlag("float-abi"))
      FloatABI = M->getFloatABI();
    SH::ABIKind ABI = SH::ABIKind::GNU;
    if (FloatABI == FloatABI::Soft)
      ABI = SH::ABIKind::GNU;
    else if (SH::isECPU(CPUKind))
      ABI = SH::ABIKind::GNUEFloat;
    else if (FloatABI == FloatABI::Hard || CPUKind == SH::CPUKind::SH2A)
      ABI = SH::ABIKind::GNUHardFloat;
    return TT.isMuslFDPIC() ? SH::getFDPICABI(ABI) : ABI;
  }
  SH::ABIKind ABI = SH::parseABI(ABIName);
  if (ABI == SH::ABIKind::Invalid)
    reportFatalUsageError(Twine("SH does not support ABI '") + ABIName + "'");
  return ABI;
}

static void validateSHSubtarget(const SHSubtarget &ST) {
  SH::ABIKind ABI = SH::getBaseABI(ST.getABI());
  if (ST.hasSH4ALDSP() && !ST.hasDSP())
    report_fatal_error("SH4AL-DSP architectural features require DSP state");
  if (ST.hasDSP() && (!ST.hasSH4A() || ST.hasSingleFPU()))
    report_fatal_error(
        "SH DSP features require the no-FPU SH-4A architectural base");
  if (ABI == SH::ABIKind::GNUHardFloat &&
      (!ST.hasDoubleFPU() || ST.hasFPUSingleMode()))
    reportFatalUsageError(
        "SH GNU hard-float ABI requires a double FPU in public double mode");
  if (ABI == SH::ABIKind::GNUHardFloatSingle &&
      (!ST.hasDoubleFPU() || !ST.hasFPUSingleMode()))
    reportFatalUsageError("SH GNU single-mode hard-float ABI requires a double "
                          "FPU in public single mode");
  if (ABI == SH::ABIKind::GNUEFloat &&
      (!ST.hasSingleFPU() || ST.hasDoubleFPU() || !ST.hasFPUSingleMode()))
    reportFatalUsageError(
        "SH GNU E-class ABI requires a single-only FPU in public single mode");
}

static Reloc::Model getSHRelocModel(const Triple &TT,
                                    const TargetOptions &Options, StringRef CPU,
                                    std::optional<Reloc::Model> RM) {
  Reloc::Model Model = SH::isFDPICABI(getSHABI(TT, Options, CPU))
                           ? Reloc::PIC_
                           : RM.value_or(Reloc::Static);
  if (Model != Reloc::Static && Model != Reloc::PIC_)
    reportFatalUsageError(
        "SH only supports static and position-independent relocation models");
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
  initializeSHIRValidationLegacyPass(PR);
  initializeSHPrepareIRLegacyPass(PR);
  initializeSHPostRAPeepholeLegacyPass(PR);
  initializeSHInlineJumpTablesLegacyPass(PR);
  initializeSHCompressJumpTablesLegacyPass(PR);
  initializeSHDelaySlotFillerLegacyPass(PR);
  initializeSHFPUStateLegacyPass(PR);
  initializeSHLiteralIslandLegacyPass(PR);
  initializeSHLiteralPoolRangeCheckLegacyPass(PR);
}

SHTargetMachine::SHTargetMachine(const Target &T, const Triple &TT,
                                 StringRef CPU, StringRef FS,
                                 const TargetOptions &Options,
                                 std::optional<Reloc::Model> RM,
                                 std::optional<CodeModel::Model> CM,
                                 CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(T, TT, getSHCPU(CPU), getSHFeatureString(FS),
                               Options,
                               getSHRelocModel(TT, Options, getSHCPU(CPU), RM),
                               getSHCodeModel(CM), OL),
      TLOF(std::make_unique<SHTargetObjectFile>()),
      Subtarget(TT, getSHCPU(CPU), getSHCPU(CPU), getSHFeatureString(FS),
                getSHABI(TT, Options, getSHCPU(CPU)), false, *this) {
  if (JIT)
    reportFatalUsageError("SH JIT code generation is not supported");
  if (!TT.isOSBinFormatELF())
    reportFatalUsageError("SH only supports the ELF object format");
  if (Options.EmulatedTLS)
    reportFatalUsageError("SH emulated TLS is not supported");
  if (Options.EnableTLSDESC)
    reportFatalUsageError("SH TLSDESC is not supported");
  setCFIFixup(true);
  initAsmInfo();
}

const SHSubtarget *SHTargetMachine::getSubtargetImpl(const Function &F) const {
  Attribute CPUAttr = F.getFnAttribute("target-cpu");
  Attribute TuneAttr = F.getFnAttribute("tune-cpu");
  Attribute FSAttr = F.getFnAttribute("target-features");
  if (F.isIntrinsic() && !CPUAttr.isValid() && !TuneAttr.isValid() &&
      !FSAttr.isValid() && !F.getFnAttribute("target-abi").isValid())
    return &Subtarget;
  std::string CPU =
      CPUAttr.isValid() ? CPUAttr.getValueAsString().str() : TargetCPU;
  CPU = getSHCPU(CPU).str();
  std::string TuneCPU =
      TuneAttr.isValid() ? TuneAttr.getValueAsString().str() : CPU;
  TuneCPU = getSHTuneCPU(TuneCPU, CPU).str();
  std::string FS =
      FSAttr.isValid() ? FSAttr.getValueAsString().str() : TargetFS;
  bool OptSize = F.hasOptSize() || F.hasMinSize();
  SH::ABIKind ABI = getSHABI(TargetTriple, Options, CPU, F.getParent());
  Attribute ABIAttr = F.getFnAttribute("target-abi");
  StringRef FunctionABI =
      ABIAttr.isValid() ? ABIAttr.getValueAsString() : StringRef();
  StringRef TargetABI = getTargetABIName(*F.getParent());
  if (!TargetABI.empty() && SH::parseABI(TargetABI) == SH::ABIKind::Invalid)
    reportFatalUsageError(
        "SH module or target option has an unsupported target ABI");
  if (!FunctionABI.empty() && !TargetABI.empty() && FunctionABI != TargetABI)
    reportFatalUsageError("SH function target ABI does not match its module");
  if (!FunctionABI.empty()) {
    ABI = SH::parseABI(FunctionABI);
    if (ABI == SH::ABIKind::Invalid)
      reportFatalUsageError("SH function has an unsupported target ABI");
  } else if (!TargetABI.empty())
    ABI = SH::parseABI(TargetABI);
  if (F.getParent()->getModuleFlag("float-abi") && !TargetABI.empty() &&
      (F.getParent()->getFloatABI() == FloatABI::Hard) !=
          SH::usesFPUArgumentABI(ABI))
    reportFatalUsageError("SH float ABI does not match its target ABI");
  if (SH::isFDPICABI(ABI) != Subtarget.isFDPIC())
    reportFatalUsageError(
        "SH function/module FDPIC ABI does not match its target machine");
  const SHSubtarget *ST;
  if (CPU == TargetCPU && TuneCPU == CPU && FS == TargetFS &&
      ABI == Subtarget.getABI() && !OptSize) {
    ST = &Subtarget;
  } else {
    std::string Key = CPU + "+tune=" + TuneCPU + FS +
                      "+abi=" + SH::getABIName(ABI).str() +
                      (OptSize ? "+optsize" : "");
    auto &I = SubtargetMap[Key];
    if (!I)
      I = std::make_unique<SHSubtarget>(TargetTriple, CPU, TuneCPU, FS, ABI,
                                        OptSize, *this);
    ST = I.get();
  }
  validateSHSubtarget(*ST);
  return ST;
}

TargetTransformInfo
SHTargetMachine::getTargetTransformInfo(const Function &F) const {
  return TargetTransformInfo(std::make_unique<SHTTIImpl>(this, F));
}

bool SHTargetMachine::splitModule(
    Module &M, unsigned NumParts,
    function_ref<void(std::unique_ptr<Module> MPart)> ModuleCallback) {
  SplitModule(M, NumParts, ModuleCallback, /*PreserveLocals=*/true);
  return true;
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
    // Reject unsupported atomic forms before generic expansion creates runtime
    // calls. Their desired values use ordinary SelectionDAG legalization.
    addPass(createSHAtomicValidateLegacyPass(getSHTargetMachine()));
    addPass(createAtomicExpandLegacyPass());
    addPass(createSHPrepareIRLegacyPass());
    addPass(createSHIRValidationLegacyPass(getSHTargetMachine()));
    TargetPassConfig::addIRPasses();
  }
  bool addInstSelector() override {
    addPass(createSHISelDagLegacyPass(getSHTargetMachine()));
    return false;
  }
  void addPreRegAlloc() override { addPass(createSHFPUStateLegacyPass()); }
  void addPostRegAlloc() override {
    addPass(createSHFPUStateLegacyPass(/*PreserveScratch=*/true));
    addPass(createSHPostRAPeepholeLegacyPass());
  }
  void addPreEmitPass() override {
    // Fold PEI adjustments and late allocated-register patterns.
    addPass(createSHPostRAPeepholeLegacyPass());
    if (getOptLevel() != CodeGenOptLevel::None)
      addPass(createMachineCopyPropagationPass(/*UseCopyInstr=*/true));
    // Inline 32-bit tables make branch relaxation conservative. Compression
    // and delay filling must preserve the relaxed branch ranges, including
    // changes in alignment padding.
    addPass(createSHInlineJumpTablesLegacyPass());
    addPass(createSHLiteralIslandLegacyPass());
    addPass(&BranchRelaxationPassID);
    addPass(createSHCompressJumpTablesLegacyPass());
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
