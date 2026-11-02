//===- SHTargetParser.h - SuperH target parsing support --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TARGETPARSER_SHTARGETPARSER_H
#define LLVM_TARGETPARSER_SHTARGETPARSER_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include <string>

namespace llvm::SH {

inline constexpr StringLiteral DefaultCPU = "sh2";
inline constexpr StringLiteral ValidCPUNames[] = {
    "sh1", "sh2", "sh2e", "sh2a", "sh3", "sh3e", "sh4", "sh4a", "sh4al-dsp"};
inline constexpr StringLiteral ValidTuneCPUNames[] = {
    "sh1",      "sh2",      "sh2e",     "sh2a",      "sh3",
    "sh3e",     "sh4",      "sh4a",     "sh4al-dsp", "st40-100",
    "st40-200", "st40-300", "st40-340", "st40-400",  "st40-500"};

enum class CPUKind : unsigned {
  Invalid,
  SH1,
  SH2,
  SH2E,
  SH2A,
  SH3,
  SH3E,
  SH4,
  SH4A,
  SH4ALDSP
};
enum class TuneCPUKind : unsigned {
  Invalid,
  SH1,
  SH2,
  SH2E,
  SH2A,
  SH3,
  SH3E,
  SH4,
  SH4A,
  SH4ALDSP,
  ST40100,
  ST40200,
  ST40300,
  ST40340,
  ST40400,
  ST40500
};

struct FeatureInfo {
  StringLiteral Name;
  unsigned CPUs;
  bool FrontendVisible;
};

enum class FPUProfile : unsigned { Default, NoFPU, Single, SingleOnly };

enum class ABIKind : unsigned {
  Invalid,
  GNU,
  GNUHardFloat,
  GNUHardFloatSingle,
  GNUEFloat,
  GNUFDPIC,
  GNUFDPICHardFloat,
  GNUFDPICHardFloatSingle,
  GNUFDPICEFloat
};

inline constexpr unsigned cpuSet(CPUKind CPU) {
  return 1U << static_cast<unsigned>(CPU);
}

inline constexpr unsigned AllCPUs =
    cpuSet(CPUKind::SH1) | cpuSet(CPUKind::SH2) | cpuSet(CPUKind::SH2E) |
    cpuSet(CPUKind::SH2A) | cpuSet(CPUKind::SH3) | cpuSet(CPUKind::SH3E) |
    cpuSet(CPUKind::SH4) | cpuSet(CPUKind::SH4A) | cpuSet(CPUKind::SH4ALDSP);
inline constexpr unsigned SH2AndLaterCPUs =
    cpuSet(CPUKind::SH2) | cpuSet(CPUKind::SH2E) | cpuSet(CPUKind::SH2A) |
    cpuSet(CPUKind::SH3) | cpuSet(CPUKind::SH3E) | cpuSet(CPUKind::SH4) |
    cpuSet(CPUKind::SH4A) | cpuSet(CPUKind::SH4ALDSP);
inline constexpr unsigned SH3AndLaterCPUs =
    cpuSet(CPUKind::SH3) | cpuSet(CPUKind::SH3E) | cpuSet(CPUKind::SH4) |
    cpuSet(CPUKind::SH4A) | cpuSet(CPUKind::SH4ALDSP);
inline constexpr unsigned EAndSH4CPUs =
    cpuSet(CPUKind::SH2E) | cpuSet(CPUKind::SH3E) | cpuSet(CPUKind::SH4) |
    cpuSet(CPUKind::SH4A);
inline constexpr unsigned SH2AAndLaterDynamicShiftCPUs =
    SH3AndLaterCPUs | cpuSet(CPUKind::SH2A);
inline constexpr unsigned SH2AFPUAndEAndSH4CPUs =
    EAndSH4CPUs | cpuSet(CPUKind::SH2A);

inline constexpr FeatureInfo FeatureInfos[] = {
    {"sh2", SH2AndLaterCPUs, true},
    {"sh2a", cpuSet(CPUKind::SH2A), true},
    {"dynamic-shift", SH2AAndLaterDynamicShiftCPUs, true},
    {"s-bit-control", SH3AndLaterCPUs, true},
    {"exception-state", SH3AndLaterCPUs, true},
    {"banked-gprs", SH3AndLaterCPUs, true},
    {"mmu", SH3AndLaterCPUs, true},
    {"prefetch", SH2AAndLaterDynamicShiftCPUs, true},
    {"sh4-debug-state",
     cpuSet(CPUKind::SH4) | cpuSet(CPUKind::SH4A) | cpuSet(CPUKind::SH4ALDSP),
     true},
    {"sh4-cache",
     cpuSet(CPUKind::SH4) | cpuSet(CPUKind::SH4A) | cpuSet(CPUKind::SH4ALDSP),
     true},
    {"mova-slot-illegal",
     cpuSet(CPUKind::SH4) | cpuSet(CPUKind::SH4A) | cpuSet(CPUKind::SH4ALDSP),
     false},
    {"sh4a-common", cpuSet(CPUKind::SH4A) | cpuSet(CPUKind::SH4ALDSP), true},
    {"sh4a", cpuSet(CPUKind::SH4A) | cpuSet(CPUKind::SH4ALDSP), true},
    {"dsp", cpuSet(CPUKind::SH4ALDSP), true},
    {"sh4al-dsp", cpuSet(CPUKind::SH4ALDSP), true},
    {"st40-300", 0, true},
    {"single-fpu", SH2AFPUAndEAndSH4CPUs, true},
    {"fsqrt",
     cpuSet(CPUKind::SH2A) | cpuSet(CPUKind::SH3E) | cpuSet(CPUKind::SH4) |
         cpuSet(CPUKind::SH4A),
     true},
    {"fpu-size-mode",
     cpuSet(CPUKind::SH2A) | cpuSet(CPUKind::SH4) | cpuSet(CPUKind::SH4A),
     true},
    {"double-fpu",
     cpuSet(CPUKind::SH2A) | cpuSet(CPUKind::SH4) | cpuSet(CPUKind::SH4A),
     true},
    {"fpu-register-bank", cpuSet(CPUKind::SH4) | cpuSet(CPUKind::SH4A), true},
    {"fpu-vector", cpuSet(CPUKind::SH4) | cpuSet(CPUKind::SH4A), true},
    {"fpu-single-mode", cpuSet(CPUKind::SH2E) | cpuSet(CPUKind::SH3E), true},
};

inline ABIKind parseABI(StringRef ABI) {
  if (ABI == "gnu")
    return ABIKind::GNU;
  if (ABI == "gnu-hard-float")
    return ABIKind::GNUHardFloat;
  if (ABI == "gnu-hard-float-single")
    return ABIKind::GNUHardFloatSingle;
  if (ABI == "gnu-e-float")
    return ABIKind::GNUEFloat;
  if (ABI == "gnu-fdpic")
    return ABIKind::GNUFDPIC;
  if (ABI == "gnu-fdpic-hard-float")
    return ABIKind::GNUFDPICHardFloat;
  if (ABI == "gnu-fdpic-hard-float-single")
    return ABIKind::GNUFDPICHardFloatSingle;
  if (ABI == "gnu-fdpic-e-float")
    return ABIKind::GNUFDPICEFloat;
  return ABIKind::Invalid;
}

inline StringRef getABIName(ABIKind ABI) {
  switch (ABI) {
  case ABIKind::GNU:
    return "gnu";
  case ABIKind::GNUHardFloat:
    return "gnu-hard-float";
  case ABIKind::GNUHardFloatSingle:
    return "gnu-hard-float-single";
  case ABIKind::GNUEFloat:
    return "gnu-e-float";
  case ABIKind::GNUFDPIC:
    return "gnu-fdpic";
  case ABIKind::GNUFDPICHardFloat:
    return "gnu-fdpic-hard-float";
  case ABIKind::GNUFDPICHardFloatSingle:
    return "gnu-fdpic-hard-float-single";
  case ABIKind::GNUFDPICEFloat:
    return "gnu-fdpic-e-float";
  case ABIKind::Invalid:
    return {};
  }
  return {};
}
inline bool isFDPICABI(ABIKind ABI) {
  return ABI == ABIKind::GNUFDPIC || ABI == ABIKind::GNUFDPICHardFloat ||
         ABI == ABIKind::GNUFDPICHardFloatSingle ||
         ABI == ABIKind::GNUFDPICEFloat;
}

inline ABIKind getBaseABI(ABIKind ABI) {
  switch (ABI) {
  case ABIKind::GNUFDPIC:
    return ABIKind::GNU;
  case ABIKind::GNUFDPICHardFloat:
    return ABIKind::GNUHardFloat;
  case ABIKind::GNUFDPICHardFloatSingle:
    return ABIKind::GNUHardFloatSingle;
  case ABIKind::GNUFDPICEFloat:
    return ABIKind::GNUEFloat;
  default:
    return ABI;
  }
}

inline ABIKind getFDPICABI(ABIKind ABI) {
  switch (getBaseABI(ABI)) {
  case ABIKind::GNU:
    return ABIKind::GNUFDPIC;
  case ABIKind::GNUHardFloat:
    return ABIKind::GNUFDPICHardFloat;
  case ABIKind::GNUHardFloatSingle:
    return ABIKind::GNUFDPICHardFloatSingle;
  case ABIKind::GNUEFloat:
    return ABIKind::GNUFDPICEFloat;
  default:
    return ABIKind::Invalid;
  }
}

inline bool usesFPUArgumentABI(ABIKind ABI) {
  ABI = getBaseABI(ABI);
  return ABI == ABIKind::GNUHardFloat || ABI == ABIKind::GNUHardFloatSingle ||
         ABI == ABIKind::GNUEFloat;
}

inline bool usesSH4HardFloatABI(ABIKind ABI) {
  ABI = getBaseABI(ABI);
  return ABI == ABIKind::GNUHardFloat || ABI == ABIKind::GNUHardFloatSingle;
}

inline bool isECPU(CPUKind CPU) {
  return CPU == CPUKind::SH2E || CPU == CPUKind::SH3E;
}

inline CPUKind parseCPU(StringRef CPU) {
  if (CPU == "sh1")
    return CPUKind::SH1;
  if (CPU == "sh2")
    return CPUKind::SH2;
  if (CPU == "sh2e")
    return CPUKind::SH2E;
  if (CPU == "sh2a")
    return CPUKind::SH2A;
  if (CPU == "sh3")
    return CPUKind::SH3;
  if (CPU == "sh3e")
    return CPUKind::SH3E;
  if (CPU == "sh4")
    return CPUKind::SH4;
  if (CPU == "sh4a")
    return CPUKind::SH4A;
  if (CPU == "sh4al-dsp")
    return CPUKind::SH4ALDSP;
  return CPUKind::Invalid;
}

inline StringRef getCPUName(CPUKind CPU) {
  switch (CPU) {
  case CPUKind::SH1:
    return "sh1";
  case CPUKind::SH2:
    return "sh2";
  case CPUKind::SH2E:
    return "sh2e";
  case CPUKind::SH2A:
    return "sh2a";
  case CPUKind::SH3:
    return "sh3";
  case CPUKind::SH3E:
    return "sh3e";
  case CPUKind::SH4:
    return "sh4";
  case CPUKind::SH4A:
    return "sh4a";
  case CPUKind::SH4ALDSP:
    return "sh4al-dsp";
  case CPUKind::Invalid:
    return {};
  }
  return {};
}

inline TuneCPUKind parseTuneCPU(StringRef CPU) {
  if (CPU == "sh1")
    return TuneCPUKind::SH1;
  if (CPU == "sh2")
    return TuneCPUKind::SH2;
  if (CPU == "sh2e")
    return TuneCPUKind::SH2E;
  if (CPU == "sh2a")
    return TuneCPUKind::SH2A;
  if (CPU == "sh3")
    return TuneCPUKind::SH3;
  if (CPU == "sh3e")
    return TuneCPUKind::SH3E;
  if (CPU == "sh4")
    return TuneCPUKind::SH4;
  if (CPU == "sh4a")
    return TuneCPUKind::SH4A;
  if (CPU == "sh4al-dsp")
    return TuneCPUKind::SH4ALDSP;
  if (CPU == "st40-100")
    return TuneCPUKind::ST40100;
  if (CPU == "st40-200")
    return TuneCPUKind::ST40200;
  if (CPU == "st40-300")
    return TuneCPUKind::ST40300;
  if (CPU == "st40-340")
    return TuneCPUKind::ST40340;
  if (CPU == "st40-400")
    return TuneCPUKind::ST40400;
  if (CPU == "st40-500")
    return TuneCPUKind::ST40500;
  return TuneCPUKind::Invalid;
}

inline StringRef getTuneCPUName(TuneCPUKind CPU) {
  switch (CPU) {
  case TuneCPUKind::SH1:
    return "sh1";
  case TuneCPUKind::SH2:
    return "sh2";
  case TuneCPUKind::SH2E:
    return "sh2e";
  case TuneCPUKind::SH2A:
    return "sh2a";
  case TuneCPUKind::SH3:
    return "sh3";
  case TuneCPUKind::SH3E:
    return "sh3e";
  case TuneCPUKind::SH4:
    return "sh4";
  case TuneCPUKind::SH4A:
    return "sh4a";
  case TuneCPUKind::SH4ALDSP:
    return "sh4al-dsp";
  case TuneCPUKind::ST40100:
    return "st40-100";
  case TuneCPUKind::ST40200:
    return "st40-200";
  case TuneCPUKind::ST40300:
    return "st40-300";
  case TuneCPUKind::ST40340:
    return "st40-340";
  case TuneCPUKind::ST40400:
    return "st40-400";
  case TuneCPUKind::ST40500:
    return "st40-500";
  case TuneCPUKind::Invalid:
    return {};
  }
  return {};
}

inline bool isTuneCompatible(CPUKind CPU, TuneCPUKind TuneCPU) {
  if (CPU == CPUKind::Invalid || TuneCPU == TuneCPUKind::Invalid)
    return false;
  if (TuneCPU == TuneCPUKind::SH2A || TuneCPU == TuneCPUKind::SH4A ||
      TuneCPU == TuneCPUKind::SH4ALDSP)
    return true;
  if (TuneCPU == TuneCPUKind::ST40100 || TuneCPU == TuneCPUKind::ST40200 ||
      TuneCPU == TuneCPUKind::ST40300 || TuneCPU == TuneCPUKind::ST40340 ||
      TuneCPU == TuneCPUKind::ST40400 || TuneCPU == TuneCPUKind::ST40500)
    return CPU == CPUKind::SH4;
  return getCPUName(CPU) == getTuneCPUName(TuneCPU);
}

inline bool hasFeature(CPUKind CPU, StringRef Feature,
                       bool FrontendOnly = false) {
  for (const FeatureInfo &Info : FeatureInfos)
    if (Info.Name == Feature && (!FrontendOnly || Info.FrontendVisible))
      return CPU != CPUKind::Invalid && (Info.CPUs & cpuSet(CPU));
  return false;
}

inline bool isValidFeatureName(StringRef Feature, bool FrontendOnly = false) {
  return llvm::any_of(FeatureInfos, [&](const FeatureInfo &Info) {
    return Info.Name == Feature && (!FrontendOnly || Info.FrontendVisible);
  });
}

inline bool isFPUProfileFeature(StringRef Feature) {
  return Feature == "single-fpu" || Feature == "fsqrt" ||
         Feature == "fpu-size-mode" || Feature == "double-fpu" ||
         Feature == "fpu-register-bank" || Feature == "fpu-vector" ||
         Feature == "fpu-single-mode";
}

inline bool hasProfileFeature(CPUKind CPU, StringRef Feature,
                              FPUProfile Profile) {
  if ((CPU != CPUKind::SH4 && CPU != CPUKind::SH4A && CPU != CPUKind::SH2A) ||
      Profile == FPUProfile::Default)
    return hasFeature(CPU, Feature);
  if (Feature == "fpu-single-mode")
    return Profile == FPUProfile::Single || Profile == FPUProfile::SingleOnly;
  if (Feature == "single-fpu" || Feature == "fsqrt")
    return Profile != FPUProfile::NoFPU;
  if (Feature == "double-fpu")
    return Profile == FPUProfile::Single;
  if (Feature == "fpu-size-mode")
    return Profile == FPUProfile::Single ||
           (Profile == FPUProfile::SingleOnly && CPU != CPUKind::SH2A);
  if (Feature == "fpu-register-bank" || Feature == "fpu-vector")
    return (CPU == CPUKind::SH4 || CPU == CPUKind::SH4A) &&
           Profile != FPUProfile::NoFPU;
  return hasFeature(CPU, Feature);
}

inline std::string
getCanonicalFeatureString(CPUKind CPU, FPUProfile Profile = FPUProfile::Default,
                          bool HasST40300 = false, bool HasMMU = true) {
  std::string Result;
  for (const FeatureInfo &Info : FeatureInfos) {
    if (!Result.empty())
      Result += ',';
    Result +=
        (Info.Name == "sh4a-common"
             ? CPU == CPUKind::SH4A || CPU == CPUKind::SH4ALDSP || HasST40300
         : Info.Name == "st40-300" ? HasST40300
         : Info.Name == "mmu" && CPU == CPUKind::SH4
             ? HasMMU
             : hasProfileFeature(CPU, Info.Name, Profile))
            ? '+'
            : '-';
    Result += Info.Name;
  }
  return Result;
}

inline bool isValidCPUName(StringRef CPU) {
  return parseCPU(CPU) != CPUKind::Invalid;
}

inline void fillValidCPUList(SmallVectorImpl<StringRef> &Values) {
  Values.append(std::begin(ValidCPUNames), std::end(ValidCPUNames));
}

inline bool isValidTuneCPUName(StringRef CPU) {
  return parseTuneCPU(CPU) != TuneCPUKind::Invalid;
}

inline void fillValidTuneCPUList(SmallVectorImpl<StringRef> &Values) {
  Values.append(std::begin(ValidTuneCPUNames), std::end(ValidTuneCPUNames));
}

inline constexpr StringLiteral NoFPUProfileFeatures[] = {
    "-single-fpu",        "-fsqrt",      "-fpu-size-mode",  "-double-fpu",
    "-fpu-register-bank", "-fpu-vector", "-fpu-single-mode"};
inline constexpr StringLiteral SingleProfileFeatures[] = {
    "+single-fpu",        "+fsqrt",      "+fpu-size-mode",  "+double-fpu",
    "+fpu-register-bank", "+fpu-vector", "+fpu-single-mode"};
inline constexpr StringLiteral SH2ASingleProfileFeatures[] = {
    "+single-fpu",        "+fsqrt",      "+fpu-size-mode",  "+double-fpu",
    "-fpu-register-bank", "-fpu-vector", "+fpu-single-mode"};
// SH4 single-only restricts arithmetic and the public ABI, not the distinct
// paired-single transfer and graphics facilities. Keep SH2A's narrower profile.
inline constexpr StringLiteral SingleOnlyProfileFeatures[] = {
    "+single-fpu",        "+fsqrt",      "+fpu-size-mode",  "-double-fpu",
    "+fpu-register-bank", "+fpu-vector", "+fpu-single-mode"};
inline constexpr StringLiteral SH2ASingleOnlyProfileFeatures[] = {
    "+single-fpu",        "+fsqrt",      "-fpu-size-mode",  "-double-fpu",
    "-fpu-register-bank", "-fpu-vector", "+fpu-single-mode"};

inline ArrayRef<StringLiteral>
getFPUProfileFeatures(FPUProfile Profile, CPUKind CPU = CPUKind::SH4) {
  switch (Profile) {
  case FPUProfile::Default:
    return {};
  case FPUProfile::NoFPU:
    return NoFPUProfileFeatures;
  case FPUProfile::Single:
    return CPU == CPUKind::SH2A
               ? ArrayRef<StringLiteral>(SH2ASingleProfileFeatures)
               : ArrayRef<StringLiteral>(SingleProfileFeatures);
  case FPUProfile::SingleOnly:
    return CPU == CPUKind::SH2A
               ? ArrayRef<StringLiteral>(SH2ASingleOnlyProfileFeatures)
               : ArrayRef<StringLiteral>(SingleOnlyProfileFeatures);
  }
  return {};
}

inline StringRef getGNUAssemblerISA(StringRef CPU,
                                    FPUProfile Profile = FPUProfile::Default,
                                    bool HasST40300 = false,
                                    bool HasMMU = true) {
  if (parseCPU(CPU) == CPUKind::SH4ALDSP)
    return "sh4al-dsp";
  if (parseCPU(CPU) == CPUKind::SH1)
    return "sh";
  if (parseCPU(CPU) == CPUKind::SH2A)
    return Profile == FPUProfile::NoFPU ? "sh2a-nofpu" : "sh2a";
  if (parseCPU(CPU) == CPUKind::SH4A)
    return Profile == FPUProfile::NoFPU ? "sh4a-nofpu" : "sh4a";
  if (parseCPU(CPU) == CPUKind::SH4 && HasST40300)
    return Profile == FPUProfile::NoFPU ? "sh4a-nofpu" : "sh4a";
  if (parseCPU(CPU) == CPUKind::SH4 && Profile == FPUProfile::NoFPU && !HasMMU)
    return "sh4-nommu-nofpu";
  if (parseCPU(CPU) == CPUKind::SH4 && Profile == FPUProfile::NoFPU)
    return "sh4-nofpu";
  return getCPUName(parseCPU(CPU));
}

} // namespace llvm::SH

#endif // LLVM_TARGETPARSER_SHTARGETPARSER_H
