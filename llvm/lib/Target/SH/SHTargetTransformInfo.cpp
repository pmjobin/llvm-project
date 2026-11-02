//===-- SHTargetTransformInfo.cpp - SH specific TTI -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHTargetTransformInfo.h"
#include "SHSubtarget.h"
#include "SHTargetMachine.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

namespace {

// Retain a substantial incentive for eliminating a single-use local function,
// but do not make that decision effectively unconditional. Final SH code can
// still grow after the callee is eliminated due to lowering and literal pools.
constexpr int SHOptSizeLastCallToStaticBonus = 1000;

} // end anonymous namespace

SHTTIImpl::SHTTIImpl(const SHTargetMachine *TM, const Function &F)
    : BaseT(TM, F.getDataLayout()), ST(TM->getSubtargetImpl(F)),
      TLI(ST->getTargetLowering()), Fn(&F),
      OptSize(F.hasOptSize() || F.hasMinSize()) {}

int SHTTIImpl::getInliningLastCallToStaticBonus() const {
  if (OptSize)
    return SHOptSizeLastCallToStaticBonus;
  return BaseT::getInliningLastCallToStaticBonus();
}

std::optional<int> SHTTIImpl::getInliningOptSizeThreshold() const {
  // Repeated small callees can grow substantially after SH legalization,
  // register allocation and literal-pool placement. Keep ordinary size-mode
  // inlining conservative without suppressing hints or last-call elimination.
  return 25;
}

bool SHTTIImpl::isLegalAddressingMode(Type *Ty, GlobalValue *BaseGV,
                                      int64_t BaseOffset, bool HasBaseReg,
                                      int64_t Scale, unsigned AddrSpace,
                                      Instruction *I,
                                      int64_t ScalableOffset) const {
  // Size optimization uses lowering's scalar displacement model. For speed,
  // only longword displacements are free: narrow displacement accesses also
  // constrain the loaded or stored value to R0.
  if (OptSize)
    return BaseT::isLegalAddressingMode(Ty, BaseGV, BaseOffset, HasBaseReg,
                                        Scale, AddrSpace, I, ScalableOffset);
  if (BaseGV || ScalableOffset || AddrSpace != 0)
    return false;
  if (Scale != 0)
    return Scale == 1 && BaseOffset == 0;
  if (!Ty || Ty->isVoidTy())
    return BaseOffset == 0;
  if (Ty->isPointerTy() || Ty->isIntegerTy(32))
    return BaseOffset >= 0 && BaseOffset <= 60 && BaseOffset % 4 == 0;
  return BaseOffset == 0;
}

bool SHTTIImpl::areInlineCompatible(const Function *Caller,
                                    const Function *Callee) const {
  if (Callee->hasFnAttribute("sh-interrupt-handler"))
    return false;
  return BaseT::areInlineCompatible(Caller, Callee);
}

bool SHTTIImpl::shouldCopyAttributeWhenOutliningFrom(
    const Function *Caller, const Attribute &Attr) const {
  if (Attr.isStringAttribute() &&
      (Attr.getKindAsString() == "sh-interrupt-handler" ||
       Attr.getKindAsString() == "sh-nosave-low-regs" ||
       Attr.getKindAsString() == "sh-resbank" ||
       Attr.getKindAsString() == "sh-sp-switch" ||
       Attr.getKindAsString() == "sh-trap-exit"))
    return false;
  return BaseT::shouldCopyAttributeWhenOutliningFrom(Caller, Attr);
}

bool SHTTIImpl::isLSRCostLess(const TargetTransformInfo::LSRCost &C1,
                              const TargetTransformInfo::LSRCost &C2) const {
  if (C1.NumRegs == C2.NumRegs && C1.NumRegs <= 3 && C1.Insns != C2.Insns)
    return C1.Insns < C2.Insns;
  return BaseT::isLSRCostLess(C1, C2);
}

TargetTransformInfo::AddressingModeKind
SHTTIImpl::getPreferredAddressingMode(const Loop *L,
                                      ScalarEvolution *SE) const {
  if (OptSize || Fn->hasOptNone() || !ST->hasSH2() ||
      ST->getSchedModel().IssueWidth != 1 ||
      TLI->getTargetMachine().getOptLevel() == CodeGenOptLevel::None || !SE ||
      L->getNumBlocks() != 1)
    return TargetTransformInfo::AMK_None;
  // Credit writeback only where SH's fixed increment matches an unconditional
  // narrow stream. Sharing an integer IV otherwise makes LSR charge for the
  // pointer update while treating the base-plus-index address as free.
  for (Instruction &I : *L->getHeader()) {
    auto *Load = dyn_cast<LoadInst>(&I);
    if (!Load || !Load->isSimple() || Load->getPointerAddressSpace() != 0)
      continue;
    Type *Ty = Load->getType();
    if (!Ty->isIntegerTy(8) && !Ty->isIntegerTy(16))
      continue;
    const auto *AR =
        dyn_cast<SCEVAddRecExpr>(SE->getSCEV(Load->getPointerOperand()));
    if (!AR || AR->getLoop() != L || !AR->isAffine() ||
        !SE->isLoopInvariant(AR->getStart(), L) ||
        isa<SCEVConstant>(AR->getStart()))
      continue;
    const auto *Step = dyn_cast<SCEVConstant>(AR->getStepRecurrence(*SE));
    if (Step && Step->getAPInt() == Ty->getIntegerBitWidth() / 8)
      return TargetTransformInfo::AMK_PostIndexed;
  }
  return TargetTransformInfo::AMK_None;
}

bool SHTTIImpl::isProfitableLSRChainElement(Instruction *I) const {
  if (!OptSize)
    return false;
  // SH's narrow displacement forms constrain the transferred value to R0.
  // Preserve existing narrow-memory chains so LSR does not save an induction
  // register by rebuilding every address around a shared integer offset.
  Type *AccessTy = nullptr;
  if (auto *Load = dyn_cast<LoadInst>(I))
    AccessTy = Load->getType();
  else if (auto *Store = dyn_cast<StoreInst>(I))
    AccessTy = Store->getValueOperand()->getType();
  return AccessTy && (AccessTy->isIntegerTy(8) || AccessTy->isIntegerTy(16));
}
