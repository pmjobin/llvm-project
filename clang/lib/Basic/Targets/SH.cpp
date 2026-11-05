//===--- SH.cpp - Implement SuperH target feature support ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SH.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/MacroBuilder.h"
#include "clang/Basic/TargetBuiltins.h"
#include "llvm/TargetParser/SHTargetParser.h"

using namespace clang;
using namespace clang::targets;

static constexpr int NumBuiltins =
    clang::SH::LastTSBuiltin - Builtin::FirstTSBuiltin;

static constexpr llvm::StringTable BuiltinStrings =
    CLANG_BUILTIN_STR_TABLE_START
#define BUILTIN CLANG_BUILTIN_STR_TABLE
#define TARGET_BUILTIN CLANG_TARGET_BUILTIN_STR_TABLE
#include "clang/Basic/BuiltinsSH.def"
    ;

static constexpr auto BuiltinInfos = Builtin::MakeInfos<NumBuiltins>({
#define BUILTIN CLANG_BUILTIN_ENTRY
#define TARGET_BUILTIN CLANG_TARGET_BUILTIN_ENTRY
#include "clang/Basic/BuiltinsSH.def"
});

const char *const SHTargetInfo::GCCRegNames[] = {
    "r0",   "r1",   "r2",   "r3",   "r4",   "r5",   "r6",   "r7",    "r8",
    "r9",   "r10",  "r11",  "r12",  "r13",  "r14",  "r15",  "pc",    "pr",
    "gbr",  "vbr",  "mach", "macl", "sr",   "t",    "fpul", "fpscr", "fr0",
    "fr1",  "fr2",  "fr3",  "fr4",  "fr5",  "fr6",  "fr7",  "fr8",   "fr9",
    "fr10", "fr11", "fr12", "fr13", "fr14", "fr15", "dr0",  "dr2",   "dr4",
    "dr6",  "dr8",  "dr10", "dr12", "dr14", "tbr",  "a0",   "a1",    "a0g",
    "a1g",  "x0",   "x1",   "y0",   "y1",   "m0",   "m1",   "dsr",   "rs",
    "re",   "mod",
};

const TargetInfo::GCCRegAlias SHTargetInfo::GCCRegAliases[] = {
    {{"sp"}, "r15"},
    {{"fp"}, "r14"},
};

SHTargetInfo::SHTargetInfo(const llvm::Triple &Triple,
                           const TargetOptions &Opts)
    : TargetInfo(Triple) {
  if (!Opts.ABI.empty()) {
    ABI = Opts.ABI;
    ABIKind = llvm::SH::parseABI(Opts.ABI);
  } else if (Triple.isMuslFDPIC()) {
    ABI = "gnu-fdpic";
    ABIKind = llvm::SH::ABIKind::GNUFDPIC;
  } else if (llvm::SH::isECPU(llvm::SH::parseCPU(Opts.CPU))) {
    ABI = "gnu-e-float";
    ABIKind = llvm::SH::ABIKind::GNUEFloat;
  } else if (llvm::SH::parseCPU(Opts.CPU) == llvm::SH::CPUKind::SH2A) {
    ABI = "gnu-hard-float";
    ABIKind = llvm::SH::ABIKind::GNUHardFloat;
  }
  NoAsmVariants = true;
  HasMustTail = false;
  HasFPReturn = true;
  LongLongAlign = 32;
  DoubleAlign = 32;
  LongDoubleAlign = 32;
  SuitableAlign = 32;
  DefaultAlignForAttributeAligned = 32;
  NewAlign = 32;
  SizeType = UnsignedInt;
  PtrDiffType = SignedInt;
  IntPtrType = SignedInt;
  WCharType = SignedLong;
  WIntType = UnsignedInt;
  UseZeroLengthBitfieldAlignment = true;
  ZeroLengthBitfieldBoundary = 32;
  MaxAtomicPromoteWidth = 0;
  MaxAtomicInlineWidth = 0;
  resetDataLayout();
}

llvm::SmallVector<Builtin::InfosShard> SHTargetInfo::getTargetBuiltins() const {
  return {{&BuiltinStrings, BuiltinInfos}};
}

void SHTargetInfo::getTargetDefines(const LangOptions &Opts,
                                    MacroBuilder &Builder) const {
  (void)Opts;
  Builder.defineMacro("__sh__");
  if (hasFeature("fpu-vector"))
    Builder.defineMacro("__SH_GRAPHICS__");
  if (hasFeature("sh4a") && hasFeature("single-fpu"))
    Builder.defineMacro("__SH_APPROX_MATH__");
  if (llvm::SH::isFDPICABI(ABIKind)) {
    Builder.defineMacro("__SH_FDPIC__", "1");
    Builder.defineMacro("__FDPIC__", "1");
  }
  StringRef CPU = getTargetOpts().CPU;
  if (CPU.empty())
    CPU = llvm::SH::DefaultCPU;
  llvm::SH::CPUKind CPUKind = llvm::SH::parseCPU(CPU);
  if (CPUKind == llvm::SH::CPUKind::SH2A) {
    Builder.defineMacro("__SH2A__");
    Builder.defineMacro("__SH_ATOMIC_MODEL_NONE__");
    bool HasSingleFPU = getTargetOpts().FeatureMap.lookup("single-fpu");
    bool HasDoubleFPU = getTargetOpts().FeatureMap.lookup("double-fpu");
    bool HasSingleMode = getTargetOpts().FeatureMap.lookup("fpu-single-mode");
    if (!HasSingleFPU)
      Builder.defineMacro("__SH2A_NOFPU__");
    else if (!HasDoubleFPU)
      Builder.defineMacro("__SH2A_SINGLE_ONLY__");
    else if (HasSingleMode)
      Builder.defineMacro("__SH2A_SINGLE__");
    else
      Builder.defineMacro("__SH2A_DOUBLE__");
    if (HasSingleFPU)
      Builder.defineMacro("__SH_FPU_ANY__");
    if (HasDoubleFPU)
      Builder.defineMacro("__SH_FPU_DOUBLE__");
  } else if (CPUKind == llvm::SH::CPUKind::SH4A ||
             CPUKind == llvm::SH::CPUKind::SH4ALDSP) {
    Builder.defineMacro("__SH4A__");
    Builder.defineMacro("__SH_ATOMIC_MODEL_NONE__");
    bool HasSingleFPU = getTargetOpts().FeatureMap.lookup("single-fpu");
    bool HasDoubleFPU = getTargetOpts().FeatureMap.lookup("double-fpu");
    bool HasSingleMode = getTargetOpts().FeatureMap.lookup("fpu-single-mode");
    if (!HasSingleFPU)
      Builder.defineMacro("__SH4_NOFPU__");
    else if (!HasDoubleFPU)
      Builder.defineMacro("__SH4_SINGLE_ONLY__");
    else if (HasSingleMode)
      Builder.defineMacro("__SH4_SINGLE__");
    else
      Builder.defineMacro("__SH4__");
    if (HasSingleFPU)
      Builder.defineMacro("__SH_FPU_ANY__");
    if (HasDoubleFPU)
      Builder.defineMacro("__SH_FPU_DOUBLE__");
  } else if (CPUKind == llvm::SH::CPUKind::SH4) {
    if (getTargetOpts().FeatureMap.lookup("st40-300"))
      Builder.defineMacro("__SH4_300__");
    bool HasSingleFPU = getTargetOpts().FeatureMap.lookup("single-fpu");
    bool HasDoubleFPU = getTargetOpts().FeatureMap.lookup("double-fpu");
    bool HasSingleMode = getTargetOpts().FeatureMap.lookup("fpu-single-mode");
    if (!HasSingleFPU) {
      Builder.defineMacro("__SH4_NOFPU__");
      Builder.defineMacro("__sh3__");
      Builder.defineMacro("__SH3__");
    } else if (!HasDoubleFPU)
      Builder.defineMacro("__SH4_SINGLE_ONLY__");
    else if (HasSingleMode)
      Builder.defineMacro("__SH4_SINGLE__");
    else
      Builder.defineMacro("__SH4__");
    if (HasSingleFPU)
      Builder.defineMacro("__SH_FPU_ANY__");
    if (HasDoubleFPU)
      Builder.defineMacro("__SH_FPU_DOUBLE__");
  } else if (CPUKind == llvm::SH::CPUKind::SH3E) {
    Builder.defineMacro("__SH3E__");
    Builder.defineMacro("__SH_FPU_ANY__");
  } else if (CPUKind == llvm::SH::CPUKind::SH3) {
    Builder.defineMacro("__sh3__");
    Builder.defineMacro("__SH3__");
  } else if (CPUKind == llvm::SH::CPUKind::SH2E) {
    Builder.defineMacro("__SH2E__");
    Builder.defineMacro("__SH_FPU_ANY__");
  } else if (CPUKind == llvm::SH::CPUKind::SH1) {
    Builder.defineMacro("__sh1__");
    Builder.defineMacro("__SH1__");
    Builder.defineMacro("__SH_ATOMIC_MODEL_NONE__");
  } else {
    Builder.defineMacro("__sh2__");
    Builder.defineMacro("__SH2__");
  }
  Builder.defineMacro(BigEndian ? "__BIG_ENDIAN__" : "__LITTLE_ENDIAN__");
  // Floating-point words follow the byte order for both SH ELF endiannesses.
  Builder.defineMacro("__FLOAT_WORD_ORDER__", BigEndian ? "__ORDER_BIG_ENDIAN__"
                                                     : "__ORDER_LITTLE_ENDIAN__");
}

bool SHTargetInfo::initFeatureMap(
    llvm::StringMap<bool> &Features, DiagnosticsEngine &Diags, StringRef CPU,
    const std::vector<std::string> &FeaturesVec) const {
  if (CPU.empty())
    CPU = llvm::SH::DefaultCPU;
  llvm::SH::CPUKind CPUKind = llvm::SH::parseCPU(CPU);
  StringRef TuneCPU = getTargetOpts().TuneCPU;
  if (!TuneCPU.empty() &&
      !llvm::SH::isTuneCompatible(CPUKind, llvm::SH::parseTuneCPU(TuneCPU))) {
    Diags.Report(diag::err_invalid_feature_combination)
        << ("SH tune CPU '" + TuneCPU +
            "' is incompatible with architecture CPU '" + CPU + "'")
               .str();
    return false;
  }
  if (CPUKind == llvm::SH::CPUKind::Invalid)
    return false;
  for (const llvm::SH::FeatureInfo &Info : llvm::SH::FeatureInfos)
    if (Info.FrontendVisible && llvm::SH::hasFeature(CPUKind, Info.Name))
      Features[Info.Name] = true;
  StringRef LastFPUFeature;
  for (StringRef Feature : FeaturesVec) {
    if (Feature.empty())
      continue;
    bool Enable = Feature.starts_with("+");
    StringRef Name = Feature.drop_front();
    bool IsValid = (Enable || Feature.starts_with("-")) &&
                   llvm::SH::isValidFeatureName(Name, /*FrontendOnly=*/true);
    bool IsSH4FPUProfileFeature = (CPUKind == llvm::SH::CPUKind::SH4 ||
                                   CPUKind == llvm::SH::CPUKind::SH4A ||
                                   CPUKind == llvm::SH::CPUKind::SH2A) &&
                                  llvm::SH::isFPUProfileFeature(Name);
    bool IsST40300Feature =
        CPUKind == llvm::SH::CPUKind::SH4 && Name == "st40-300";
    bool IsSH4MMUFeature = CPUKind == llvm::SH::CPUKind::SH4 && Name == "mmu";
    if (!IsValid || (!IsSH4FPUProfileFeature && !IsST40300Feature &&
                     !IsSH4MMUFeature && Enable != Features.lookup(Name))) {
      Diags.Report(diag::err_invalid_feature_combination)
          << ("unsupported SH target feature '" + Feature + "'").str();
      return false;
    }
    Features[Name] = Enable;
    if (IsSH4FPUProfileFeature)
      LastFPUFeature = Feature;
  }
  auto Has = [&](StringRef Feature) { return Features.lookup(Feature); };
  auto MatchesProfile = [&](llvm::SH::FPUProfile Profile) {
    return llvm::all_of(
        llvm::SH::FeatureInfos, [&](const llvm::SH::FeatureInfo &Info) {
          return !llvm::SH::isFPUProfileFeature(Info.Name) ||
                 Has(Info.Name) ==
                     llvm::SH::hasProfileFeature(CPUKind, Info.Name, Profile);
        });
  };
  llvm::SH::FPUProfile Profile = llvm::SH::FPUProfile::Default;
  bool HasSelectableFPUProfile = CPUKind == llvm::SH::CPUKind::SH4 ||
                                 CPUKind == llvm::SH::CPUKind::SH4A ||
                                 CPUKind == llvm::SH::CPUKind::SH2A;
  bool CoherentCapabilities =
      !HasSelectableFPUProfile || MatchesProfile(Profile);
  if (HasSelectableFPUProfile)
    for (llvm::SH::FPUProfile Candidate :
         {llvm::SH::FPUProfile::NoFPU, llvm::SH::FPUProfile::Single,
          llvm::SH::FPUProfile::SingleOnly})
      if (MatchesProfile(Candidate)) {
        Profile = Candidate;
        CoherentCapabilities = true;
        break;
      }
  llvm::SH::ABIKind BaseABI = llvm::SH::getBaseABI(ABIKind);
  bool CoherentABI =
      BaseABI == llvm::SH::ABIKind::GNU ||
      (BaseABI == llvm::SH::ABIKind::GNUHardFloat &&
       (CPUKind == llvm::SH::CPUKind::SH4 ||
        CPUKind == llvm::SH::CPUKind::SH4A ||
        CPUKind == llvm::SH::CPUKind::SH2A) &&
       Profile == llvm::SH::FPUProfile::Default) ||
      (BaseABI == llvm::SH::ABIKind::GNUHardFloatSingle &&
       (CPUKind == llvm::SH::CPUKind::SH4 ||
        CPUKind == llvm::SH::CPUKind::SH4A ||
        CPUKind == llvm::SH::CPUKind::SH2A) &&
       Profile == llvm::SH::FPUProfile::Single) ||
      (BaseABI == llvm::SH::ABIKind::GNUEFloat && Has("single-fpu") &&
       !Has("double-fpu") && Has("fpu-single-mode"));
  if (CPUKind == llvm::SH::CPUKind::SH4 && !Has("mmu") &&
      Profile != llvm::SH::FPUProfile::NoFPU) {
    Diags.Report(diag::err_invalid_feature_combination)
        << "SH4 without an MMU requires the no-FPU implementation profile";
    return false;
  }
  if (!CoherentCapabilities && !LastFPUFeature.empty()) {
    Diags.Report(diag::err_invalid_feature_combination)
        << ("unsupported SH target feature '" + LastFPUFeature + "'").str();
    return false;
  }
  if (!CoherentABI) {
    Diags.Report(diag::err_invalid_feature_combination)
        << "incoherent SH FPU capability or ABI profile";
    return false;
  }
  return true;
}

bool SHTargetInfo::isValidCPUName(StringRef Name) const {
  return llvm::SH::isValidCPUName(Name);
}

void SHTargetInfo::fillValidCPUList(SmallVectorImpl<StringRef> &Values) const {
  llvm::SH::fillValidCPUList(Values);
}

bool SHTargetInfo::isValidTuneCPUName(StringRef Name) const {
  return llvm::SH::isValidTuneCPUName(Name);
}

void SHTargetInfo::fillValidTuneCPUList(
    SmallVectorImpl<StringRef> &Values) const {
  llvm::SH::fillValidTuneCPUList(Values);
}

bool SHTargetInfo::setCPU(StringRef Name) { return isValidCPUName(Name); }

bool SHTargetInfo::setABI(const std::string &Name) {
  llvm::SH::ABIKind NewABIKind = llvm::SH::parseABI(Name);
  if (NewABIKind == llvm::SH::ABIKind::Invalid)
    return false;
  ABI = Name;
  ABIKind = NewABIKind;
  resetDataLayout();
  return true;
}

bool SHTargetInfo::handleTargetFeatures(std::vector<std::string> &Features,
                                        DiagnosticsEngine &Diags) {
  (void)Diags;
  StringRef CPU = getTargetOpts().CPU;
  if (CPU.empty())
    CPU = llvm::SH::DefaultCPU;
  llvm::SH::CPUKind CPUKind = llvm::SH::parseCPU(CPU);
  bool HasSingleFPU = llvm::SH::hasFeature(CPUKind, "single-fpu");
  bool HasDoubleFPU = llvm::SH::hasFeature(CPUKind, "double-fpu");
  for (StringRef Feature : Features) {
    if (Feature == "+single-fpu")
      HasSingleFPU = true;
    else if (Feature == "-single-fpu")
      HasSingleFPU = false;
    else if (Feature == "+double-fpu")
      HasDoubleFPU = true;
    else if (Feature == "-double-fpu")
      HasDoubleFPU = false;
  }
  if ((CPUKind == llvm::SH::CPUKind::SH4 ||
       CPUKind == llvm::SH::CPUKind::SH4A ||
       CPUKind == llvm::SH::CPUKind::SH2A) &&
      HasSingleFPU && !HasDoubleFPU) {
    DoubleWidth = 32;
    DoubleAlign = 32;
    DoubleFormat = &llvm::APFloat::IEEEsingle();
  }
  return true;
}

bool SHTargetInfo::hasFeature(StringRef Feature) const {
  StringRef CPU = getTargetOpts().CPU;
  if (CPU.empty())
    CPU = llvm::SH::DefaultCPU;
  llvm::SH::CPUKind CPUKind = llvm::SH::parseCPU(CPU);
  if (Feature == "sh")
    return true;
  if (Feature == "sh1")
    return true;
  if (Feature == "sh2" || Feature == "sh2e" || Feature == "sh2a" ||
      Feature == "sh3" || Feature == "sh3e" || Feature == "sh4" ||
      Feature == "sh4a" || Feature == "sh4al-dsp")
    return (Feature == "sh2" && getTargetOpts().FeatureMap.lookup("sh2")) ||
           (Feature == "sh4a" && getTargetOpts().FeatureMap.lookup("sh4a")) ||
           (Feature == "sh4al-dsp" &&
            getTargetOpts().FeatureMap.lookup("sh4al-dsp")) ||
           llvm::SH::getCPUName(CPUKind) == Feature;
  if (llvm::SH::isValidFeatureName(Feature, /*FrontendOnly=*/true))
    return getTargetOpts().FeatureMap.lookup(Feature);
  return llvm::SH::hasFeature(CPUKind, Feature, /*FrontendOnly=*/true);
}

bool SHTargetInfo::isValidFeatureName(StringRef Feature) const {
  return llvm::SH::isValidFeatureName(Feature, /*FrontendOnly=*/true);
}

ArrayRef<const char *> SHTargetInfo::getGCCRegNames() const {
  return llvm::ArrayRef(GCCRegNames);
}

ArrayRef<TargetInfo::GCCRegAlias> SHTargetInfo::getGCCRegAliases() const {
  return llvm::ArrayRef(GCCRegAliases);
}

bool SHTargetInfo::validateAsmConstraint(
    const char *&Name, TargetInfo::ConstraintInfo &Info) const {
  switch (*Name) {
  case 'r':
  case 'z':
  case 'l':
  case 't':
  case 'u':
  case 'x':
    Info.setAllowsRegister();
    return true;
  case 'I':
    if (Name[1] == '0' && Name[2] == '8')
      Name += 2;
    else
      return false;
    Info.setRequiresImmediate(-128, 127);
    return true;
  case 'K':
    if (Name[1] == '0' && Name[2] == '4')
      Info.setRequiresImmediate(0, 15);
    else if (Name[1] == '0' && Name[2] == '5')
      Info.setRequiresImmediate(0, 31);
    else if (Name[1] == '0' && Name[2] == '8')
      Info.setRequiresImmediate(0, 255);
    else
      return false;
    Name += 2;
    return true;
  case 'M':
    Info.setRequiresImmediate(1);
    return true;
  case 'N':
    Info.setRequiresImmediate(0);
    return true;
  case 'J':
    if ((Name[1] != 'm' || (Name[2] != 'b' && Name[2] != 'w')) &&
        !(Name[1] == 'h' && Name[2] == 'b'))
      return false;
    Name += 2;
    Info.setRequiresImmediate();
    return true;
  case 'P':
    if (!(Name[1] == '2' && Name[2] == '7'))
      return false;
    Name += 2;
    Info.setRequiresImmediate();
    return true;
  case 'm':
    Info.setAllowsMemory();
    return true;
  case 'p':
    Info.setAllowsRegister();
    return true;
  default:
    return false;
  }
}

std::string SHTargetInfo::convertConstraint(const char *&Constraint) const {
  if ((*Constraint == 'I' && Constraint[1] == '0' && Constraint[2] == '8') ||
      (*Constraint == 'K' && Constraint[1] == '0' &&
       (Constraint[2] == '4' || Constraint[2] == '5' ||
        Constraint[2] == '8')) ||
      (*Constraint == 'J' &&
       ((Constraint[1] == 'm' &&
         (Constraint[2] == 'b' || Constraint[2] == 'w')) ||
        (Constraint[1] == 'h' && Constraint[2] == 'b'))) ||
      (*Constraint == 'P' && Constraint[1] == '2' && Constraint[2] == '7')) {
    std::string Result = std::string("@3") + std::string(Constraint, 3);
    Constraint += 2;
    return Result;
  }
  return TargetInfo::convertConstraint(Constraint);
}

bool SHTargetInfo::validateOutputSize(const llvm::StringMap<bool> &FeatureMap,
                                      StringRef Constraint,
                                      unsigned Size) const {
  (void)FeatureMap;
  bool IsReadWrite = Constraint.starts_with("+");
  Constraint = Constraint.ltrim("=+&");
  return Constraint != "t" || (!IsReadWrite && Size == 32);
}

bool SHTargetInfo::validateInputSize(const llvm::StringMap<bool> &FeatureMap,
                                     StringRef Constraint,
                                     unsigned Size) const {
  (void)FeatureMap;
  (void)Size;
  return Constraint != "t";
}
