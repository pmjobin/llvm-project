//===-- SHPrepareIR.cpp - Prepare IR for SH code generation --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SH.h"
#include "SHIRValidation.h"
#include "SHSubtarget.h"
#include "SHTargetMachine.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IntrinsicsSH.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/Pass.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Transforms/Utils/Local.h"

using namespace llvm;

#define DEBUG_TYPE "sh-prepare-ir"

namespace {

static void diagnoseUnsupported(Function &F, const Instruction &I,
                                const Twine &Message) {
  F.getContext().diagnose(
      DiagnosticInfoUnsupported(F, Message.str(), I.getDebugLoc()));
}

// Native SelectionDAGBuilder owns ordinary aggregate memory. Its field chains
// are parallel even for a volatile aggregate, so retain the established ordered
// scalar accesses for that case until the native builder can express this
// target contract without a second aggregate layout walk.
static Value *getAggregateByteAddress(IRBuilder<> &Builder, Value *Base,
                                      uint64_t Offset) {
  return Builder.CreateConstGEP1_64(Builder.getInt8Ty(), Base, Offset);
}

static Value *loadAggregateScalar(IRBuilder<> &Builder, Type *Ty, Value *Base,
                                  uint64_t Offset, Align BaseAlign,
                                  bool IsVolatile) {
  Align Alignment = commonAlignment(BaseAlign, Offset);
  Value *Address = getAggregateByteAddress(Builder, Base, Offset);
  LoadInst *Load = Builder.CreateAlignedLoad(Ty, Address, Alignment);
  Load->setVolatile(IsVolatile);
  return Load;
}

static void storeAggregateScalar(IRBuilder<> &Builder, Value *StoredValue,
                                 Value *Base, uint64_t Offset, Align BaseAlign,
                                 bool IsVolatile) {
  Align Alignment = commonAlignment(BaseAlign, Offset);
  Value *Address = getAggregateByteAddress(Builder, Base, Offset);
  StoreInst *Store =
      Builder.CreateAlignedStore(StoredValue, Address, Alignment);
  Store->setVolatile(IsVolatile);
}

static void loadAggregateElements(IRBuilder<> &Builder, Type *Ty, Value *Base,
                                  uint64_t Offset, Align BaseAlign,
                                  bool IsVolatile, const DataLayout &DL,
                                  SmallVectorImpl<unsigned> &Indices,
                                  Value *&Result) {
  if (auto *StructTy = dyn_cast<StructType>(Ty)) {
    const StructLayout *Layout = DL.getStructLayout(StructTy);
    for (unsigned I = 0; I != StructTy->getNumElements(); ++I) {
      Indices.push_back(I);
      loadAggregateElements(Builder, StructTy->getElementType(I), Base,
                            Offset + Layout->getElementOffset(I), BaseAlign,
                            IsVolatile, DL, Indices, Result);
      Indices.pop_back();
    }
    return;
  }
  if (const auto *ArrayTy = dyn_cast<ArrayType>(Ty)) {
    uint64_t Stride =
        DL.getTypeAllocSize(ArrayTy->getElementType()).getFixedValue();
    for (uint64_t I = 0; I != ArrayTy->getNumElements(); ++I) {
      Indices.push_back(I);
      loadAggregateElements(Builder, ArrayTy->getElementType(), Base,
                            Offset + I * Stride, BaseAlign, IsVolatile, DL,
                            Indices, Result);
      Indices.pop_back();
    }
    return;
  }
  Value *Element =
      loadAggregateScalar(Builder, Ty, Base, Offset, BaseAlign, IsVolatile);
  Result = Builder.CreateInsertValue(Result, Element, Indices);
}

static void storeAggregateElements(IRBuilder<> &Builder, Type *Ty,
                                   Value *StoredValue, Value *Base,
                                   uint64_t Offset, Align BaseAlign,
                                   bool IsVolatile, const DataLayout &DL,
                                   SmallVectorImpl<unsigned> &Indices) {
  if (auto *StructTy = dyn_cast<StructType>(Ty)) {
    const StructLayout *Layout = DL.getStructLayout(StructTy);
    for (unsigned I = 0; I != StructTy->getNumElements(); ++I) {
      Indices.push_back(I);
      storeAggregateElements(Builder, StructTy->getElementType(I), StoredValue,
                             Base, Offset + Layout->getElementOffset(I),
                             BaseAlign, IsVolatile, DL, Indices);
      Indices.pop_back();
    }
    return;
  }
  if (const auto *ArrayTy = dyn_cast<ArrayType>(Ty)) {
    uint64_t Stride =
        DL.getTypeAllocSize(ArrayTy->getElementType()).getFixedValue();
    for (uint64_t I = 0; I != ArrayTy->getNumElements(); ++I) {
      Indices.push_back(I);
      storeAggregateElements(Builder, ArrayTy->getElementType(), StoredValue,
                             Base, Offset + I * Stride, BaseAlign, IsVolatile,
                             DL, Indices);
      Indices.pop_back();
    }
    return;
  }
  Value *Element = Builder.CreateExtractValue(StoredValue, Indices);
  storeAggregateScalar(Builder, Element, Base, Offset, BaseAlign, IsVolatile);
}

static bool lowerVolatileAggregateMemory(Function &F) {
  SmallVector<Instruction *, 8> Worklist;
  for (BasicBlock &BB : F)
    for (Instruction &I : BB)
      if ((isa<LoadInst>(I) && cast<LoadInst>(I).isVolatile() &&
           I.getType()->isAggregateType()) ||
          (isa<StoreInst>(I) && cast<StoreInst>(I).isVolatile() &&
           cast<StoreInst>(I).getValueOperand()->getType()->isAggregateType()))
        Worklist.push_back(&I);

  bool Changed = false;
  const DataLayout &DL = F.getDataLayout();
  for (Instruction *I : Worklist) {
    Type *Ty = isa<LoadInst>(I)
                   ? I->getType()
                   : cast<StoreInst>(I)->getValueOperand()->getType();
    if (!isSupportedSHAggregateType(Ty))
      continue;

    IRBuilder<> Builder(I);
    Builder.SetCurrentDebugLocation(I->getDebugLoc());
    SmallVector<unsigned, 8> Indices;
    if (auto *Load = dyn_cast<LoadInst>(I)) {
      Value *Result = PoisonValue::get(Ty);
      loadAggregateElements(Builder, Ty, Load->getPointerOperand(), 0,
                            Load->getAlign(), Load->isVolatile(), DL, Indices,
                            Result);
      Load->replaceAllUsesWith(Result);
    } else {
      auto *Store = cast<StoreInst>(I);
      storeAggregateElements(Builder, Ty, Store->getValueOperand(),
                             Store->getPointerOperand(), 0, Store->getAlign(),
                             Store->isVolatile(), DL, Indices);
    }
    I->eraseFromParent();
    Changed = true;
  }
  return Changed;
}

static bool isKnownSH32BitMemoryLength(const Value *Length) {
  if (const auto *Extend = dyn_cast<ZExtInst>(Length))
    return Extend->getSrcTy()->getIntegerBitWidth() <= 32;
  if (const auto *And = dyn_cast<BinaryOperator>(Length);
      And && And->getOpcode() == Instruction::And)
    for (const Value *Operand : And->operands())
      if (const auto *Mask = dyn_cast<ConstantInt>(Operand);
          Mask && Mask->getValue().getActiveBits() <= 32)
        return true;
  return false;
}

static Value *getSH32BitMemoryLength(IRBuilder<> &Builder, Value *Length) {
  if (const auto *ConstantLength = dyn_cast<ConstantInt>(Length))
    return Builder.getInt32(ConstantLength->getZExtValue());
  if (const auto *Extend = dyn_cast<ZExtInst>(Length))
    return Builder.CreateZExtOrTrunc(Extend->getOperand(0),
                                     Builder.getInt32Ty());
  return Builder.CreateTrunc(Length, Builder.getInt32Ty());
}

static bool canonicalizeSHMemoryIntrinsicLengths(Function &F) {
  SmallVector<MemIntrinsic *, 8> Worklist;
  for (BasicBlock &BB : F)
    for (Instruction &I : BB)
      if (auto *MI = dyn_cast<MemIntrinsic>(&I)) {
        Intrinsic::ID ID = MI->getIntrinsicID();
        if ((ID == Intrinsic::memcpy || ID == Intrinsic::memmove ||
             ID == Intrinsic::memset) &&
            MI->getLength()->getType()->isIntegerTy(64))
          Worklist.push_back(MI);
      }

  for (MemIntrinsic *MI : Worklist) {
    Value *Length = MI->getLength();
    const auto *ConstantLength = dyn_cast<ConstantInt>(Length);
    if (ConstantLength && ConstantLength->getValue().getActiveBits() > 32) {
      diagnoseUnsupported(
          F, *MI, "SH memory intrinsic size exceeds the 32-bit address space");
      MI->eraseFromParent();
      continue;
    }
    if (!ConstantLength && !isKnownSH32BitMemoryLength(Length)) {
      diagnoseUnsupported(
          F, *MI, "SH i64 memory intrinsic size is not provably 32-bit");
      MI->eraseFromParent();
      continue;
    }

    IRBuilder<> Builder(MI);
    Builder.SetCurrentDebugLocation(MI->getDebugLoc());
    Value *Length32 = getSH32BitMemoryLength(Builder, Length);
    CallInst *Replacement;
    if (auto *Set = dyn_cast<MemSetInst>(MI))
      Replacement = Builder.CreateMemSet(
          Set->getRawDest(), Set->getValue(), Length32, Set->getDestAlign(),
          Set->isVolatile(), Set->getAAMetadata());
    else {
      auto *Transfer = cast<AnyMemTransferInst>(MI);
      if (isa<MemCpyInst>(MI))
        Replacement = Builder.CreateMemCpy(
            Transfer->getRawDest(), Transfer->getDestAlign(),
            Transfer->getRawSource(), Transfer->getSourceAlign(), Length32,
            Transfer->isVolatile(), Transfer->getAAMetadata());
      else
        Replacement = Builder.CreateMemMove(
            Transfer->getRawDest(), Transfer->getDestAlign(),
            Transfer->getRawSource(), Transfer->getSourceAlign(), Length32,
            Transfer->isVolatile(), Transfer->getAAMetadata());
    }
    Replacement->copyMetadata(*MI);
    MI->eraseFromParent();
  }
  return !Worklist.empty();
}

// Recognize an unobservable matrix product consumed immediately by a bounded
// transform. This retains the exact four FTRVs forming the product: it is not
// the numerically different reassociation A * (B * v). The explicit compound
// intrinsic is also available when a caller cannot expose this local pattern.
static bool retainPrivateMatrixProducts(Function &F) {
  SmallVector<CallInst *, 4> Products;
  for (BasicBlock &BB : F)
    for (Instruction &I : BB)
      if (auto *Call = dyn_cast<CallInst>(&I);
          Call && Call->getIntrinsicID() == Intrinsic::sh_ftrv4)
        Products.push_back(Call);

  bool Changed = false;
  for (CallInst *Product : Products) {
    // A previous fusion may have removed this prospective consumer.
    if (!Product || Product->hasOperandBundles() || Product->isMustTailCall() ||
        Product->isConvergent() || Product->cannotDuplicate())
      continue;
    auto *Storage =
        dyn_cast<AllocaInst>(Product->getArgOperand(0)->stripPointerCasts());
    if (!Storage || !Storage->isStaticAlloca() ||
        Storage->getAllocationSize(F.getDataLayout()) != TypeSize::getFixed(64))
      continue;
    Instruction *Next = Product->getNextNode();
    while (Next && (isa<DbgInfoIntrinsic>(Next) ||
                    isa<GetElementPtrInst>(Next) || isa<BitCastInst>(Next)))
      Next = Next->getNextNode();
    auto *Consumer = dyn_cast_or_null<CallInst>(Next);
    if (!Consumer || Consumer->hasOperandBundles() ||
        Consumer->isMustTailCall() || Consumer->isConvergent() ||
        Consumer->cannotDuplicate())
      continue;
    bool Array = Consumer->getIntrinsicID() == Intrinsic::sh_ftrv_array;
    if (!Array && Consumer->getIntrinsicID() != Intrinsic::sh_ftrv4)
      continue;
    unsigned MatrixArg = Array ? 3 : 2;
    if (Consumer->getArgOperand(MatrixArg)->stripPointerCasts() != Storage)
      continue;
    auto *Count = Array ? dyn_cast<ConstantInt>(Consumer->getArgOperand(2))
                        : ConstantInt::get(Type::getInt32Ty(F.getContext()), 4);
    // Zero suppresses all compound FP work; the original product executes
    // even when the second operation's count is zero. Do not discard it.
    if (!Count || Count->isZero())
      continue;

    SmallVector<Value *, 8> Worklist{Storage};
    SmallPtrSet<Value *, 8> Seen;
    bool Private = true;
    while (!Worklist.empty() && Private) {
      Value *V = Worklist.pop_back_val();
      if (!Seen.insert(V).second)
        continue;
      for (User *U : V->users()) {
        if (U == Product || U == Consumer) {
          auto *Call = cast<CallInst>(U);
          unsigned Allowed = U == Product ? 0 : MatrixArg;
          for (unsigned I = 0; I != Call->arg_size(); ++I)
            if (I != Allowed && Call->getArgOperand(I) == V)
              Private = false;
        } else if ((isa<GetElementPtrInst>(U) || isa<BitCastInst>(U)) &&
                   U->stripPointerCasts() == Storage) {
          Worklist.push_back(U);
        } else if (auto *II = dyn_cast<IntrinsicInst>(U);
                   !II || (II->getIntrinsicID() != Intrinsic::lifetime_start &&
                           II->getIntrinsicID() != Intrinsic::lifetime_end &&
                           !isa<DbgInfoIntrinsic>(II))) {
          Private = false;
        }
      }
    }
    if (!Private)
      continue;

    IRBuilder<> Builder(Consumer);
    auto *Combined = cast<CallInst>(Builder.CreateIntrinsic(
        Intrinsic::sh_ftrv_compose, {},
        {Consumer->getArgOperand(0), Consumer->getArgOperand(1), Count,
         Product->getArgOperand(2), Product->getArgOperand(1)}));
    for (unsigned Arg : {0U, 1U})
      if (MaybeAlign Alignment = Consumer->getParamAlign(Arg))
        Combined->addParamAttr(
            Arg, Attribute::getWithAlignment(F.getContext(), *Alignment));
    for (unsigned Arg : {1U, 2U})
      if (MaybeAlign Alignment = Product->getParamAlign(Arg))
        Combined->addParamAttr(
            5 - Arg, Attribute::getWithAlignment(F.getContext(), *Alignment));
    for (CallInst *&Candidate : Products)
      if (Candidate == Consumer)
        Candidate = nullptr;
    Consumer->eraseFromParent();
    Product->eraseFromParent();
    SmallVector<WeakTrackingVH, 8> Dead;
    for (Value *V : Seen) {
      for (User *U : make_early_inc_range(V->users()))
        if (auto *II = dyn_cast<IntrinsicInst>(U);
            II && (II->getIntrinsicID() == Intrinsic::lifetime_start ||
                   II->getIntrinsicID() == Intrinsic::lifetime_end))
          II->eraseFromParent();
      Dead.push_back(V);
    }
    RecursivelyDeleteTriviallyDeadInstructionsPermissive(Dead);
    Changed = true;
  }
  return Changed;
}

// Two adjacent scalar transforms can share the existing bounded array owner.
// Admit only disjoint noalias parameter objects and consecutive vector
// accesses; a call, state access, asm, mutation, or control-flow edge stops the
// pattern. This avoids inferring residency after GRAPHICS_END restores the XF
// tail.
static bool fuseAdjacentMatrixTransforms(Function &F) {
  SmallVector<WeakTrackingVH, 8> Candidates;
  for (BasicBlock &BB : F)
    for (Instruction &I : BB)
      if (auto *Call = dyn_cast<CallInst>(&I);
          Call && Call->getIntrinsicID() == Intrinsic::sh_ftrv)
        Candidates.push_back(Call);

  auto Next = [](Instruction *I) {
    do {
      I = I->getNextNode();
    } while (I && (isa<GetElementPtrInst>(I) || isa<BitCastInst>(I) ||
                   isa<DbgInfoIntrinsic>(I)));
    return I;
  };
  auto Previous = [](Instruction *I) {
    do {
      I = I->getPrevNode();
    } while (I && (isa<GetElementPtrInst>(I) || isa<BitCastInst>(I) ||
                   isa<DbgInfoIntrinsic>(I)));
    return I;
  };
  auto OrdinaryCall = [](CallInst *Call) {
    return Call && Call->getIntrinsicID() == Intrinsic::sh_ftrv &&
           !Call->hasOperandBundles() && !Call->isMustTailCall() &&
           !Call->isConvergent() && !Call->cannotDuplicate();
  };
  const DataLayout &DL = F.getDataLayout();
  auto Parameter = [&](Value *Pointer, int64_t &Offset) {
    return dyn_cast<Argument>(
        GetPointerBaseWithConstantOffset(Pointer, Offset, DL));
  };
  auto Consecutive = [&](Value *First, Value *Second) -> Argument * {
    int64_t FirstOffset, SecondOffset;
    Argument *Base = Parameter(First, FirstOffset);
    if (!Base || Base != Parameter(Second, SecondOffset) ||
        !isInt<32>(FirstOffset) || !isInt<32>(SecondOffset) ||
        SecondOffset != FirstOffset + 16)
      return nullptr;
    return Base;
  };

  bool Changed = false;
  for (WeakTrackingVH &Candidate : Candidates) {
    auto *First = dyn_cast_or_null<CallInst>(static_cast<Value *>(Candidate));
    if (!OrdinaryCall(First))
      continue;
    auto *Load0 = dyn_cast<LoadInst>(First->getArgOperand(1));
    auto *Store0 = dyn_cast_or_null<StoreInst>(Next(First));
    if (!Load0 || Previous(First) != Load0 || !Load0->isSimple() ||
        !Load0->hasOneUse() || !First->hasOneUse() || !Store0 ||
        !Store0->isSimple() || Store0->getValueOperand() != First)
      continue;
    auto *Load1 = dyn_cast_or_null<LoadInst>(Next(Store0));
    auto *Second = Load1 ? dyn_cast_or_null<CallInst>(Next(Load1)) : nullptr;
    auto *Store1 = Second ? dyn_cast_or_null<StoreInst>(Next(Second)) : nullptr;
    if (!Load1 || !Load1->isSimple() || !Load1->hasOneUse() ||
        !OrdinaryCall(Second) || Second->getArgOperand(1) != Load1 ||
        !Second->hasOneUse() || !Store1 || !Store1->isSimple() ||
        Store1->getValueOperand() != Second ||
        First->getArgOperand(0)->stripPointerCasts() !=
            Second->getArgOperand(0)->stripPointerCasts())
      continue;
    // The counted intrinsic requires AS0 and at least four-byte alignment.
    // Leave weaker accesses to ordinary vector legalization, and unsupported
    // address spaces to the IR validator rather than creating an invalid call.
    if (Load0->getAlign() < Align(4) || Load1->getAlign() < Align(4) ||
        Store0->getAlign() < Align(4) || Store1->getAlign() < Align(4) ||
        Load0->getPointerAddressSpace() || Load1->getPointerAddressSpace() ||
        Store0->getPointerAddressSpace() || Store1->getPointerAddressSpace())
      continue;
    Argument *Dst =
        Consecutive(Store0->getPointerOperand(), Store1->getPointerOperand());
    Argument *Src =
        Consecutive(Load0->getPointerOperand(), Load1->getPointerOperand());
    int64_t MatrixOffset;
    Argument *Matrix = Parameter(First->getArgOperand(0), MatrixOffset);
    if (!Dst || !Src || !Matrix || Dst == Src || Dst == Matrix ||
        Src == Matrix || !Dst->hasNoAliasAttr() || !Src->hasNoAliasAttr() ||
        !Matrix->hasNoAliasAttr())
      continue;

    // Insert after all pointer definitions. The removed operations are the
    // only intervening memory/FP effects, and their parameter objects are
    // disjoint. Count two has no zero-count speculation or changed FP order.
    IRBuilder<> Builder(Store1);
    auto *Combined = cast<CallInst>(Builder.CreateIntrinsic(
        Intrinsic::sh_ftrv_array, {},
        {Store0->getPointerOperand(), Load0->getPointerOperand(),
         Builder.getInt32(2), First->getArgOperand(0)}));
    Combined->setDebugLoc(First->getDebugLoc());
    for (auto [Index, Alignment] :
         {std::pair{0U, std::min(Store0->getAlign(), Store1->getAlign())},
          std::pair{1U, std::min(Load0->getAlign(), Load1->getAlign())}})
      Combined->addParamAttr(
          Index, Attribute::getWithAlignment(F.getContext(), Alignment));
    if (MaybeAlign Alignment = First->getParamAlign(0))
      Combined->addParamAttr(
          3, Attribute::getWithAlignment(F.getContext(), *Alignment));
    Store1->eraseFromParent();
    Store0->eraseFromParent();
    Second->eraseFromParent();
    First->eraseFromParent();
    Load1->eraseFromParent();
    Load0->eraseFromParent();
    Changed = true;
  }
  return Changed;
}

static bool prepareSHIR(Function &F) {
  bool Changed = lowerVolatileAggregateMemory(F);
  Changed |= canonicalizeSHMemoryIntrinsicLengths(F);
  Changed |= retainPrivateMatrixProducts(F);
  Changed |= fuseAdjacentMatrixTransforms(F);
  return Changed;
}

class SHPrepareIRLegacy : public FunctionPass {
public:
  static char ID;

  SHPrepareIRLegacy() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override { return prepareSHIR(F); }

  StringRef getPassName() const override { return "SH prepare IR"; }
};

} // namespace

char SHPrepareIRLegacy::ID = 0;

INITIALIZE_PASS(SHPrepareIRLegacy, DEBUG_TYPE, "SH prepare IR", false, false)

FunctionPass *llvm::createSHPrepareIRLegacyPass() {
  return new SHPrepareIRLegacy();
}

PreservedAnalyses SHPrepareIRPass::run(Function &F,
                                       FunctionAnalysisManager &FAM) {
  bool Changed = prepareSHIR(F);
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PreservedAnalyses SHPrepareIRModulePass::run(Module &M,
                                             ModuleAnalysisManager &MAM) {
  bool Changed = false;
  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    Changed |= prepareSHIR(F);
    if (!validateSHIR(F, TM.getSubtargetImpl(F)->getABI()) ||
        !validateSHConfiguration(F, TM)) {
      F.deleteBody();
      Changed = true;
    }
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
