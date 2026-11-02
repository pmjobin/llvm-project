//===- SH.cpp -------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABIInfoImpl.h"
#include "TargetInfo.h"
#include "llvm/TargetParser/SHTargetParser.h"

using namespace clang;
using namespace clang::CodeGen;

namespace {

ABIArgInfo getDirectAggregate() {
  ABIArgInfo Info = ABIArgInfo::getDirect();
  Info.setCanBeFlattened(false);
  return Info;
}

ABIArgInfo getDirectAggregate(llvm::Type *CoerceTy) {
  ABIArgInfo Info = ABIArgInfo::getDirect(CoerceTy);
  Info.setCanBeFlattened(false);
  return Info;
}

class SHABIInfo : public ABIInfo {
  llvm::SH::ABIKind ABI;

  ABIArgInfo classifyReturnType(QualType Ty) const;
  ABIArgInfo classifyArgumentType(QualType Ty) const;
  bool isFPUABIRealType(QualType Ty) const;
  QualType getSoleFloatMember(QualType Ty) const;
  llvm::StructType *getHardFloatComplexType(QualType Ty) const;
  ABIArgInfo getHardFloatDirectAggregate(QualType Ty) const;

public:
  SHABIInfo(CodeGenTypes &CGT, llvm::SH::ABIKind ABI)
      : ABIInfo(CGT), ABI(ABI) {}

  void computeInfo(CGFunctionInfo &FI) const override;
  RValue EmitVAArg(CodeGenFunction &CGF, Address VAListAddr, QualType Ty,
                   AggValueSlot Slot) const override;
};

class SHTargetCodeGenInfo : public TargetCodeGenInfo {
  std::string ABIName;

public:
  SHTargetCodeGenInfo(CodeGenTypes &CGT, StringRef ABIName)
      : TargetCodeGenInfo(
            std::make_unique<SHABIInfo>(CGT, llvm::SH::parseABI(ABIName))),
        ABIName(ABIName) {}

  void setTargetAttributes(const Decl *D, llvm::GlobalValue *GV,
                           CodeGen::CodeGenModule &CGM) const override;
  bool
  shouldSetCPUAndFeaturesAttributesOnGeneratedDeclarations() const override {
    return true;
  }
  llvm::Value *getUBSanFunctionCodePointer(CodeGen::CodeGenFunction &CGF,
                                           llvm::Value *Callee) const override;
  void emitTargetMetadata(CodeGen::CodeGenModule &CGM,
                          const llvm::MapVector<GlobalDecl, StringRef>
                              &MangledDeclNames) const override;
};

} // namespace

bool SHABIInfo::isFPUABIRealType(QualType Ty) const {
  if (!Ty->isRealFloatingType())
    return false;
  uint64_t Width = getContext().getTypeSize(Ty);
  return Width == 32 || (Width == 64 && llvm::SH::usesSH4HardFloatABI(ABI));
}

QualType SHABIInfo::getSoleFloatMember(QualType Ty) const {
  while (const auto *RT = Ty->getAs<RecordType>()) {
    const RecordDecl *RD = RT->getDecl();
    // A union retains its aggregate mode in the GNU SH ABI.
    if (RD->isUnion())
      return QualType();
    QualType MemberTy;
    if (const auto *CXXRD = dyn_cast<CXXRecordDecl>(RD))
      for (const CXXBaseSpecifier &Base : CXXRD->bases()) {
        if (Base.isVirtual())
          return QualType();
        if (isEmptyRecord(getContext(), Base.getType(), true))
          continue;
        if (!MemberTy.isNull() ||
            !getContext()
                 .getASTRecordLayout(CXXRD)
                 .getBaseClassOffset(Base.getType()->getAsCXXRecordDecl())
                 .isZero())
          return QualType();
        MemberTy = Base.getType();
      }
    for (const FieldDecl *Field : RD->fields()) {
      if (Field->isUnnamedBitField() ||
          getContext().getTypeSize(Field->getType()) == 0)
        continue;
      if (!MemberTy.isNull())
        return QualType();
      if (Field->isBitField() ||
          getContext().getASTRecordLayout(RD).getFieldOffset(
              Field->getFieldIndex()) != 0)
        return QualType();
      MemberTy = Field->getType();
    }
    if (MemberTy.isNull() ||
        getContext().getTypeSize(Ty) != getContext().getTypeSize(MemberTy) ||
        getContext().getTypeAlign(Ty) < getContext().getTypeAlign(MemberTy))
      return QualType();
    Ty = MemberTy;
  }
  if (const auto *CT = Ty->getAs<ComplexType>())
    return CT->getElementType()->isRealFloatingType() &&
                   getContext().getTypeSize(CT->getElementType()) <= 64
               ? Ty
               : QualType();
  return Ty->isRealFloatingType() && getContext().getTypeSize(Ty) <= 64
             ? Ty
             : QualType();
}

llvm::StructType *SHABIInfo::getHardFloatComplexType(QualType Ty) const {
  if (Ty->isRecordType()) {
    Ty = getSoleFloatMember(Ty);
    if (Ty.isNull())
      return nullptr;
  }
  const auto *ComplexTy = Ty->getAs<ComplexType>();
  if (!ComplexTy)
    return nullptr;
  QualType ElementTy = ComplexTy->getElementType();
  if (!isFPUABIRealType(ElementTy))
    return nullptr;
  llvm::Type *IRType = CGT.ConvertType(ElementTy);
  return llvm::StructType::get(getVMContext(), {IRType, IRType});
}

ABIArgInfo SHABIInfo::getHardFloatDirectAggregate(QualType Ty) const {
  if (QualType MemberTy = getSoleFloatMember(Ty);
      !MemberTy.isNull() && MemberTy->isRealFloatingType())
    return getDirectAggregate(CGT.ConvertType(MemberTy));
  uint64_t Width = getContext().getTypeSize(Ty);
  return getDirectAggregate(
      llvm::IntegerType::get(getVMContext(), Width <= 32 ? 32 : 64));
}

ABIArgInfo SHABIInfo::classifyReturnType(QualType Ty) const {
  if (Ty->isVoidType())
    return ABIArgInfo::getIgnore();
  // GNU SH returns native vectors through the hidden R2 result pointer,
  // including eight-byte vectors. In-function FV allocation does not alter it.
  if (Ty->isVectorType())
    return getNaturalAlignIndirect(Ty, getDataLayout().getAllocaAddrSpace());
  if (llvm::SH::usesFPUArgumentABI(ABI))
    if (llvm::StructType *ComplexTy = getHardFloatComplexType(Ty))
      return getDirectAggregate(ComplexTy);

  if (isAggregateTypeForABI(Ty)) {
    if (isEmptyRecord(getContext(), Ty, true))
      return ABIArgInfo::getIgnore();

    TypeInfoChars Info = getContext().getTypeInfoInChars(Ty);
    CharUnits RequiredAlign;
    switch (Info.Width.getQuantity()) {
    default:
      return getNaturalAlignIndirect(Ty, getDataLayout().getAllocaAddrSpace());
    case 1:
      RequiredAlign = CharUnits::One();
      break;
    case 2:
      RequiredAlign = CharUnits::fromQuantity(2);
      break;
    case 4:
    case 8:
      RequiredAlign = CharUnits::fromQuantity(4);
      break;
    }
    if (Info.Align < RequiredAlign)
      return getNaturalAlignIndirect(Ty, getDataLayout().getAllocaAddrSpace());
    if (llvm::SH::usesFPUArgumentABI(ABI))
      return getHardFloatDirectAggregate(Ty);
    return getDirectAggregate();
  }

  if (const auto *ED = Ty->getAsEnumDecl())
    Ty = ED->getIntegerType();
  if (isPromotableIntegerTypeForABI(Ty))
    return ABIArgInfo::getExtend(Ty);
  return ABIArgInfo::getDirect();
}

ABIArgInfo SHABIInfo::classifyArgumentType(QualType Ty) const {
  Ty = useFirstFieldIfTransparentUnion(Ty);
  if (CGCXXABI::RecordArgABI RAA = getRecordArgABI(Ty, getCXXABI()))
    return getNaturalAlignIndirect(Ty, getDataLayout().getAllocaAddrSpace(),
                                   RAA == CGCXXABI::RAA_DirectInMemory);

  // The E-float ABI uses FPRs only for real single-precision arguments.
  // Complex arguments use the integer area, even though complex float returns
  // still use fr0/fr1.
  if (llvm::SH::usesSH4HardFloatABI(ABI))
    if (llvm::StructType *ComplexTy = getHardFloatComplexType(Ty))
      return getDirectAggregate(ComplexTy);

  if (isAggregateTypeForABI(Ty)) {
    if (isEmptyRecord(getContext(), Ty, true))
      return ABIArgInfo::getIgnore();
    TypeInfoChars Info = getContext().getTypeInfoInChars(Ty);
    if (llvm::SH::usesFPUArgumentABI(ABI)) {
      if (QualType MemberTy = getSoleFloatMember(Ty);
          !MemberTy.isNull() && MemberTy->isRealFloatingType())
        return getHardFloatDirectAggregate(Ty);
      return getNaturalAlignIndirect(Ty, getDataLayout().getAllocaAddrSpace(),
                                     /*ByVal=*/true);
    }
    if (Info.Width <= CharUnits::fromQuantity(8) &&
        Info.Align <= CharUnits::fromQuantity(4))
      return getDirectAggregate();
    return getNaturalAlignIndirect(Ty, getDataLayout().getAllocaAddrSpace(),
                                   /*ByVal=*/true);
  }

  if (const auto *ED = Ty->getAsEnumDecl())
    Ty = ED->getIntegerType();
  if (isPromotableIntegerTypeForABI(Ty))
    return ABIArgInfo::getExtend(Ty);
  return ABIArgInfo::getDirect();
}

void SHABIInfo::computeInfo(CGFunctionInfo &FI) const {
  if (!getCXXABI().classifyReturnType(FI))
    FI.getReturnInfo() = classifyReturnType(FI.getReturnType());
  for (auto &Arg : FI.arguments())
    Arg.info = classifyArgumentType(Arg.type);
}

RValue SHABIInfo::EmitVAArg(CodeGenFunction &CGF, Address VAListAddr,
                            QualType Ty, AggValueSlot Slot) const {
  if (isEmptyRecord(getContext(), Ty, true))
    return Slot.asRValue();
  TypeInfoChars Info = getContext().getTypeInfoInChars(Ty);
  if (llvm::SH::usesFPUArgumentABI(ABI)) {
    QualType FloatTy = Ty->isRealFloatingType() ? Ty : getSoleFloatMember(Ty);
    const auto *ComplexTy = Ty->getAs<ComplexType>();
    if (!ComplexTy && !FloatTy.isNull())
      ComplexTy = FloatTy->getAs<ComplexType>();
    if (ComplexTy)
      FloatTy = llvm::SH::usesSH4HardFloatABI(ABI) ? ComplexTy->getElementType()
                                                   : QualType();
    bool InFPRs = !FloatTy.isNull() && isFPUABIRealType(FloatTy);
    unsigned NextIndex = InFPRs ? 2 : 0;
    unsigned LimitIndex = InFPRs ? 3 : 1;
    Address NextAddr =
        CGF.Builder.CreateStructGEP(VAListAddr, NextIndex, "vaarg.next.ptr");
    Address LimitAddr =
        CGF.Builder.CreateStructGEP(VAListAddr, LimitIndex, "vaarg.limit.ptr");
    Address StackAddr =
        CGF.Builder.CreateStructGEP(VAListAddr, 4, "vaarg.stack.ptr");
    llvm::Value *RawNext = CGF.Builder.CreateLoad(NextAddr, "vaarg.next");
    llvm::Value *Limit = CGF.Builder.CreateLoad(LimitAddr, "vaarg.limit");
    llvm::Value *Stack = CGF.Builder.CreateLoad(StackAddr, "vaarg.stack");
    CharUnits Size = Info.Width.alignTo(CharUnits::fromQuantity(4));
    llvm::Value *Candidate = RawNext;
    // Alignment follows the component, not the complete complex value.
    // Complex float can start at an odd FPR cursor; complex double must start
    // at an even one and fit all four registers without splitting to the stack.
    if (InFPRs && getContext().getTypeSize(FloatTy) == 64) {
      // The speculative bias can point beyond the register-save object when
      // the cursor is exhausted. It must not make the fit test depend on poison.
      llvm::Value *Biased = CGF.Builder.CreateGEP(
          CGF.Int8Ty, Candidate, CGF.Builder.getInt32(7));
      Candidate = CGF.Builder.CreateIntrinsic(
          llvm::Intrinsic::ptrmask, {Candidate->getType(), CGF.IntPtrTy},
          {Biased, llvm::ConstantInt::getSigned(CGF.IntPtrTy, -8)}, nullptr,
          "vaarg.next.aligned");
    }
    llvm::Value *Next = CGF.Builder.CreateGEP(
        CGF.Int8Ty, Candidate, CGF.Builder.getSize(Size), "vaarg.reg.next");
    llvm::Value *Fits = CGF.Builder.CreateICmpULE(Next, Limit, "vaarg.in.regs");
    llvm::Value *StackNext = CGF.Builder.CreateInBoundsGEP(
        CGF.Int8Ty, Stack, CGF.Builder.getSize(Size), "vaarg.stack.next");
    llvm::Value *ExhaustedNext =
        !InFPRs && llvm::SH::getBaseABI(ABI) == llvm::SH::ABIKind::GNUEFloat
            ? Limit
            : RawNext;
    CGF.Builder.CreateStore(CGF.Builder.CreateSelect(Fits, Next, ExhaustedNext),
                            NextAddr);
    CGF.Builder.CreateStore(CGF.Builder.CreateSelect(Fits, Stack, StackNext),
                            StackAddr);
    llvm::Value *Pointer =
        CGF.Builder.CreateSelect(Fits, Candidate, Stack, "vaarg.addr");
    if (!InFPRs && CGF.CGM.getDataLayout().isBigEndian() &&
        Info.Width < CharUnits::fromQuantity(4))
      Pointer = CGF.Builder.CreateInBoundsGEP(
          CGF.Int8Ty, Pointer,
          CGF.Builder.getSize(CharUnits::fromQuantity(4) - Info.Width));
    Address ValueAddr(Pointer, CGF.ConvertTypeForMem(Ty), Info.Align);
    if (ComplexTy && InFPRs && Info.Width.getQuantity() == 8 &&
        CGF.CGM.getDataLayout().isLittleEndian()) {
      // GNU SH4's SCmode convention puts the real component first in physical
      // register order at an even cursor (fr4/fr5, for example), opposite to
      // the scalar register-save order. At an odd cursor the caller instead
      // uses the scalar order (fr4/fr7). Stack components always retain their
      // natural real/imaginary order.
      llvm::Value *EvenCursor = CGF.Builder.CreateICmpEQ(
          CGF.Builder.CreateAnd(CGF.Builder.CreatePtrToInt(RawNext, CGF.IntPtrTy),
                                4),
          llvm::ConstantInt::get(CGF.IntPtrTy, 0), "vaarg.even.cursor");
      llvm::Value *Swap =
          CGF.Builder.CreateAnd(Fits, EvenCursor, "vaarg.swap.components");
      llvm::StructType *PairTy = getHardFloatComplexType(Ty);
      Address PairAddr = ValueAddr.withElementType(PairTy);
      llvm::Value *First =
          CGF.Builder.CreateLoad(CGF.Builder.CreateStructGEP(PairAddr, 0));
      llvm::Value *Second =
          CGF.Builder.CreateLoad(CGF.Builder.CreateStructGEP(PairAddr, 1));
      llvm::Value *Real = CGF.Builder.CreateSelect(Swap, Second, First);
      llvm::Value *Imaginary = CGF.Builder.CreateSelect(Swap, First, Second);
      if (Ty->isAnyComplexType())
        return RValue::getComplex(Real, Imaginary);
      // An aggregate va_arg needs an address in natural memory order. Do not
      // mutate the shared register save area: va_copy may still refer to it.
      ValueAddr = CGF.CreateMemTemp(Ty, "vaarg.complex.record");
      PairAddr = ValueAddr.withElementType(PairTy);
      CGF.Builder.CreateStore(Real, CGF.Builder.CreateStructGEP(PairAddr, 0));
      CGF.Builder.CreateStore(Imaginary,
                              CGF.Builder.CreateStructGEP(PairAddr, 1));
    }
    return CGF.EmitLoadOfAnyValue(CGF.MakeAddrLValue(ValueAddr, Ty), Slot);
  }
  return emitVoidPtrVAArg(CGF, VAListAddr, Ty, /*IsIndirect=*/false, Info,
                          CharUnits::fromQuantity(4),
                          /*AllowHigherAlign=*/false, Slot,
                          /*ForceRightAdjust=*/true);
}

std::unique_ptr<TargetCodeGenInfo>
CodeGen::createSHTargetCodeGenInfo(CodeGenModule &CGM, StringRef ABIName) {
  return std::make_unique<SHTargetCodeGenInfo>(CGM.getTypes(), ABIName);
}

void SHTargetCodeGenInfo::setTargetAttributes(
    const Decl *D, llvm::GlobalValue *GV, CodeGen::CodeGenModule &CGM) const {
  (void)CGM;
  auto *F = dyn_cast<llvm::Function>(GV);
  if (!F)
    return;
  F->addFnAttr("target-abi", ABIName);
  const auto *FD = dyn_cast_or_null<FunctionDecl>(D);
  if (!FD)
    return;
  StringRef HandlerKind;
  if (FD->hasAttr<SHInterruptHandlerAttr>())
    HandlerKind = "interrupt";
  else if (FD->hasAttr<SHTRAPAHandlerAttr>())
    HandlerKind = "trapa";
  else
    return;
  F->addFnAttr("sh-interrupt-handler", HandlerKind);
  if (FD->hasAttr<SHNoSaveLowRegsAttr>())
    F->addFnAttr("sh-nosave-low-regs");
  if (FD->hasAttr<SHResbankAttr>())
    F->addFnAttr("sh-resbank");
  if (const auto *A = FD->getAttr<SHSpSwitchAttr>())
    F->addFnAttr("sh-sp-switch", A->getStackSymbol());
  if (const auto *A = FD->getAttr<SHTrapExitAttr>())
    F->addFnAttr("sh-trap-exit", llvm::Twine(A->getTrapNumber()).str());
  F->addFnAttr(llvm::Attribute::NoInline);
  F->addFnAttr(llvm::Attribute::NoUnwind);
  F->addFnAttr(llvm::Attribute::DisableSanitizerInstrumentation);
  F->addFnAttr("disable-tail-calls", "true");
}

llvm::Value *
SHTargetCodeGenInfo::getUBSanFunctionCodePointer(CodeGenFunction &CGF,
                                                 llvm::Value *Callee) const {
  if (!llvm::SH::isFDPICABI(llvm::SH::parseABI(ABIName)))
    return Callee;
  return CGF.Builder.CreateAlignedLoad(
      CGF.Int8PtrTy, Callee, CGF.getPointerAlign(), "ubsan.function.entry");
}

void SHTargetCodeGenInfo::emitTargetMetadata(
    CodeGen::CodeGenModule &CGM,
    const llvm::MapVector<GlobalDecl, StringRef> &MangledDeclNames) const {
  (void)MangledDeclNames;
  llvm::LLVMContext &Ctx = CGM.getLLVMContext();
  CGM.getModule().addModuleFlag(llvm::Module::Error, "target-abi",
                                llvm::MDString::get(Ctx, ABIName));
}
