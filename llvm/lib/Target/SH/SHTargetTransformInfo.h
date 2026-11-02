//===-- SHTargetTransformInfo.h - SH specific TTI --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHTARGETTRANSFORMINFO_H
#define LLVM_LIB_TARGET_SH_SHTARGETTRANSFORMINFO_H

#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/BasicTTIImpl.h"

namespace llvm {

class SHSubtarget;
class SHTargetLowering;
class SHTargetMachine;

class SHTTIImpl final : public BasicTTIImplBase<SHTTIImpl> {
  using BaseT = BasicTTIImplBase<SHTTIImpl>;
  friend BaseT;

  const SHSubtarget *ST;
  const SHTargetLowering *TLI;
  const Function *Fn;
  bool OptSize;

  const SHSubtarget *getST() const { return ST; }
  const SHTargetLowering *getTLI() const { return TLI; }

public:
  explicit SHTTIImpl(const SHTargetMachine *TM, const Function &F);
  int getInliningLastCallToStaticBonus() const override;
  std::optional<int> getInliningOptSizeThreshold() const override;
  bool isLegalAddressingMode(Type *Ty, GlobalValue *BaseGV, int64_t BaseOffset,
                             bool HasBaseReg, int64_t Scale, unsigned AddrSpace,
                             Instruction *I = nullptr,
                             int64_t ScalableOffset = 0) const override;
  bool areInlineCompatible(const Function *Caller,
                           const Function *Callee) const override;
  bool
  shouldCopyAttributeWhenOutliningFrom(const Function *Caller,
                                       const Attribute &Attr) const override;
  bool isLSRCostLess(const TargetTransformInfo::LSRCost &C1,
                     const TargetTransformInfo::LSRCost &C2) const override;
  TargetTransformInfo::AddressingModeKind
  getPreferredAddressingMode(const Loop *L, ScalarEvolution *SE) const override;
  bool isProfitableLSRChainElement(Instruction *I) const override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_SH_SHTARGETTRANSFORMINFO_H
