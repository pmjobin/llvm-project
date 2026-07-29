//===-- SHLowerI64StackAlign.cpp - Lower i64 stack alignment -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SH.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

#define DEBUG_TYPE "sh-lower-i64-stack-align"

namespace {

static bool isSupportedSHAggregateElement(Type *Ty) {
  if (Ty->isIntegerTy(8) || Ty->isIntegerTy(16) || Ty->isIntegerTy(32) ||
      Ty->isIntegerTy(64))
    return true;
  if (const auto *PointerTy = dyn_cast<PointerType>(Ty))
    return PointerTy->getAddressSpace() == 0;
  if (const auto *ArrayTy = dyn_cast<ArrayType>(Ty))
    return isSupportedSHAggregateElement(ArrayTy->getElementType());
  if (const auto *StructTy = dyn_cast<StructType>(Ty)) {
    if (StructTy->isOpaque())
      return false;
    return llvm::all_of(StructTy->elements(), isSupportedSHAggregateElement);
  }
  return false;
}

static bool isSupportedSHVarArgType(Type *Ty) {
  return Ty->isIntegerTy(32) || Ty->isIntegerTy(64) ||
         (Ty->isPointerTy() && Ty->getPointerAddressSpace() == 0) ||
         (Ty->isAggregateType() && isSupportedSHAggregateElement(Ty));
}

static LoadInst *reconstructSHVarArgAggregate(IRBuilder<> &Builder, Function &F,
                                              Type *Ty, Value *Cursor,
                                              uint64_t ValueSize) {
  IRBuilder<> EntryBuilder(&*F.getEntryBlock().getFirstInsertionPt());
  AllocaInst *Temporary = EntryBuilder.CreateAlloca(Ty, nullptr, "vaarg.tmp");
  Temporary->setAlignment(Align(4));

  Type *Int8Ty = Builder.getInt8Ty();
  Type *Int32Ty = Builder.getInt32Ty();
  for (uint64_t Offset = 0; Offset < ValueSize; Offset += 4) {
    uint64_t ValidBytes = std::min<uint64_t>(4, ValueSize - Offset);
    uint64_t SourceOffset =
        Offset + (!F.getDataLayout().isLittleEndian() ? 4 - ValidBytes : 0);
    if (ValidBytes == 4) {
      Value *Source = Builder.CreateConstGEP1_64(Int8Ty, Cursor, SourceOffset,
                                                 "vaarg.source");
      LoadInst *Chunk =
          Builder.CreateAlignedLoad(Int32Ty, Source, Align(4), "vaarg.chunk");
      Value *Destination = Builder.CreateConstGEP1_64(Int8Ty, Temporary, Offset,
                                                      "vaarg.destination");
      Builder.CreateAlignedStore(Chunk, Destination, Align(4));
      continue;
    }
    for (uint64_t Byte = 0; Byte != ValidBytes; ++Byte) {
      Value *Source = Builder.CreateConstGEP1_64(
          Int8Ty, Cursor, SourceOffset + Byte, "vaarg.source");
      LoadInst *Piece =
          Builder.CreateAlignedLoad(Int8Ty, Source, Align(1), "vaarg.byte");
      Value *Destination = Builder.CreateConstGEP1_64(
          Int8Ty, Temporary, Offset + Byte, "vaarg.destination");
      Builder.CreateAlignedStore(Piece, Destination, Align(1));
    }
  }
  return Builder.CreateAlignedLoad(Ty, Temporary, Align(4));
}

static bool lowerSHVarArgs(Function &F) {
  const DataLayout &DL = F.getDataLayout();
  SmallVector<VAArgInst *, 8> Worklist;
  for (BasicBlock &BB : F)
    for (Instruction &I : BB)
      if (auto *VA = dyn_cast<VAArgInst>(&I))
        Worklist.push_back(VA);

  for (VAArgInst *VA : Worklist) {
    if (!F.isVarArg())
      report_fatal_error("SH va_arg requires a variadic function");
    Type *Ty = VA->getType();
    if (!isSupportedSHVarArgType(Ty))
      report_fatal_error(
          "SH va_arg requires a supported default-promoted ABI type");
    Value *VAListStorage = VA->getPointerOperand();
    if (VAListStorage->getType()->getPointerAddressSpace() != 0)
      report_fatal_error("SH va_list only supports address space zero");
    if (!Ty->isSized())
      report_fatal_error("SH va_arg type must have a fixed size");
    TypeSize Size = DL.getTypeAllocSize(Ty);
    if (Size.isScalable() || Size.getFixedValue() == 0)
      report_fatal_error("SH va_arg type must have a nonzero fixed size");

    IRBuilder<> Builder(VA);
    Type *PointerTy = PointerType::getUnqual(F.getContext());
    LoadInst *Cursor = Builder.CreateAlignedLoad(PointerTy, VAListStorage,
                                                 Align(4), "vaarg.cursor");
    Value *ValueAddress = Cursor;
    uint64_t ValueSize = Size.getFixedValue();
    if (Ty->isAggregateType() && alignTo(ValueSize, 4) > 60)
      report_fatal_error("SH va_arg aggregate size cannot exceed 60 bytes");
    LoadInst *LoadedValue;
    if (Ty->isAggregateType()) {
      LoadedValue =
          reconstructSHVarArgAggregate(Builder, F, Ty, Cursor, ValueSize);
    } else {
      if (!DL.isLittleEndian() && ValueSize < 4)
        ValueAddress = Builder.CreateConstGEP1_64(Builder.getInt8Ty(), Cursor,
                                                  4 - ValueSize);
      Align ValueAlign = std::min(DL.getABITypeAlign(Ty), Align(4));
      LoadedValue = Builder.CreateAlignedLoad(Ty, ValueAddress, ValueAlign);
    }
    Value *NextCursor = Builder.CreateConstGEP1_64(
        Builder.getInt8Ty(), Cursor, alignTo(ValueSize, 4), "vaarg.next");
    Builder.CreateAlignedStore(NextCursor, VAListStorage, Align(4));
    VA->replaceAllUsesWith(LoadedValue);
    VA->eraseFromParent();
  }
  return !Worklist.empty();
}

static Align getSHScalarMemoryAlignment(Type *Ty) {
  if (Ty->isIntegerTy(8))
    return Align(1);
  if (Ty->isIntegerTy(16))
    return Align(2);
  return Align(4);
}

static Value *getAggregateByteAddress(IRBuilder<> &Builder, Value *Base,
                                      uint64_t Offset) {
  return Builder.CreateConstGEP1_64(Builder.getInt8Ty(), Base, Offset);
}

static Value *loadUnalignedAggregateScalar(IRBuilder<> &Builder, Type *Ty,
                                           Value *Base, uint64_t Offset,
                                           bool IsVolatile,
                                           const DataLayout &DL) {
  Type *IntTy = Ty->isPointerTy() ? Builder.getInt32Ty() : Ty;
  unsigned Size = DL.getTypeStoreSize(Ty).getFixedValue();
  Value *Result = ConstantInt::get(IntTy, 0);
  for (unsigned Byte = 0; Byte != Size; ++Byte) {
    Value *Address = getAggregateByteAddress(Builder, Base, Offset + Byte);
    LoadInst *Load =
        Builder.CreateAlignedLoad(Builder.getInt8Ty(), Address, Align(1));
    Load->setVolatile(IsVolatile);
    Value *Piece = Builder.CreateZExt(Load, IntTy);
    unsigned Shift =
        8 *
        (DL.isLittleEndian() ? Byte : static_cast<unsigned>(Size - 1 - Byte));
    if (Shift != 0)
      Piece = Builder.CreateShl(Piece, Shift);
    Result = Builder.CreateOr(Result, Piece);
  }
  if (Ty->isPointerTy())
    return Builder.CreateIntToPtr(Result, Ty);
  return Result;
}

static void storeUnalignedAggregateScalar(IRBuilder<> &Builder,
                                          Value *StoredValue, Value *Base,
                                          uint64_t Offset, bool IsVolatile,
                                          const DataLayout &DL) {
  Type *Ty = StoredValue->getType();
  Value *Bits = Ty->isPointerTy()
                    ? Builder.CreatePtrToInt(StoredValue, Builder.getInt32Ty())
                    : StoredValue;
  unsigned Size = DL.getTypeStoreSize(Ty).getFixedValue();
  for (unsigned Byte = 0; Byte != Size; ++Byte) {
    unsigned Shift =
        8 *
        (DL.isLittleEndian() ? Byte : static_cast<unsigned>(Size - 1 - Byte));
    Value *Piece = Shift == 0 ? Bits : Builder.CreateLShr(Bits, Shift);
    Piece = Builder.CreateTrunc(Piece, Builder.getInt8Ty());
    Value *Address = getAggregateByteAddress(Builder, Base, Offset + Byte);
    StoreInst *Store = Builder.CreateAlignedStore(Piece, Address, Align(1));
    Store->setVolatile(IsVolatile);
  }
}

static Value *loadAggregateScalar(IRBuilder<> &Builder, Type *Ty, Value *Base,
                                  uint64_t Offset, Align BaseAlign,
                                  bool IsVolatile, const DataLayout &DL) {
  Align Alignment = commonAlignment(BaseAlign, Offset);
  if (Alignment < getSHScalarMemoryAlignment(Ty))
    return loadUnalignedAggregateScalar(Builder, Ty, Base, Offset, IsVolatile,
                                        DL);
  Value *Address = getAggregateByteAddress(Builder, Base, Offset);
  LoadInst *Load = Builder.CreateAlignedLoad(Ty, Address, Alignment);
  Load->setVolatile(IsVolatile);
  return Load;
}

static void storeAggregateScalar(IRBuilder<> &Builder, Value *StoredValue,
                                 Value *Base, uint64_t Offset, Align BaseAlign,
                                 bool IsVolatile, const DataLayout &DL) {
  Align Alignment = commonAlignment(BaseAlign, Offset);
  if (Alignment < getSHScalarMemoryAlignment(StoredValue->getType())) {
    storeUnalignedAggregateScalar(Builder, StoredValue, Base, Offset,
                                  IsVolatile, DL);
    return;
  }
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
      loadAggregateScalar(Builder, Ty, Base, Offset, BaseAlign, IsVolatile, DL);
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
  storeAggregateScalar(Builder, Element, Base, Offset, BaseAlign, IsVolatile,
                       DL);
}

static bool lowerAggregateMemory(Function &F) {
  SmallVector<Instruction *, 8> Worklist;
  for (BasicBlock &BB : F)
    for (Instruction &I : BB)
      if ((isa<LoadInst>(I) && I.getType()->isAggregateType()) ||
          (isa<StoreInst>(I) &&
           cast<StoreInst>(I).getValueOperand()->getType()->isAggregateType()))
        Worklist.push_back(&I);

  bool Changed = false;
  const DataLayout &DL = F.getDataLayout();
  for (Instruction *I : Worklist) {
    Type *Ty = isa<LoadInst>(I)
                   ? I->getType()
                   : cast<StoreInst>(I)->getValueOperand()->getType();
    if (!isSupportedSHAggregateElement(Ty))
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

static bool lowerSHMemoryLibcalls(Function &F) {
  SmallVector<MemIntrinsic *, 8> Worklist;
  for (BasicBlock &BB : F)
    for (Instruction &I : BB)
      if (auto *MI = dyn_cast<MemIntrinsic>(&I)) {
        Intrinsic::ID ID = MI->getIntrinsicID();
        if (ID != Intrinsic::memcpy && ID != Intrinsic::memmove &&
            ID != Intrinsic::memset)
          continue;
        const auto *Size = dyn_cast<ConstantInt>(MI->getLength());
        uint64_t InlineLimit = isa<MemMoveInst>(MI) ? 16 : 60;
        bool IsAddressSpaceZero =
            MI->getRawDest()->getType()->getPointerAddressSpace() == 0;
        if (const auto *Transfer = dyn_cast<AnyMemTransferInst>(MI))
          IsAddressSpaceZero &=
              Transfer->getRawSource()->getType()->getPointerAddressSpace() ==
              0;
        if (!MI->isVolatile() && IsAddressSpaceZero &&
            MI->getLength()->getType()->isIntegerTy(32) &&
            (!Size || Size->getZExtValue() > InlineLimit))
          Worklist.push_back(MI);
      }

  if (Worklist.empty())
    return false;
  Module *M = F.getParent();
  LLVMContext &Context = F.getContext();
  Type *PointerTy = PointerType::getUnqual(Context);
  Type *Int32Ty = Type::getInt32Ty(Context);
  for (MemIntrinsic *MI : Worklist) {
    IRBuilder<> Builder(MI);
    Builder.SetCurrentDebugLocation(MI->getDebugLoc());
    FunctionType *FunctionTy;
    SmallVector<Value *, 3> Arguments;
    StringRef Name;
    if (auto *Set = dyn_cast<MemSetInst>(MI)) {
      Name = "memset";
      FunctionTy =
          FunctionType::get(PointerTy, {PointerTy, Int32Ty, Int32Ty}, false);
      Arguments = {Set->getRawDest(),
                   Builder.CreateZExt(Set->getValue(), Int32Ty),
                   Set->getLength()};
    } else {
      auto *Transfer = cast<AnyMemTransferInst>(MI);
      Name = isa<MemCpyInst>(MI) ? "memcpy" : "memmove";
      FunctionTy =
          FunctionType::get(PointerTy, {PointerTy, PointerTy, Int32Ty}, false);
      Arguments = {Transfer->getRawDest(), Transfer->getRawSource(),
                   Transfer->getLength()};
    }
    Builder.CreateCall(M->getOrInsertFunction(Name, FunctionTy), Arguments);
    MI->eraseFromParent();
  }
  return true;
}

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
  bool Changed = lowerSHMemoryLibcalls(F);
  Changed |= lowerSHVarArgs(F);
  Changed |= lowerAggregateMemory(F);
  Changed |= lowerI64StackAlignment(F);
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
