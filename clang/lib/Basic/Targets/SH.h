//===--- SH.h - Declare SuperH target feature support -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares SH TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_SH_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_SH_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/SHTargetParser.h"

namespace clang {
namespace targets {

class LLVM_LIBRARY_VISIBILITY SHTargetInfo : public TargetInfo {
  std::string ABI = "gnu";
  llvm::SH::ABIKind ABIKind = llvm::SH::ABIKind::GNU;

  static const char *const GCCRegNames[];
  static const TargetInfo::GCCRegAlias GCCRegAliases[];

public:
  SHTargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts);

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  llvm::SmallVector<Builtin::InfosShard> getTargetBuiltins() const override;

  bool
  initFeatureMap(llvm::StringMap<bool> &Features, DiagnosticsEngine &Diags,
                 StringRef CPU,
                 const std::vector<std::string> &FeaturesVec) const override;
  bool isValidCPUName(StringRef Name) const override;
  void fillValidCPUList(SmallVectorImpl<StringRef> &Values) const override;
  bool isValidTuneCPUName(StringRef Name) const override;
  void fillValidTuneCPUList(SmallVectorImpl<StringRef> &Values) const override;
  bool supportsTargetAttributeTune() const override { return true; }
  bool setCPU(StringRef Name) override;
  bool setABI(const std::string &Name) override;
  StringRef getABI() const override { return ABI; }
  bool handleTargetFeatures(std::vector<std::string> &Features,
                            DiagnosticsEngine &Diags) override;
  bool hasFeature(StringRef Feature) const override;
  bool isValidFeatureName(StringRef Feature) const override;
  bool allowsLargerPreferedTypeAlignment() const override { return false; }

  ArrayRef<const char *> getGCCRegNames() const override;
  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override;

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override;
  std::string convertConstraint(const char *&Constraint) const override;
  bool validateOutputSize(const llvm::StringMap<bool> &FeatureMap,
                          StringRef Constraint, unsigned Size) const override;
  bool validateInputSize(const llvm::StringMap<bool> &FeatureMap,
                         StringRef Constraint, unsigned Size) const override;
  std::string_view getClobbers() const override { return ""; }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return llvm::SH::usesFPUArgumentABI(ABIKind)
               ? TargetInfo::SHHardFloatBuiltinVaList
               : TargetInfo::VoidPtrBuiltinVaList;
  }
};

} // namespace targets
} // namespace clang

#endif // LLVM_CLANG_LIB_BASIC_TARGETS_SH_H
