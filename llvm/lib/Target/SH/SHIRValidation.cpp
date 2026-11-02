//===-- SHIRValidation.cpp - Validate SH IR invariants -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHIRValidation.h"
#include "SH.h"
#include "SHSubtarget.h"
#include "SHTargetMachine.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/EHPersonalities.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicsSH.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

namespace {

static bool diagnoseUnsupported(const Function &F, const Twine &Message,
                                const Instruction *I = nullptr) {
  F.getContext().diagnose(
      DiagnosticInfoUnsupported(F, Message, I ? I->getDebugLoc() : DebugLoc()));
  return false;
}

class SHAtomicValidateLegacy : public FunctionPass {
  SHTargetMachine &TM;

public:
  static char ID;

  explicit SHAtomicValidateLegacy(SHTargetMachine &TM)
      : FunctionPass(ID), TM(TM) {}

  bool runOnFunction(Function &F) override {
    if (validateSHAtomics(F, TM.getSubtargetImpl(F)->hasDoubleFPU()))
      return false;
    replaceSHFunctionWithUnreachable(F);
    return true;
  }

  StringRef getPassName() const override {
    return "SH validate atomic operations";
  }
};

class SHIRValidationLegacy : public ModulePass {
  SHTargetMachine &TM;

public:
  static char ID;

  explicit SHIRValidationLegacy(SHTargetMachine &TM) : ModulePass(ID), TM(TM) {}

  bool runOnModule(Module &M) override {
    bool Changed = false;
    for (Function &F : M) {
      if (F.isDeclaration() ||
          (validateSHIR(F, TM.getSubtargetImpl(F)->getABI()) &&
           validateSHConfiguration(F, TM)))
        continue;
      F.deleteBody();
      Changed = true;
    }
    return Changed;
  }

  StringRef getPassName() const override { return "SH validate prepared IR"; }
};

} // namespace

void llvm::replaceSHFunctionWithUnreachable(Function &F) {
  F.deleteBody();
  BasicBlock *Entry = BasicBlock::Create(F.getContext(), "entry", &F);
  new UnreachableInst(F.getContext(), Entry);
}

bool llvm::isSupportedSHCallingConvention(CallingConv::ID CallConv) {
  return CallConv == CallingConv::C || CallConv == CallingConv::Fast;
}

bool llvm::validateSHConfiguration(const Function &F,
                                   const SHTargetMachine &TM) {
  Attribute HandlerAttr = F.getFnAttribute("sh-interrupt-handler");
  if (!HandlerAttr.isValid())
    return true;
  const SHSubtarget &ST = *TM.getSubtargetImpl(F);
  if (F.hasFnAttribute("sh-nosave-low-regs") && !ST.hasBankedGPRs())
    return diagnoseUnsupported(
        F, "SH nosave_low_regs requires hardware-banked r0-r7 registers");
  if (F.hasFnAttribute("sh-resbank") && !ST.hasSH2A())
    return diagnoseUnsupported(F, "SH resbank requires an SH2A target");
  if (F.hasFnAttribute("sh-sp-switch") && TM.isPositionIndependent())
    return diagnoseUnsupported(
        F, "SH sp_switch is unsupported for position-independent code");
  for (const BasicBlock &BB : F)
    for (const Instruction &I : BB)
      if (const auto *Alloca = dyn_cast<AllocaInst>(&I);
          Alloca && Alloca->getAlign() > Align(4))
        return diagnoseUnsupported(
            F, "SH core hardware handlers do not support stack realignment",
            Alloca);
  return true;
}

void llvm::requireSupportedSHCallingConvention(CallingConv::ID CallConv) {
  assert(isSupportedSHCallingConvention(CallConv) &&
         "unsupported SH calling convention survived IR validation");
}

static bool validateSupportedFastFunction(const Function &F) {
  if (F.getCallingConv() != CallingConv::Fast)
    return true;
  if (!F.hasLocalLinkage())
    return diagnoseUnsupported(
        F, "SH fast calling convention requires internal or private linkage");
  if (F.isVarArg())
    return diagnoseUnsupported(
        F, "SH fast calling convention does not support variadic functions");
  for (const User *FunctionUser : F.users()) {
    if (const auto *Call = dyn_cast<CallBase>(FunctionUser)) {
      if (Call->getCalledOperand()->stripPointerCasts() == &F)
        continue;
    }
    const auto *Expr = dyn_cast<ConstantExpr>(FunctionUser);
    if (Expr && Expr->getOpcode() == Instruction::PtrToInt &&
        llvm::all_of(Expr->users(), [Expr](const User *MetadataUser) {
          const auto *Store = dyn_cast<StoreInst>(MetadataUser);
          return Store && Store->getValueOperand() == Expr;
        }))
      continue;
    return diagnoseUnsupported(
        F, "SH fast calling convention functions may only be used by direct "
           "calls or stored as integer metadata");
  }
  return true;
}

static bool validateSupportedCall(const CallBase &Call) {
  const Function &F = *Call.getFunction();
  if (!isSupportedSHCallingConvention(Call.getCallingConv()))
    return diagnoseUnsupported(
        F, "SH only supports the C and fast calling conventions", &Call);
  const Function *Callee = Call.getCalledFunction();
  const Value *CalledValue = Call.getCalledOperand()->stripPointerCasts();
  SmallPtrSet<const GlobalAlias *, 4> VisitedAliases;
  while (const auto *Alias = dyn_cast<GlobalAlias>(CalledValue)) {
    if (!VisitedAliases.insert(Alias).second)
      break;
    CalledValue = Alias->getAliaseeObject();
  }
  const Function *ResolvedCallee = dyn_cast_or_null<Function>(CalledValue);
  if (Call.getCallingConv() == CallingConv::Fast) {
    if (Call.getFunctionType()->isVarArg())
      return diagnoseUnsupported(
          F, "SH fast calling convention does not support variadic calls",
          &Call);
    if (!Callee)
      return diagnoseUnsupported(F, "SH indirect fast calls are not supported",
                                 &Call);
    if (!Callee->hasLocalLinkage())
      return diagnoseUnsupported(
          F, "SH fast calling convention requires internal or private linkage",
          &Call);
  }
  if (Callee && Callee->getCallingConv() != Call.getCallingConv())
    return diagnoseUnsupported(
        F, "SH call site and callee calling conventions must match", &Call);
  if (ResolvedCallee && ResolvedCallee->hasFnAttribute("sh-interrupt-handler"))
    return diagnoseUnsupported(
        F, "SH hardware handlers cannot be called as ordinary functions",
        &Call);
  return true;
}

bool llvm::isSupportedSHVectorType(Type *Ty) {
  const auto *V = dyn_cast<FixedVectorType>(Ty);
  return V && V->getElementType()->isFloatTy() &&
         (V->getNumElements() == 2 || V->getNumElements() == 4);
}

static bool isSupportedSHABIScalarType(Type *Ty) {
  const auto *Integer = dyn_cast<IntegerType>(Ty);
  return (Integer && Integer->getBitWidth() <= 64) || Ty->isFloatTy() ||
         Ty->isDoubleTy() ||
         (Ty->isPointerTy() && Ty->getPointerAddressSpace() == 0);
}

static bool isSupportedSHStackType(Type *Ty) {
  if (isSupportedSHABIScalarType(Ty) || isSupportedSHVectorType(Ty))
    return true;
  if (const auto *ArrayTy = dyn_cast<ArrayType>(Ty))
    return isSupportedSHStackType(ArrayTy->getElementType());
  if (const auto *StructTy = dyn_cast<StructType>(Ty)) {
    if (StructTy->isOpaque())
      return false;
    for (Type *ElementTy : StructTy->elements())
      if (!isSupportedSHStackType(ElementTy))
        return false;
    return true;
  }
  return false;
}

bool llvm::isSupportedSHAggregateType(Type *Ty) {
  return Ty->isAggregateType() && isSupportedSHStackType(Ty);
}

bool llvm::isSupportedSHValueType(Type *Ty) {
  return isSupportedSHABIScalarType(Ty) || isSupportedSHVectorType(Ty) ||
         isSupportedSHAggregateType(Ty);
}

bool llvm::isSupportedSHVarArgType(Type *Ty) {
  return isSupportedSHVectorType(Ty) || Ty->isIntegerTy(32) ||
         Ty->isIntegerTy(64) || Ty->isDoubleTy() ||
         (Ty->isPointerTy() && Ty->getPointerAddressSpace() == 0) ||
         isSupportedSHAggregateType(Ty);
}

uint64_t llvm::getFixedSHTypeAllocSize(const DataLayout &DL, Type *Ty) {
  assert(Ty->isSized() && "unsized aggregate survived SH IR validation");
  TypeSize Size = DL.getTypeAllocSize(Ty);
  assert(!Size.isScalable() && "scalable aggregate survived SH IR validation");
  return Size.getFixedValue();
}

bool llvm::isDirectSHAggregateReturn(const DataLayout &DL, Type *Ty) {
  if (!isSupportedSHAggregateType(Ty))
    return false;
  uint64_t Size = getFixedSHTypeAllocSize(DL, Ty);
  switch (Size) {
  default:
    return false;
  case 1:
  case 2:
  case 4:
  case 8:
    return true;
  }
}

void llvm::requireSupportedSHByValType(const DataLayout &DL, Type *Ty,
                                       Align Alignment) {
  assert(Ty && isSupportedSHAggregateType(Ty) &&
         "unsupported byval type survived SH IR validation");
  assert(getFixedSHTypeAllocSize(DL, Ty) != 0 &&
         "zero-sized byval survived SH IR validation");
  assert(Alignment.value() <= SHMaxStackAlignment &&
         "over-aligned byval survived SH IR validation");
}

static bool validateSupportedSHByValType(const Function &F,
                                         const DataLayout &DL, Type *Ty,
                                         Align Alignment) {
  if (!Ty || !isSupportedSHAggregateType(Ty))
    return diagnoseUnsupported(
        F, "SH byval requires a fixed aggregate containing only integers, "
           "address-space-zero pointers, f32, and f64");
  if (getFixedSHTypeAllocSize(DL, Ty) == 0)
    return diagnoseUnsupported(
        F, "SH zero-sized byval arguments are not supported");
  if (Alignment.value() > SHMaxStackAlignment)
    return diagnoseUnsupported(F, Twine("SH byval alignment cannot exceed ") +
                                      Twine(SHMaxStackAlignment) + " bytes");
  return true;
}

static bool validateSupportedSHSRetType(const Function &F, const DataLayout &DL,
                                        Type *Ty) {
  if (!Ty || (!isSupportedSHAggregateType(Ty) && !isSupportedSHVectorType(Ty)))
    return diagnoseUnsupported(
        F, "SH sret requires a fixed aggregate containing only integers, "
           "address-space-zero pointers, f32, and f64");
  if (getFixedSHTypeAllocSize(DL, Ty) == 0)
    return diagnoseUnsupported(F,
                               "SH zero-sized sret results are not supported");
  return true;
}

static bool isSHRuntimePointer(Type *Ty) {
  const auto *PointerTy = dyn_cast<PointerType>(Ty);
  return PointerTy && PointerTy->getAddressSpace() == 0;
}

static bool isSHAtomicOrderOperand(const Value *Value) {
  const auto *Order = dyn_cast<ConstantInt>(Value);
  if (!Order || !Order->getType()->isIntegerTy(32))
    return false;
  switch (static_cast<AtomicOrderingCABI>(Order->getZExtValue())) {
  case AtomicOrderingCABI::relaxed:
  case AtomicOrderingCABI::acquire:
  case AtomicOrderingCABI::release:
  case AtomicOrderingCABI::acq_rel:
  case AtomicOrderingCABI::seq_cst:
    return true;
  case AtomicOrderingCABI::consume:
    return false;
  }
  return false;
}

SHAtomicRuntimeCallKind llvm::getSHAtomicRuntimeCallKind(const CallBase &Call) {
  const Function *Callee = Call.getCalledFunction();
  if (!Callee || Call.getCallingConv() != CallingConv::C)
    return SHAtomicRuntimeCallKind::None;

  StringRef Name = Callee->getName();
  auto IsPointerArg = [&](unsigned Index) {
    return Index < Call.arg_size() &&
           isSHRuntimePointer(Call.getArgOperand(Index)->getType());
  };
  auto IsIntegerArg = [&](unsigned Index, unsigned Width) {
    return Index < Call.arg_size() &&
           Call.getArgOperand(Index)->getType()->isIntegerTy(Width);
  };

  unsigned Width = 0;
  if (Name.ends_with("_1"))
    Width = 8;
  else if (Name.ends_with("_2"))
    Width = 16;
  else if (Name.ends_with("_4"))
    Width = 32;
  else if (Name.ends_with("_8"))
    Width = 64;

  if (Width != 0) {
    StringRef Base = Name.drop_back(2);
    if (Base == "__atomic_load")
      return Call.arg_size() == 2 && IsPointerArg(0) &&
                     isSHAtomicOrderOperand(Call.getArgOperand(1)) &&
                     Call.getType()->isIntegerTy(Width)
                 ? SHAtomicRuntimeCallKind::Operation
                 : SHAtomicRuntimeCallKind::None;
    if (Base == "__atomic_store")
      return Call.arg_size() == 3 && IsPointerArg(0) &&
                     IsIntegerArg(1, Width) &&
                     isSHAtomicOrderOperand(Call.getArgOperand(2)) &&
                     Call.getType()->isVoidTy()
                 ? SHAtomicRuntimeCallKind::Operation
                 : SHAtomicRuntimeCallKind::None;
    if (Base == "__atomic_compare_exchange")
      return Call.arg_size() == 5 && IsPointerArg(0) && IsPointerArg(1) &&
                     IsIntegerArg(2, Width) &&
                     isSHAtomicOrderOperand(Call.getArgOperand(3)) &&
                     isSHAtomicOrderOperand(Call.getArgOperand(4)) &&
                     Call.getType()->isIntegerTy(1) &&
                     Call.hasRetAttr(Attribute::ZExt)
                 ? SHAtomicRuntimeCallKind::CompareExchange
                 : SHAtomicRuntimeCallKind::None;
    if (Base == "__atomic_exchange" || Base == "__atomic_fetch_add" ||
        Base == "__atomic_fetch_sub" || Base == "__atomic_fetch_and" ||
        Base == "__atomic_fetch_or" || Base == "__atomic_fetch_xor" ||
        Base == "__atomic_fetch_nand")
      return Call.arg_size() == 3 && IsPointerArg(0) &&
                     IsIntegerArg(1, Width) &&
                     isSHAtomicOrderOperand(Call.getArgOperand(2)) &&
                     Call.getType()->isIntegerTy(Width)
                 ? SHAtomicRuntimeCallKind::Operation
                 : SHAtomicRuntimeCallKind::None;
    return SHAtomicRuntimeCallKind::None;
  }

  if (Name != "__atomic_load" && Name != "__atomic_store" &&
      Name != "__atomic_exchange" && Name != "__atomic_compare_exchange")
    return SHAtomicRuntimeCallKind::None;
  if (Call.arg_empty())
    return SHAtomicRuntimeCallKind::None;
  const auto *Size = dyn_cast<ConstantInt>(Call.getArgOperand(0));
  if (!Size || !Size->getType()->isIntegerTy(32) ||
      (Size->getZExtValue() != 1 && Size->getZExtValue() != 2 &&
       Size->getZExtValue() != 4 && Size->getZExtValue() != 8) ||
      !IsPointerArg(1))
    return SHAtomicRuntimeCallKind::None;

  if (Name == "__atomic_load" || Name == "__atomic_store")
    return Call.arg_size() == 4 && IsPointerArg(2) &&
                   isSHAtomicOrderOperand(Call.getArgOperand(3)) &&
                   Call.getType()->isVoidTy()
               ? SHAtomicRuntimeCallKind::Operation
               : SHAtomicRuntimeCallKind::None;
  if (Name == "__atomic_exchange")
    return Call.arg_size() == 5 && IsPointerArg(2) && IsPointerArg(3) &&
                   isSHAtomicOrderOperand(Call.getArgOperand(4)) &&
                   Call.getType()->isVoidTy()
               ? SHAtomicRuntimeCallKind::Operation
               : SHAtomicRuntimeCallKind::None;
  return Call.arg_size() == 6 && IsPointerArg(2) && IsPointerArg(3) &&
                 isSHAtomicOrderOperand(Call.getArgOperand(4)) &&
                 isSHAtomicOrderOperand(Call.getArgOperand(5)) &&
                 Call.getType()->isIntegerTy(1) &&
                 Call.hasRetAttr(Attribute::ZExt)
             ? SHAtomicRuntimeCallKind::CompareExchange
             : SHAtomicRuntimeCallKind::None;
}

bool llvm::validateSHAtomics(const Function &F, bool AllowFloating) {
  auto ValidateSupportedScope = [&](SyncScope::ID Scope, const Instruction &I) {
    if (Scope != SyncScope::System && Scope != SyncScope::SingleThread)
      return diagnoseUnsupported(F, "SH synchronization scope is not supported",
                                 &I);
    return true;
  };
  auto ValidateSupportedType = [&](Type *Ty, unsigned PointerAddressSpace,
                                   const Instruction &I) {
    bool Supported = Ty->isIntegerTy(8) || Ty->isIntegerTy(16) ||
                     Ty->isIntegerTy(32) || Ty->isIntegerTy(64) ||
                     (AllowFloating && (Ty->isFloatTy() || Ty->isDoubleTy())) ||
                     (Ty->isPointerTy() && Ty->getPointerAddressSpace() == 0);
    if (!Supported || PointerAddressSpace != 0)
      return diagnoseUnsupported(
          F, "SH generic atomic operation is not supported for this type", &I);
    return true;
  };

  for (const BasicBlock &BB : F) {
    for (const Instruction &I : BB) {
      if (const auto *Fence = dyn_cast<FenceInst>(&I)) {
        if (!ValidateSupportedScope(Fence->getSyncScopeID(), I))
          return false;
        continue;
      }
      if (const auto *Load = dyn_cast<LoadInst>(&I); Load && Load->isAtomic()) {
        if (!ValidateSupportedScope(Load->getSyncScopeID(), I) ||
            !ValidateSupportedType(Load->getType(),
                                   Load->getPointerAddressSpace(), I))
          return false;
        continue;
      }
      if (const auto *Store = dyn_cast<StoreInst>(&I);
          Store && Store->isAtomic()) {
        if (!ValidateSupportedScope(Store->getSyncScopeID(), I) ||
            !ValidateSupportedType(Store->getValueOperand()->getType(),
                                   Store->getPointerAddressSpace(), I))
          return false;
        continue;
      }
      if (const auto *RMW = dyn_cast<AtomicRMWInst>(&I)) {
        if (!ValidateSupportedScope(RMW->getSyncScopeID(), I))
          return false;
        Type *Ty = RMW->getValOperand()->getType();
        if (!ValidateSupportedType(Ty, RMW->getPointerAddressSpace(), I))
          return false;
        if (Ty->isPointerTy() && RMW->getOperation() != AtomicRMWInst::Xchg)
          return diagnoseUnsupported(
              F, "SH generic atomic operation is not supported for this type",
              &I);
        continue;
      }
      if (const auto *CAS = dyn_cast<AtomicCmpXchgInst>(&I)) {
        if (!ValidateSupportedScope(CAS->getSyncScopeID(), I) ||
            !ValidateSupportedType(CAS->getCompareOperand()->getType(),
                                   CAS->getPointerAddressSpace(), I))
          return false;
      }
    }
  }
  return true;
}

static bool isBlockAddressInteger(const Value *V,
                                  SmallPtrSetImpl<const Value *> &Visited) {
  if (!Visited.insert(V).second)
    return false;
  if (const auto *Cast = dyn_cast<PtrToIntInst>(V))
    return isa<BlockAddress>(Cast->getPointerOperand()->stripPointerCasts());
  if (const auto *Phi = dyn_cast<PHINode>(V))
    return llvm::any_of(Phi->incoming_values(), [&](const Value *Incoming) {
      return isBlockAddressInteger(Incoming, Visited);
    });
  return false;
}

static bool validateSHMemoryIntrinsic(const MemIntrinsic &MI) {
  const Function &F = *MI.getFunction();
  Intrinsic::ID ID = MI.getIntrinsicID();
  if (ID != Intrinsic::memcpy && ID != Intrinsic::memmove &&
      ID != Intrinsic::memset)
    return diagnoseUnsupported(
        F, "SH atomic and inline-only memory intrinsics are not supported",
        &MI);
  if (MI.getRawDest()->getType()->getPointerAddressSpace() != 0)
    return diagnoseUnsupported(
        F, "SH memory intrinsics only support address space zero", &MI);
  if (const auto *Transfer = dyn_cast<AnyMemTransferInst>(&MI))
    if (Transfer->getRawSource()->getType()->getPointerAddressSpace() != 0)
      return diagnoseUnsupported(
          F, "SH memory intrinsics only support address space zero", &MI);
  if (!MI.getLength()->getType()->isIntegerTy(32))
    return diagnoseUnsupported(F, "SH memory intrinsic sizes must be i32", &MI);
  if (!MI.isVolatile())
    return true;
  const auto *Size = dyn_cast<ConstantInt>(MI.getLength());
  if (!Size)
    return diagnoseUnsupported(
        F, "SH volatile memory intrinsics require a constant size", &MI);
  uint64_t InlineLimit = ID == Intrinsic::memmove ? 16 : 60;
  if (Size->getZExtValue() > InlineLimit)
    return diagnoseUnsupported(
        F,
        Twine("SH volatile memory intrinsic size cannot exceed ") +
            Twine(InlineLimit) + " bytes",
        &MI);
  return true;
}

static bool isUnsupportedSHEHInstruction(const Instruction &I) {
  return isa<CatchSwitchInst, CatchPadInst, CleanupPadInst, CatchReturnInst,
             CleanupReturnInst>(I);
}

static bool validateSHInlineAsm(const CallBase &Call, const DataLayout &DL) {
  const auto *IA = dyn_cast<InlineAsm>(Call.getCalledOperand());
  if (!IA)
    return true;
  InlineAsm::ConstraintInfoVector Constraints = IA->ParseConstraints();
  unsigned DirectOutputIndex = 0;
  for (const InlineAsm::ConstraintInfo &Constraint : Constraints) {
    if (Constraint.Type != InlineAsm::isOutput || Constraint.isIndirect)
      continue;
    Type *OutputTy = Call.getType();
    if (auto *StructTy = dyn_cast<StructType>(OutputTy)) {
      if (DirectOutputIndex >= StructTy->getNumElements())
        return true;
      OutputTy = StructTy->getElementType(DirectOutputIndex);
    }
    ++DirectOutputIndex;
    if (!Constraint.hasMatchingInput())
      continue;
    unsigned InputIndex = 0;
    for (int I = 0; I != Constraint.MatchingInput; ++I)
      if (Constraints[I].hasArg())
        ++InputIndex;
    if (InputIndex >= Call.arg_size() || !OutputTy->isSized() ||
        !Call.getArgOperand(InputIndex)->getType()->isSized())
      return true;
    Type *InputTy = Call.getArgOperand(InputIndex)->getType();
    TypeSize OutputSize = DL.getTypeStoreSize(OutputTy);
    TypeSize InputSize = DL.getTypeStoreSize(InputTy);
    if (OutputSize == InputSize)
      continue;
    if (OutputTy->isIntegerTy() && InputTy->isIntegerTy() &&
        OutputSize.getKnownMinValue() <= 4 && InputSize.getKnownMinValue() <= 4)
      continue;
    return diagnoseUnsupported(*Call.getFunction(),
                               "SH tied inline assembly operands require "
                               "compatible integer GPR values",
                               &Call);
  }
  return true;
}

static bool containsNonzeroSHAddressSpace(Type *Ty) {
  if (const auto *Pointer = dyn_cast<PointerType>(Ty))
    return Pointer->getAddressSpace() != 0;
  if (const auto *Array = dyn_cast<ArrayType>(Ty))
    return containsNonzeroSHAddressSpace(Array->getElementType());
  if (const auto *Struct = dyn_cast<StructType>(Ty))
    return llvm::any_of(Struct->elements(), containsNonzeroSHAddressSpace);
  if (const auto *Vector = dyn_cast<VectorType>(Ty))
    return containsNonzeroSHAddressSpace(Vector->getElementType());
  return false;
}

static bool hasUnsupportedSHABIAttribute(const Argument &Arg) {
  return Arg.hasAttribute(Attribute::InAlloca) ||
         Arg.hasAttribute(Attribute::Preallocated) ||
         Arg.hasAttribute(Attribute::ByRef) ||
         Arg.hasAttribute(Attribute::SwiftSelf) ||
         Arg.hasAttribute(Attribute::SwiftAsync) ||
         Arg.hasAttribute(Attribute::SwiftError);
}

static bool hasUnsupportedSHABIAttribute(const CallBase &Call, unsigned ArgNo) {
  return Call.paramHasAttr(ArgNo, Attribute::InAlloca) ||
         Call.paramHasAttr(ArgNo, Attribute::Preallocated) ||
         Call.paramHasAttr(ArgNo, Attribute::ByRef) ||
         Call.paramHasAttr(ArgNo, Attribute::SwiftSelf) ||
         Call.paramHasAttr(ArgNo, Attribute::SwiftAsync) ||
         Call.paramHasAttr(ArgNo, Attribute::SwiftError);
}

static bool validateLegalizableSHMemoryType(const Function &F, Type *Ty,
                                            const Instruction &I,
                                            bool AllowByteInteger = true) {
  if (const auto *Struct = dyn_cast<StructType>(Ty))
    return llvm::all_of(Struct->elements(), [&](Type *Element) {
      return validateLegalizableSHMemoryType(F, Element, I, AllowByteInteger);
    });
  if (const auto *Array = dyn_cast<ArrayType>(Ty))
    return validateLegalizableSHMemoryType(F, Array->getElementType(), I,
                                           AllowByteInteger);
  if (const auto *Vector = dyn_cast<VectorType>(Ty)) {
    if (Vector->isScalableTy())
      return diagnoseUnsupported(
          F, "SH scalable vector memory operations are not supported", &I);
    return validateLegalizableSHMemoryType(F, Vector->getElementType(), I,
                                           false);
  }
  const auto *Integer = dyn_cast<IntegerType>(Ty);
  if (Integer && !isPowerOf2_64(Integer->getBitWidth())) {
    bool Simple = isa<LoadInst>(I) ? cast<LoadInst>(I).isSimple()
                                   : cast<StoreInst>(I).isSimple();
    // Native memory legalization splits these exact byte-sized scalars; keep
    // the prior rejection of volatile, atomic, vector and fractional bytes.
    if (AllowByteInteger && Simple && Integer->getBitWidth() <= 64 &&
        Integer->getBitWidth() % 8 == 0)
      return true;
    return diagnoseUnsupported(
        F, "SH integer memory operations require a power-of-two bit width", &I);
  }
  return true;
}

static bool containsUnsupportedSHFloatingType(Type *Ty) {
  if (const auto *Vector = dyn_cast<VectorType>(Ty))
    return containsUnsupportedSHFloatingType(Vector->getElementType());
  return Ty->isFloatingPointTy() && !Ty->isFloatTy() && !Ty->isDoubleTy();
}

static bool validateSupportedSHFloatingSemantics(const Instruction &I) {
  bool HasFloatingSemantics =
      I.getOpcode() == Instruction::FNeg || I.isBinaryOp() ||
      isa<FCmpInst, IntrinsicInst>(I) ||
      (isa<CastInst>(I) && I.getOpcode() != Instruction::BitCast);
  if (!HasFloatingSemantics)
    return true;
  if (containsUnsupportedSHFloatingType(I.getType()) ||
      llvm::any_of(I.operands(), [](const Use &Operand) {
        return containsUnsupportedSHFloatingType(Operand->getType());
      }))
    return diagnoseUnsupported(
        *I.getFunction(),
        "SH floating-point semantics only support f32 and f64", &I);
  return true;
}

bool llvm::validateSHIR(const Function &F, SH::ABIKind ABI) {
  const DataLayout &DL = F.getDataLayout();
  if (!isSupportedSHCallingConvention(F.getCallingConv()))
    return diagnoseUnsupported(
        F, "SH only supports the C and fast calling conventions");
  if (!validateSupportedFastFunction(F))
    return false;
  Attribute HandlerAttr = F.getFnAttribute("sh-interrupt-handler");
  bool IsHandler = HandlerAttr.isValid();
  Attribute NoSaveLowRegsAttr = F.getFnAttribute("sh-nosave-low-regs");
  if (NoSaveLowRegsAttr.isValid()) {
    if (!NoSaveLowRegsAttr.getValueAsString().empty())
      return diagnoseUnsupported(
          F, "SH nosave_low_regs attribute has an invalid value");
    if (!IsHandler || HandlerAttr.getValueAsString() != "interrupt")
      return diagnoseUnsupported(
          F, "SH nosave_low_regs requires an interrupt hardware handler");
  }
  Attribute ResbankAttr = F.getFnAttribute("sh-resbank");
  if (ResbankAttr.isValid()) {
    if (!ResbankAttr.getValueAsString().empty())
      return diagnoseUnsupported(F,
                                 "SH resbank attribute has an invalid value");
    if (!IsHandler || HandlerAttr.getValueAsString() != "interrupt")
      return diagnoseUnsupported(
          F, "SH resbank requires an interrupt hardware handler");
    if (NoSaveLowRegsAttr.isValid())
      return diagnoseUnsupported(
          F, "SH resbank and nosave_low_regs attributes are incompatible");
  }
  Attribute StackSwitchAttr = F.getFnAttribute("sh-sp-switch");
  if (StackSwitchAttr.isValid()) {
    StringRef Symbol = StackSwitchAttr.getValueAsString();
    if (Symbol.empty() || Symbol.contains('\0'))
      return diagnoseUnsupported(
          F, "SH sp_switch attribute has an invalid symbol");
    if (!IsHandler || HandlerAttr.getValueAsString() != "interrupt")
      return diagnoseUnsupported(
          F, "SH sp_switch requires an interrupt hardware handler");
  }
  Attribute TrapExitAttr = F.getFnAttribute("sh-trap-exit");
  if (TrapExitAttr.isValid()) {
    unsigned Number;
    if (TrapExitAttr.getValueAsString().getAsInteger(10, Number) ||
        Number > 255)
      return diagnoseUnsupported(
          F, "SH trap_exit attribute has an invalid number");
    if (!IsHandler || HandlerAttr.getValueAsString() != "interrupt")
      return diagnoseUnsupported(
          F, "SH trap_exit requires an interrupt hardware handler");
    if (ResbankAttr.isValid())
      return diagnoseUnsupported(
          F, "SH trap_exit and resbank attributes are incompatible");
    if (StackSwitchAttr.isValid())
      return diagnoseUnsupported(
          F, "SH trap_exit and sp_switch attributes are incompatible");
  }
  if (IsHandler) {
    StringRef Kind = HandlerAttr.getValueAsString();
    if (Kind != "interrupt" && Kind != "trapa")
      return diagnoseUnsupported(F,
                                 "SH handler attribute has an invalid value");
    if (F.getCallingConv() != CallingConv::C ||
        !F.getReturnType()->isVoidTy() || !F.arg_empty() || F.isVarArg())
      return diagnoseUnsupported(F, "SH hardware handlers require the C "
                                    "calling convention and void () type");
    if (SH::isFDPICABI(ABI))
      return diagnoseUnsupported(F,
                                 "SH FDPIC hardware handlers are unsupported "
                                 "because entry has no r12 descriptor context");
    if (F.hasFnAttribute(Attribute::Naked))
      return diagnoseUnsupported(
          F, "SH hardware handlers cannot use the naked attribute");
    if (F.hasFnAttribute(Attribute::AlwaysInline))
      return diagnoseUnsupported(
          F, "SH hardware handlers cannot use the alwaysinline attribute");
    if (F.hasFnAttribute(Attribute::SanitizeAddress) ||
        F.hasFnAttribute(Attribute::SanitizeHWAddress) ||
        F.hasFnAttribute(Attribute::SanitizeMemory) ||
        F.hasFnAttribute(Attribute::SanitizeThread) ||
        F.hasFnAttribute(Attribute::SanitizeMemTag) ||
        F.hasFnAttribute(Attribute::SafeStack) ||
        F.hasFnAttribute(Attribute::ShadowCallStack))
      return diagnoseUnsupported(
          F, "SH hardware handler sanitizer instrumentation is unsupported");
  }
  if (F.hasPersonalityFn()) {
    if (IsHandler)
      return diagnoseUnsupported(
          F, "SH hardware handlers do not support exception handling");
    EHPersonality Personality = classifyEHPersonality(F.getPersonalityFn());
    if (Personality != EHPersonality::GNU_CXX &&
        Personality != EHPersonality::GNU_C)
      return diagnoseUnsupported(
          F, "SH only supports GNU Itanium landing-pad personalities");
  }
  if (containsNonzeroSHAddressSpace(F.getReturnType()))
    return diagnoseUnsupported(F,
                               "SH nonzero address spaces are not supported");
  if (F.getReturnType()->isVectorTy())
    return diagnoseUnsupported(
        F, "SH vector returns require an explicit sret parameter");
  if (!F.getReturnType()->isVoidTy() &&
      !isSupportedSHValueType(F.getReturnType()))
    return diagnoseUnsupported(
        F, "SH function return type is not supported by the ABI");

  unsigned SRetCount = 0;
  for (const Argument &Arg : F.args()) {
    if (containsNonzeroSHAddressSpace(Arg.getType()))
      return diagnoseUnsupported(F,
                                 "SH nonzero address spaces are not supported");
    if (hasUnsupportedSHABIAttribute(Arg))
      return diagnoseUnsupported(
          F, "SH function argument has unsupported ABI attributes");
    if (Arg.hasByValAttr()) {
      if (!validateSupportedSHByValType(
              F, DL, Arg.getParamByValType(),
              Arg.getParamAlign().value_or(
                  DL.getABITypeAlign(Arg.getParamByValType()))))
        return false;
      continue;
    }
    if (Arg.hasStructRetAttr()) {
      ++SRetCount;
      if (!validateSupportedSHSRetType(F, DL, Arg.getParamStructRetType()))
        return false;
      continue;
    }
    if (!isSupportedSHValueType(Arg.getType()))
      return diagnoseUnsupported(
          F, "SH function argument type is not supported by the ABI");
  }
  if (SRetCount > 1)
    return diagnoseUnsupported(F,
                               "SH functions may have only one sret parameter");
  if (F.getFnStackAlign() && F.getFnStackAlign()->value() > SHMaxStackAlignment)
    return diagnoseUnsupported(
        F, Twine("SH function stack alignment cannot exceed ") +
               Twine(SHMaxStackAlignment) + " bytes");
  if (F.hasFnAttribute("probe-stack") || F.hasFnAttribute("stack-probe-size"))
    return diagnoseUnsupported(F, "SH stack probing is not supported");

  for (const BasicBlock &BB : F) {
    if (IsHandler && BB.isEHPad())
      return diagnoseUnsupported(
          F, "SH hardware handlers do not support exception-handling pads");
    if (BB.isEHPad() && !isa<LandingPadInst>(BB.getFirstNonPHIIt()))
      return diagnoseUnsupported(F,
                                 "SH only supports Itanium landingpad EH pads");
    for (const Instruction &I : BB) {
      if (IsHandler && isa<InvokeInst, LandingPadInst, ResumeInst>(I))
        return diagnoseUnsupported(F,
                                   "SH hardware handlers do not support "
                                   "exception-handling control flow",
                                   &I);
      if (!validateSupportedSHFloatingSemantics(I))
        return false;
      if (containsNonzeroSHAddressSpace(I.getType()) ||
          llvm::any_of(I.operands(), [](const Use &Operand) {
            return containsNonzeroSHAddressSpace(Operand->getType());
          }))
        return diagnoseUnsupported(
            F, "SH nonzero address spaces are not supported", &I);
      if (isUnsupportedSHEHInstruction(I))
        return diagnoseUnsupported(
            F, "SH does not support funclet exception-handling IR", &I);
      if (const auto *Alloca = dyn_cast<AllocaInst>(&I)) {
        Type *AllocatedTy = Alloca->getAllocatedType();
        if (!AllocatedTy->isSized() ||
            DL.getTypeAllocSize(AllocatedTy).isScalable())
          return diagnoseUnsupported(
              F, "SH stack objects require a fixed allocation size", &I);
        if (Alloca->getAlign().value() > SHMaxStackAlignment)
          return diagnoseUnsupported(
              F,
              Twine("SH stack object alignment cannot exceed ") +
                  Twine(SHMaxStackAlignment) + " bytes",
              &I);
      }
      if (const auto *VA = dyn_cast<VAArgInst>(&I)) {
        if (!F.isVarArg())
          return diagnoseUnsupported(
              F, "SH va_arg requires a variadic function", &I);
        Type *Ty = VA->getType();
        if (!Ty->isIntegerTy(32) && !Ty->isIntegerTy(64) && !Ty->isDoubleTy() &&
            !Ty->isPointerTy())
          return diagnoseUnsupported(
              F, "SH va_arg requires a supported default-promoted ABI type",
              &I);
      }
      if (const auto *Load = dyn_cast<LoadInst>(&I))
        if (!validateLegalizableSHMemoryType(F, Load->getType(), I))
          return false;
      if (const auto *Store = dyn_cast<StoreInst>(&I))
        if (!validateLegalizableSHMemoryType(
                F, Store->getValueOperand()->getType(), I))
          return false;
      if (const auto *BinOp = dyn_cast<BinaryOperator>(&I)) {
        SmallPtrSet<const Value *, 8> Visited;
        if (isBlockAddressInteger(BinOp->getOperand(0), Visited) ||
            isBlockAddressInteger(BinOp->getOperand(1), Visited))
          return diagnoseUnsupported(
              F, "SH block-address arithmetic is not supported", &I);
      }
      if (isa<AtomicRMWInst>(&I) || isa<AtomicCmpXchgInst>(&I) ||
          (isa<LoadInst>(&I) && cast<LoadInst>(I).isAtomic()) ||
          (isa<StoreInst>(&I) && cast<StoreInst>(I).isAtomic()))
        return diagnoseUnsupported(
            F, "SH generic atomic operation survived AtomicExpand", &I);
      const auto *Call = dyn_cast<CallBase>(&I);
      if (!Call)
        continue;
      if (!validateSHInlineAsm(*Call, DL) || !validateSupportedCall(*Call))
        return false;
      if (const auto *MI = dyn_cast<MemIntrinsic>(Call)) {
        if (!validateSHMemoryIntrinsic(*MI))
          return false;
        continue;
      }
      switch (Call->getIntrinsicID()) {
      case Intrinsic::sh_tas_b:
      case Intrinsic::sh_get_thread_pointer:
      case Intrinsic::sh_set_thread_pointer:
        continue;
      case Intrinsic::sh_get_fpscr:
      case Intrinsic::sh_set_fpscr:
        if (F.hasFnAttribute("sh-interrupt-handler"))
          return diagnoseUnsupported(
              F, "SH FPSCR control intrinsics are unsupported in handlers", &I);
        continue;
      case Intrinsic::sh_fsca:
      case Intrinsic::sh_fsrra:
      case Intrinsic::sh_fipr:
      case Intrinsic::sh_ftrv:
      case Intrinsic::sh_ftrv4:
      case Intrinsic::sh_ftrv_array:
      case Intrinsic::sh_ftrv_compose:
        if (F.hasFnAttribute("sh-interrupt-handler") ||
            F.hasFnAttribute(Attribute::StrictFP))
          return diagnoseUnsupported(F,
                                     "SH graphics intrinsics are unsupported "
                                     "in handlers or strict FP functions",
                                     &I);
        continue;
      case Intrinsic::memcpy_element_unordered_atomic:
      case Intrinsic::memmove_element_unordered_atomic:
      case Intrinsic::memset_element_unordered_atomic:
        return diagnoseUnsupported(
            F, "SH atomic memory intrinsics are not supported", &I);
      case Intrinsic::frameaddress:
      case Intrinsic::returnaddress: {
        const auto *Depth = dyn_cast<ConstantInt>(Call->getArgOperand(0));
        if (!Depth || !Depth->isZero())
          return diagnoseUnsupported(
              F,
              "SH frame and return address intrinsics only support depth zero",
              &I);
        continue;
      }
      case Intrinsic::vastart:
        if (!F.isVarArg())
          return diagnoseUnsupported(
              F, "SH va_start requires a variadic function", &I);
        if (llvm::none_of(F.args(), [](const Argument &Arg) {
              return !Arg.hasStructRetAttr();
            }))
          return diagnoseUnsupported(
              F, "SH va_start requires at least one fixed parameter", &I);
        continue;
      case Intrinsic::threadlocal_address: {
        const auto *GV = dyn_cast<GlobalVariable>(
            Call->getArgOperand(0)->stripPointerCasts());
        if (!GV || !GV->isThreadLocal())
          return diagnoseUnsupported(
              F, "SH thread-local address intrinsic requires a TLS global", &I);
        continue;
      }
      case Intrinsic::eh_return_i64:
        return diagnoseUnsupported(
            F, "SH EH return requires a 32-bit stack offset", &I);
      case Intrinsic::eh_exceptionpointer:
      case Intrinsic::eh_exceptioncode:
      case Intrinsic::eh_unwind_init:
      case Intrinsic::eh_dwarf_cfa:
      case Intrinsic::eh_sjlj_lsda:
      case Intrinsic::eh_sjlj_callsite:
      case Intrinsic::eh_sjlj_functioncontext:
      case Intrinsic::eh_sjlj_setjmp:
      case Intrinsic::eh_sjlj_longjmp:
      case Intrinsic::eh_sjlj_setup_dispatch:
        return diagnoseUnsupported(
            F,
            "SH language exception handling is not supported: EH "
            "intrinsic",
            &I);
      default:
        break;
      }

      Intrinsic::ID IntrinsicID = Call->getIntrinsicID();
      if (IntrinsicID != Intrinsic::not_intrinsic) {
        if (Intrinsic::isTargetIntrinsic(IntrinsicID))
          return diagnoseUnsupported(
              F, "target intrinsic is not supported on SH", &I);
        continue;
      }
      if (const auto *CallInst = dyn_cast<llvm::CallInst>(Call))
        if (CallInst->isMustTailCall())
          return diagnoseUnsupported(F, "SH musttail calls are not supported",
                                     &I);

      SHAtomicRuntimeCallKind AtomicCallKind =
          getSHAtomicRuntimeCallKind(*Call);
      Type *ReturnTy = Call->getType();
      if (ReturnTy->isVectorTy())
        return diagnoseUnsupported(
            F, "SH vector call results require an explicit sret parameter", &I);
      if (!ReturnTy->isVoidTy() && !isSupportedSHValueType(ReturnTy) &&
          !(AtomicCallKind == SHAtomicRuntimeCallKind::CompareExchange &&
            ReturnTy->isIntegerTy(1)))
        return diagnoseUnsupported(
            F, "SH call return type is not supported by the ABI", &I);

      unsigned CallSRetCount = 0;
      for (unsigned ArgNo = 0; ArgNo != Call->arg_size(); ++ArgNo) {
        Type *ArgTy = Call->getArgOperand(ArgNo)->getType();
        bool IsUnnamed = Call->getFunctionType()->isVarArg() &&
                         ArgNo >= Call->getFunctionType()->getNumParams();
        bool IsFPUABIFloat =
            IsUnnamed && ArgTy->isFloatTy() && SH::usesFPUArgumentABI(ABI);
        if (IsUnnamed && !isSupportedSHVarArgType(ArgTy) && !IsFPUABIFloat)
          return diagnoseUnsupported(F,
                                     "SH variadic arguments must use supported "
                                     "default-promoted ABI types",
                                     &I);
        if (hasUnsupportedSHABIAttribute(*Call, ArgNo))
          return diagnoseUnsupported(
              F, "SH call argument has unsupported ABI attributes", &I);
        if (Call->paramHasAttr(ArgNo, Attribute::ByVal)) {
          Type *ByValTy = Call->getParamByValType(ArgNo);
          if (!validateSupportedSHByValType(F, DL, ByValTy,
                                            Call->getParamAlign(ArgNo).value_or(
                                                DL.getABITypeAlign(ByValTy))))
            return false;
          continue;
        }
        if (Call->paramHasAttr(ArgNo, Attribute::StructRet)) {
          ++CallSRetCount;
          if (!validateSupportedSHSRetType(F, DL,
                                           Call->getParamStructRetType(ArgNo)))
            return false;
          continue;
        }
        if (!isSupportedSHValueType(ArgTy))
          return diagnoseUnsupported(
              F, "SH call argument type is not supported by the ABI", &I);
      }
      if (CallSRetCount > 1)
        return diagnoseUnsupported(
            F, "SH calls may have only one sret parameter", &I);
    }
  }
  return true;
}

char SHAtomicValidateLegacy::ID = 0;
char SHIRValidationLegacy::ID = 0;

INITIALIZE_PASS(SHAtomicValidateLegacy, "sh-atomic-validate",
                "SH validate atomic operations", false, true)
INITIALIZE_PASS(SHIRValidationLegacy, "sh-ir-validate",
                "SH validate prepared IR", false, true)

FunctionPass *llvm::createSHAtomicValidateLegacyPass(SHTargetMachine &TM) {
  return new SHAtomicValidateLegacy(TM);
}

ModulePass *llvm::createSHIRValidationLegacyPass(SHTargetMachine &TM) {
  return new SHIRValidationLegacy(TM);
}

PreservedAnalyses SHAtomicValidatePass::run(Function &F,
                                            FunctionAnalysisManager &FAM) {
  if (validateSHAtomics(F, TM.getSubtargetImpl(F)->hasDoubleFPU()))
    return PreservedAnalyses::all();
  replaceSHFunctionWithUnreachable(F);
  return PreservedAnalyses::none();
}
