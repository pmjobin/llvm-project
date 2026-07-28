//===-- SHLowerI64StackAlign.cpp - Lower i64 stack alignment -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SH.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"

using namespace llvm;

#define DEBUG_TYPE "sh-lower-i64-stack-align"

namespace {

static void lowerI64StackAccessAlignment(Value &Pointer,
                                         SmallPtrSetImpl<Value *> &Visited) {
  if (!Visited.insert(&Pointer).second)
    return;
  for (User *Use : Pointer.users()) {
    if (auto *Cast = dyn_cast<BitCastInst>(Use)) {
      lowerI64StackAccessAlignment(*Cast, Visited);
      continue;
    }
    if (auto *GEP = dyn_cast<GetElementPtrInst>(Use)) {
      lowerI64StackAccessAlignment(*GEP, Visited);
      continue;
    }
    if (auto *Load = dyn_cast<LoadInst>(Use)) {
      if (Load->getPointerOperand() == &Pointer && Load->getAlign() > Align(4))
        Load->setAlignment(Align(4));
      continue;
    }
    if (auto *Store = dyn_cast<StoreInst>(Use))
      if (Store->getPointerOperand() == &Pointer &&
          Store->getAlign() > Align(4))
        Store->setAlignment(Align(4));
  }
}

static bool lowerI64StackAlignment(Function &F) {
  // The GNU-compatible scalar ABI guarantees four-byte stack alignment.
  // SH accepts i64 stack accesses at that alignment and this backend rejects
  // every use that could observe or expose a local stack address. Keeping an
  // explicit eight-byte alloca alignment would therefore request general
  // stack realignment without changing any supported program's behavior.
  bool Changed = false;
  for (BasicBlock &BB : F)
    for (Instruction &I : BB)
      if (auto *Alloca = dyn_cast<AllocaInst>(&I);
          Alloca && Alloca->isStaticAlloca() &&
          Alloca->getAllocatedType()->isIntegerTy(64) &&
          Alloca->getAlign() == Align(8)) {
        Alloca->setAlignment(Align(4));
        SmallPtrSet<Value *, 8> Visited;
        lowerI64StackAccessAlignment(*Alloca, Visited);
        Changed = true;
      }
  return Changed;
}

static bool prepareSHIR(Function &F) {
  bool Changed = lowerI64StackAlignment(F);
  validateSHIR(F);
  return Changed;
}

class SHLowerI64StackAlignLegacy : public FunctionPass {
public:
  static char ID;

  SHLowerI64StackAlignLegacy() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override { return prepareSHIR(F); }

  StringRef getPassName() const override {
    return "SH lower i64 stack alignment";
  }
};

} // namespace

char SHLowerI64StackAlignLegacy::ID = 0;

INITIALIZE_PASS(SHLowerI64StackAlignLegacy, DEBUG_TYPE,
                "SH lower i64 stack alignment", false, false)

FunctionPass *llvm::createSHLowerI64StackAlignLegacyPass() {
  return new SHLowerI64StackAlignLegacy();
}

PreservedAnalyses SHLowerI64StackAlignPass::run(Function &F,
                                                FunctionAnalysisManager &FAM) {
  if (!prepareSHIR(F))
    return PreservedAnalyses::all();
  PreservedAnalyses PA;
  PA.preserveSet<CFGAnalyses>();
  return PA;
}
