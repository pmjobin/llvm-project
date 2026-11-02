//===-- SHMCTargetDesc.cpp - SH Target Descriptions ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHMCTargetDesc.h"
#include "SHBaseInfo.h"
#include "SHInstPrinter.h"
#include "SHMCAsmInfo.h"
#include "SHTargetStreamer.h"
#include "TargetInfo/SHTargetInfo.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/SHTargetParser.h"
#include "llvm/TargetParser/SubtargetFeature.h"

#include <algorithm>
#include <optional>
#include <tuple>

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "SHGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "SHGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "SHGenRegisterInfo.inc"

static MCAsmInfo *createSHMCAsmInfo(const MCRegisterInfo &MRI, const Triple &TT,
                                    const MCTargetOptions &Options) {
  MCAsmInfo *MAI = new SHMCAsmInfo(TT, Options);
  unsigned SP = MRI.getDwarfRegNum(SH::R15, true);
  MAI->addInitialFrameState(MCCFIInstruction::cfiDefCfa(nullptr, SP, 0));
  return MAI;
}

static MCInstrInfo *createSHMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitSHMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createSHMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitSHMCRegisterInfo(X, SH::PR);
  return X;
}

static void printSHCPUHelp() {
  static bool Printed = false;
  if (Printed)
    return;
  errs() << "Available CPUs for this target:\n\n";
  for (StringRef Name : SH::ValidCPUNames)
    errs() << '\t' << Name << '\n';
  errs() << "\nUse -mcpu to specify the target architecture.\n";
  Printed = true;
}

static MCSubtargetInfo *createSHMCSubtargetInfo(const Triple &TT, StringRef CPU,
                                                StringRef FS) {
  if (CPU.empty())
    CPU = SH::DefaultCPU;
  if (CPU == "help") {
    printSHCPUHelp();
    CPU = SH::DefaultCPU;
  } else if (!SH::isValidCPUName(CPU)) {
    errs() << "'" << CPU << "' is not a recognized processor for this target\n";
    return nullptr;
  }
  SubtargetFeatures Features(FS);
  SubtargetFeatures FilteredFeatures;
  for (const std::string &Feature : Features.getFeatures()) {
    if (Feature == "+cpuhelp")
      printSHCPUHelp();
    else
      FilteredFeatures.AddFeature(Feature);
  }
  return createSHMCSubtargetInfoImpl(TT, CPU, CPU,
                                     FilteredFeatures.getString());
}

static MCInstPrinter *createSHMCInstPrinter(const Triple &TT,
                                            unsigned SyntaxVariant,
                                            const MCAsmInfo &MAI,
                                            const MCInstrInfo &MII,
                                            const MCRegisterInfo &MRI) {
  if (SyntaxVariant == 0)
    return new SHInstPrinter(MAI, MII, MRI);
  return nullptr;
}

namespace {

class SHMCInstrAnalysis : public MCInstrAnalysis {
public:
  explicit SHMCInstrAnalysis(const MCInstrInfo *Info) : MCInstrAnalysis(Info) {}

  bool evaluateBranch(const MCInst &Inst, uint64_t Addr, uint64_t Size,
                      uint64_t &Target) const override {
    if ((!isConditionalBranch(Inst) && !isUnconditionalBranch(Inst) &&
         !isCall(Inst)) ||
        Inst.getNumOperands() == 0 || !Inst.getOperand(0).isImm())
      return false;
    Target = Addr + SH::PCRelativeBias + Inst.getOperand(0).getImm();
    return true;
  }
};

static bool matchesMachineProfile(const MCSubtargetInfo &STI, SH::CPUKind CPU,
                                  SH::FPUProfile Profile,
                                  bool HasST40300 = false) {
  SubtargetFeatures Required(
      SH::getCanonicalFeatureString(CPU, Profile, HasST40300));
  if (STI.checkFeatures(Required.getString()))
    return true;
  if (Profile != SH::FPUProfile::SingleOnly ||
      (CPU != SH::CPUKind::SH4 && CPU != SH::CPUKind::SH4A))
    return false;

  // Explicitly restricted single-only IR and MC feature sets remain valid
  // when driver defaults enable the separate graphics and paired facilities.
  // Match the complete restricted profile, including every architecture bit.
  SubtargetFeatures Restricted;
  for (const std::string &Feature : Required.getFeatures()) {
    StringRef Name = SubtargetFeatures::StripFlag(Feature);
    if (Name == "fpu-size-mode" || Name == "fpu-register-bank" ||
        Name == "fpu-vector")
      Restricted.AddFeature(Name, false);
    else
      Restricted.AddFeature(Feature);
  }
  return STI.checkFeatures(Restricted.getString());
}

static unsigned getMachineFlags(const MCSubtargetInfo &STI, MCContext &Ctx) {
  const FeatureBitset &Features = STI.getFeatureBits();
  if (Features[SH::FeatureDSP] || Features[SH::FeatureSH4ALDSP]) {
    if (Features[SH::FeatureSH4ALDSP] && !Features[SH::FeatureDSP]) {
      Ctx.reportError(SMLoc(),
                      "SH4AL-DSP architectural features require DSP state");
      return 0;
    }
    if (!Features[SH::FeatureSH4A] || Features[SH::FeatureSingleFPU] ||
        Features[SH::FeatureFSQRT] || Features[SH::FeatureFPUSizeMode] ||
        Features[SH::FeatureDoubleFPU] ||
        Features[SH::FeatureFPURegisterBank] ||
        Features[SH::FeatureFPUVector] || Features[SH::FeatureFPUSingleMode]) {
      Ctx.reportError(
          SMLoc(),
          "SH DSP features require the no-FPU SH-4A architectural base");
      return 0;
    }
    return ELF::EF_SH4AL_DSP;
  }
  for (auto [Profile, Flag] :
       {std::pair{SH::FPUProfile::Default, ELF::EF_SH2A},
        std::pair{SH::FPUProfile::NoFPU, ELF::EF_SH2A_NOFPU},
        std::pair{SH::FPUProfile::Single, ELF::EF_SH2A},
        std::pair{SH::FPUProfile::SingleOnly, ELF::EF_SH2A}})
    if (STI.checkFeatures(
            SH::getCanonicalFeatureString(SH::CPUKind::SH2A, Profile)))
      return Flag;
  if (STI.checkFeatures(SH::getCanonicalFeatureString(
          SH::CPUKind::SH4, SH::FPUProfile::NoFPU, false, false)))
    return ELF::EF_SH4_NOMMU_NOFPU;
  for (auto [Profile, Flag] :
       {std::pair{SH::FPUProfile::Default, ELF::EF_SH4A},
        std::pair{SH::FPUProfile::NoFPU, ELF::EF_SH4A_NOFPU},
        std::pair{SH::FPUProfile::Single, ELF::EF_SH4A},
        std::pair{SH::FPUProfile::SingleOnly, ELF::EF_SH4A}})
    if (matchesMachineProfile(STI, SH::CPUKind::SH4A, Profile))
      return Flag;
  for (auto [Profile, Flag] :
       {std::pair{SH::FPUProfile::Default, ELF::EF_SH4A},
        std::pair{SH::FPUProfile::NoFPU, ELF::EF_SH4A_NOFPU},
        std::pair{SH::FPUProfile::Single, ELF::EF_SH4A},
        std::pair{SH::FPUProfile::SingleOnly, ELF::EF_SH4A}})
    if (matchesMachineProfile(STI, SH::CPUKind::SH4, Profile, true))
      return Flag;
  for (auto [CPU, Profile, Flag] :
       {std::tuple{SH::CPUKind::SH1, SH::FPUProfile::Default, ELF::EF_SH1},
        std::tuple{SH::CPUKind::SH2, SH::FPUProfile::Default, ELF::EF_SH2},
        std::tuple{SH::CPUKind::SH2E, SH::FPUProfile::Default, ELF::EF_SH2E},
        std::tuple{SH::CPUKind::SH3, SH::FPUProfile::Default, ELF::EF_SH3},
        std::tuple{SH::CPUKind::SH3E, SH::FPUProfile::Default, ELF::EF_SH3E},
        std::tuple{SH::CPUKind::SH4, SH::FPUProfile::Default, ELF::EF_SH4},
        std::tuple{SH::CPUKind::SH4, SH::FPUProfile::NoFPU, ELF::EF_SH4_NOFPU},
        std::tuple{SH::CPUKind::SH4, SH::FPUProfile::Single, ELF::EF_SH4},
        std::tuple{SH::CPUKind::SH4, SH::FPUProfile::SingleOnly, ELF::EF_SH4}})
    if (matchesMachineProfile(STI, CPU, Profile))
      return Flag;
  Ctx.reportError(
      SMLoc(), "SH feature combination has no supported ELF machine identity");
  return 0;
}

static bool isFDPICMode(const Triple &TT, const MCTargetOptions &Options) {
  if (!Options.ABIName.empty())
    return SH::isFDPICABI(SH::parseABI(Options.ABIName));
  return TT.isMuslFDPIC() || Options.FDPIC;
}

static unsigned mergeMachineFlags(unsigned Current, unsigned Requested,
                                  MCContext &Ctx) {
  unsigned CurrentMachine = Current & ELF::EF_SH_MACH_MASK;
  if (CurrentMachine == ELF::EF_SH4AL_DSP || Requested == ELF::EF_SH4AL_DSP) {
    unsigned Other =
        CurrentMachine == ELF::EF_SH4AL_DSP ? Requested : CurrentMachine;
    switch (Other) {
    case ELF::EF_SH1:
    case ELF::EF_SH2:
    case ELF::EF_SH3:
    case ELF::EF_SH4_NOMMU_NOFPU:
    case ELF::EF_SH4_NOFPU:
    case ELF::EF_SH4A_NOFPU:
    case ELF::EF_SH4AL_DSP:
      return (Current & ~ELF::EF_SH_MACH_MASK) | ELF::EF_SH4AL_DSP;
    default:
      Ctx.reportError(
          SMLoc(),
          "SH4AL-DSP cannot be merged with the requested SH architecture");
      return Current;
    }
  }
  auto IsSH2A = [](unsigned Machine) {
    return Machine == ELF::EF_SH2A || Machine == ELF::EF_SH2A_NOFPU;
  };
  if (IsSH2A(CurrentMachine) || IsSH2A(Requested)) {
    unsigned SH2AMachine = IsSH2A(CurrentMachine) ? CurrentMachine : Requested;
    unsigned OtherMachine = IsSH2A(CurrentMachine) ? Requested : CurrentMachine;
    if (IsSH2A(OtherMachine))
      SH2AMachine = CurrentMachine == ELF::EF_SH2A || Requested == ELF::EF_SH2A
                        ? ELF::EF_SH2A
                        : ELF::EF_SH2A_NOFPU;
    else if (OtherMachine == ELF::EF_SH2E)
      SH2AMachine = ELF::EF_SH2A;
    else if (OtherMachine != ELF::EF_SH1 && OtherMachine != ELF::EF_SH2) {
      Ctx.reportError(
          SMLoc(), "SH-2A cannot be merged with the requested SH architecture");
      return Current;
    }
    return (Current & ~ELF::EF_SH_MACH_MASK) | SH2AMachine;
  }
  struct Capabilities {
    unsigned Generation;
    bool HasFPU;
    bool HasMMU;
  };
  auto Decode = [](unsigned Machine) -> std::optional<Capabilities> {
    switch (Machine) {
    case ELF::EF_SH1:
      return Capabilities{0, false, false};
    case ELF::EF_SH2:
      return Capabilities{1, false, false};
    case ELF::EF_SH2E:
      return Capabilities{1, true, false};
    case ELF::EF_SH3:
      return Capabilities{2, false, true};
    case ELF::EF_SH3E:
      return Capabilities{2, true, true};
    case ELF::EF_SH4_NOMMU_NOFPU:
      return Capabilities{3, false, false};
    case ELF::EF_SH4_NOFPU:
      return Capabilities{3, false, true};
    case ELF::EF_SH4:
      return Capabilities{3, true, true};
    case ELF::EF_SH4A_NOFPU:
      return Capabilities{4, false, true};
    case ELF::EF_SH4A:
      return Capabilities{4, true, true};
    default:
      return std::nullopt;
    }
  };
  std::optional<Capabilities> CurrentCapabilities = Decode(CurrentMachine);
  std::optional<Capabilities> RequestedCapabilities = Decode(Requested);
  if (!CurrentCapabilities || !RequestedCapabilities) {
    Ctx.reportError(SMLoc(),
                    "SH object has an unsupported ELF machine identity");
    return Current;
  }
  unsigned Generation = std::max(CurrentCapabilities->Generation,
                                 RequestedCapabilities->Generation);
  bool HasFPU = CurrentCapabilities->HasFPU || RequestedCapabilities->HasFPU;
  bool HasMMU = CurrentCapabilities->HasMMU || RequestedCapabilities->HasMMU;
  unsigned Machine;
  if (Generation == 0)
    Machine = ELF::EF_SH1;
  else if (Generation == 1)
    Machine = HasFPU ? ELF::EF_SH2E : ELF::EF_SH2;
  else if (Generation == 2)
    Machine = HasFPU ? ELF::EF_SH3E : ELF::EF_SH3;
  else if (Generation == 3 && !HasFPU && !HasMMU)
    Machine = ELF::EF_SH4_NOMMU_NOFPU;
  else if (Generation == 3)
    Machine = HasFPU ? ELF::EF_SH4 : ELF::EF_SH4_NOFPU;
  else
    Machine = HasFPU ? ELF::EF_SH4A : ELF::EF_SH4A_NOFPU;
  return (Current & ~ELF::EF_SH_MACH_MASK) | Machine;
}

class SHTargetELFStreamer : public SHTargetStreamer {
private:
  unsigned InitialFlags;
  bool EmittedMachine = false;

public:
  SHTargetELFStreamer(MCStreamer &S, const MCSubtargetInfo &STI)
      : SHTargetStreamer(S),
        InitialFlags(getMachineFlags(STI, S.getContext()) |
                     (isFDPICMode(STI.getTargetTriple(),
                                  S.getContext().getTargetOptions())
                          ? static_cast<unsigned>(ELF::EF_SH_FDPIC)
                          : 0U)) {}

  void emitMachine(const MCSubtargetInfo &STI) override {
    auto &Writer = static_cast<MCELFStreamer &>(Streamer).getWriter();
    unsigned Requested = getMachineFlags(STI, Streamer.getContext());
    if (!EmittedMachine)
      Writer.setELFHeaderEFlags(Requested |
                                (InitialFlags & ~ELF::EF_SH_MACH_MASK));
    else
      Writer.setELFHeaderEFlags(mergeMachineFlags(
          Writer.getELFHeaderEFlags(), Requested, Streamer.getContext()));
    EmittedMachine = true;
  }

  void finish() override {
    if (!EmittedMachine)
      static_cast<MCELFStreamer &>(Streamer).getWriter().setELFHeaderEFlags(
          InitialFlags);
  }
};

class SHMCObjectFileInfo : public MCObjectFileInfo {
public:
  unsigned getTextSectionAlignment() const override {
    return SH::InstructionAlignment;
  }

  void initSHMCObjectFileInfo(MCContext &Ctx, bool PIC, bool LargeCodeModel) {
    initMCObjectFileInfo(Ctx, PIC, LargeCodeModel);
    FDECFIEncoding = dwarf::DW_EH_PE_pcrel | dwarf::DW_EH_PE_sdata4;
  }
};

} // namespace

static MCObjectFileInfo *createSHMCObjectFileInfo(MCContext &Ctx, bool PIC,
                                                  bool LargeCodeModel) {
  auto *MOFI = new SHMCObjectFileInfo();
  MOFI->initSHMCObjectFileInfo(Ctx, PIC, LargeCodeModel);
  return MOFI;
}

static MCTargetStreamer *
createSHObjectTargetStreamer(MCStreamer &S, const MCSubtargetInfo &STI) {
  return new SHTargetELFStreamer(S, STI);
}

static MCTargetStreamer *createSHAsmTargetStreamer(MCStreamer &S,
                                                   formatted_raw_ostream &OS,
                                                   MCInstPrinter *InstPrinter) {
  return new SHTargetAsmStreamer(S, OS);
}

static MCInstrAnalysis *createSHMCInstrAnalysis(const MCInstrInfo *Info) {
  return new SHMCInstrAnalysis(Info);
}

static MCTargetStreamer *createSHNullTargetStreamer(MCStreamer &S) {
  return new SHTargetStreamer(S);
}

static void registerTargetMC(Target &T) {
  TargetRegistry::RegisterMCAsmInfo(T, createSHMCAsmInfo);
  TargetRegistry::RegisterMCInstrInfo(T, createSHMCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(T, createSHMCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(T, createSHMCSubtargetInfo);
  TargetRegistry::RegisterMCInstrAnalysis(T, createSHMCInstrAnalysis);
  TargetRegistry::RegisterMCInstPrinter(T, createSHMCInstPrinter);
  TargetRegistry::RegisterMCCodeEmitter(T, createSHMCCodeEmitter);
  TargetRegistry::RegisterMCAsmBackend(T, createSHMCAsmBackend);
  TargetRegistry::RegisterMCObjectFileInfo(T, createSHMCObjectFileInfo);
  TargetRegistry::RegisterObjectTargetStreamer(T, createSHObjectTargetStreamer);
  TargetRegistry::RegisterAsmTargetStreamer(T, createSHAsmTargetStreamer);
  TargetRegistry::RegisterNullTargetStreamer(T, createSHNullTargetStreamer);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeSHTargetMC() {
  registerTargetMC(getTheSHTarget());
  registerTargetMC(getTheSHLETarget());
}
