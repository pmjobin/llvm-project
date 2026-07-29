//===-- SHISelLowering.cpp - SH SelectionDAG lowering --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHISelLowering.h"
#include "SH.h"
#include "SHConstantPoolValue.h"
#include "SHMachineFunctionInfo.h"
#include "SHSubtarget.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/CodeGen/Analysis.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicsSH.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

SHTargetLowering::SHTargetLowering(const TargetMachine &TM,
                                   const SHSubtarget &STI)
    : TargetLowering(TM, STI) {
  addRegisterClass(MVT::i32, &SH::GPRRegClass);
  setStackPointerRegisterToSaveRestore(SH::R15);
  setMinFunctionAlignment(Align(2));
  setPrefFunctionAlignment(Align(4));

  for (unsigned Opcode :
       {ISD::ADD, ISD::SUB, ISD::MUL, ISD::XOR, ISD::SHL, ISD::SRA, ISD::SRL})
    setOperationAction(Opcode, MVT::i32, Legal);
  for (unsigned Opcode : {ISD::ADDC, ISD::ADDE, ISD::SUBC, ISD::SUBE})
    setOperationAction(Opcode, MVT::i32, Legal);
  for (unsigned Opcode : {ISD::SHL_PARTS, ISD::SRL_PARTS, ISD::SRA_PARTS})
    setOperationAction(Opcode, MVT::i32, Custom);
  setOperationAction(ISD::SETCC, MVT::i64, Custom);
  setOperationAction(ISD::LOAD, MVT::i64, Custom);
  setOperationAction(ISD::STORE, MVT::i64, Custom);
  setOperationAction(ISD::Constant, MVT::i32, Legal);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i32, Legal);
  for (unsigned Opcode : {ISD::SDIV, ISD::UDIV, ISD::SREM, ISD::UREM})
    setOperationAction(Opcode, MVT::i32, Expand);
  setOperationAction(ISD::SDIVREM, MVT::i32, Custom);
  setOperationAction(ISD::UDIVREM, MVT::i32, Custom);
  // Preserve i64 div/rem in IR until SH can diagnose the unsupported
  // operation instead of letting generic expansion create an apparent
  // supported implementation.
  setMaxDivRemBitWidthSupported(64);

  for (MVT MemVT : {MVT::i8, MVT::i16}) {
    setLoadExtAction(ISD::EXTLOAD, MVT::i32, MemVT, Legal);
    setLoadExtAction(ISD::SEXTLOAD, MVT::i32, MemVT, Legal);
    setLoadExtAction(ISD::ZEXTLOAD, MVT::i32, MemVT, Legal);
    setTruncStoreAction(MVT::i32, MemVT, Legal);
  }

  for (unsigned Opcode :
       {ISD::LOAD, ISD::STORE, ISD::MULHU, ISD::MULHS, ISD::AND, ISD::OR,
        ISD::ROTL, ISD::ROTR, ISD::BR_CC, ISD::SELECT, ISD::SELECT_CC,
        ISD::SETCC, ISD::GlobalAddress, ISD::BlockAddress, ISD::JumpTable,
        ISD::ConstantPool, ISD::GlobalTLSAddress, ISD::DYNAMIC_STACKALLOC})
    setOperationAction(Opcode, MVT::i32, Custom);
  setOperationAction(ISD::BR_JT, MVT::Other, Custom);
  setOperationAction(ISD::VASTART, MVT::Other, Custom);
  setOperationAction(ISD::ATOMIC_FENCE, MVT::Other, Custom);
  setOperationAction(ISD::VAARG, MVT::Other, Expand);
  setOperationAction(ISD::VACOPY, MVT::Other, Expand);
  setOperationAction(ISD::VAEND, MVT::Other, Expand);
  setMinimumJumpTableEntries(4);

  setTargetDAGCombine(ISD::ADD);
  MaxStoresPerMemcpy = MaxStoresPerMemcpyOptSize = 60;
  MaxStoresPerMemmove = MaxStoresPerMemmoveOptSize = 16;
  MaxStoresPerMemset = MaxStoresPerMemsetOptSize = 60;
  setMaxAtomicSizeInBitsSupported(0);
  computeRegisterProperties(STI.getRegisterInfo());
}

static void requireSupportedCallingConvention(CallingConv::ID CallConv) {
  if (CallConv != CallingConv::C)
    report_fatal_error("SH only supports the C calling convention");
}

SDValue SHTargetLowering::PerformDAGCombine(SDNode *N,
                                            DAGCombinerInfo &DCI) const {
  if (N->getOpcode() != ISD::ADD)
    return SDValue();

  SDValue Address = N->getOperand(0);
  SDValue Addend = N->getOperand(1);
  if (!isa<GlobalAddressSDNode>(Address) && isa<GlobalAddressSDNode>(Addend))
    std::swap(Address, Addend);
  const auto *Global = dyn_cast<GlobalAddressSDNode>(Address);
  const auto *Constant = dyn_cast<ConstantSDNode>(Addend);
  if (!Global || !Constant || Address.getOpcode() != ISD::GlobalAddress)
    return SDValue();

  auto [Offset, Overflow] =
      AddOverflow<int64_t>(Global->getOffset(), Constant->getSExtValue());
  if (Overflow || !isInt<32>(Offset))
    return SDValue();

  SelectionDAG &DAG = DCI.DAG;
  return DAG.getGlobalAddress(Global->getGlobal(), SDLoc(N), N->getValueType(0),
                              Offset);
}

static bool isSupportedSHMemoryType(Type *Ty) {
  return Ty->isIntegerTy(8) || Ty->isIntegerTy(16) || Ty->isIntegerTy(32) ||
         Ty->isIntegerTy(64) ||
         (Ty->isPointerTy() && Ty->getPointerAddressSpace() == 0);
}

static bool isSupportedSHScalarType(Type *Ty) {
  return Ty->isIntegerTy(32) || Ty->isIntegerTy(64) ||
         (Ty->isPointerTy() && Ty->getPointerAddressSpace() == 0);
}

static bool isSupportedSHStackType(Type *Ty) {
  if (isSupportedSHMemoryType(Ty))
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

static bool isSupportedSHAggregateType(Type *Ty) {
  return Ty->isAggregateType() && isSupportedSHStackType(Ty);
}

static bool isSupportedSHValueType(Type *Ty) {
  return isSupportedSHScalarType(Ty) || isSupportedSHAggregateType(Ty);
}

static bool isSupportedSHVarArgType(Type *Ty) {
  return Ty->isIntegerTy(32) || Ty->isIntegerTy(64) ||
         (Ty->isPointerTy() && Ty->getPointerAddressSpace() == 0) ||
         isSupportedSHAggregateType(Ty);
}

static uint64_t getFixedSHTypeAllocSize(const DataLayout &DL, Type *Ty) {
  if (!Ty->isSized())
    report_fatal_error("SH aggregate type must have a fixed size");
  TypeSize Size = DL.getTypeAllocSize(Ty);
  if (Size.isScalable())
    report_fatal_error("SH scalable aggregate types are not supported");
  return Size.getFixedValue();
}

static bool isDirectSHAggregateReturn(const DataLayout &DL, Type *Ty) {
  if (!isSupportedSHAggregateType(Ty))
    return false;
  uint64_t Size = getFixedSHTypeAllocSize(DL, Ty);
  Align RequiredAlignment;
  switch (Size) {
  default:
    return false;
  case 1:
    RequiredAlignment = Align(1);
    break;
  case 2:
    RequiredAlignment = Align(2);
    break;
  case 4:
  case 8:
    RequiredAlignment = Align(4);
    break;
  }
  return DL.getABITypeAlign(Ty) >= RequiredAlignment;
}

static void requireSupportedSHByValType(const DataLayout &DL, Type *Ty,
                                        Align Alignment) {
  if (!Ty || !isSupportedSHAggregateType(Ty))
    report_fatal_error(
        "SH calls only support scalar i32, i64, and pointer arguments");
  if (getFixedSHTypeAllocSize(DL, Ty) == 0)
    report_fatal_error("SH zero-sized byval arguments are not supported");
  if (Alignment > Align(4))
    report_fatal_error(
        "SH byval alignment greater than 4 requires unsupported stack "
        "realignment");
}

static void requireSupportedSHSRetType(const DataLayout &DL, Type *Ty) {
  if (!Ty || !isSupportedSHAggregateType(Ty))
    report_fatal_error(
        "SH sret requires a fixed aggregate containing only integers and "
        "address-space-zero pointers");
  if (getFixedSHTypeAllocSize(DL, Ty) == 0)
    report_fatal_error("SH zero-sized sret results are not supported");
}

static void requireSupportedSHMemoryAlignment(Type *Ty, Align Alignment,
                                              bool IsLoad) {
  if (Ty->isIntegerTy(8))
    return;
  if (Ty->isIntegerTy(16)) {
    if (Alignment < Align(2))
      report_fatal_error(
          IsLoad ? "SH requires 2-byte alignment for 16-bit loads"
                 : "SH requires 2-byte alignment for 16-bit stores");
    return;
  }
  if (Alignment < Align(4))
    report_fatal_error(
        IsLoad ? "SH requires 4-byte alignment for 32- and 64-bit loads"
               : "SH requires 4-byte alignment for 32- and 64-bit stores");
}

enum class SHAtomicRuntimeCallKind {
  None,
  Operation,
  CompareExchange,
};

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

static SHAtomicRuntimeCallKind
getSHAtomicRuntimeCallKind(const CallBase &Call) {
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
  if (Name.ends_with("_4"))
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
      (Size->getZExtValue() != 4 && Size->getZExtValue() != 8) ||
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

bool llvm::isSHAtomicCompareExchangeCall(const CallBase &Call) {
  return getSHAtomicRuntimeCallKind(Call) ==
         SHAtomicRuntimeCallKind::CompareExchange;
}

static void validateSHAllocaUses(const AllocaInst &Alloca) {
  SmallVector<const Value *, 8> Worklist(1, &Alloca);
  SmallPtrSet<const Value *, 8> Visited;
  while (!Worklist.empty()) {
    const Value *Pointer = Worklist.pop_back_val();
    if (!Visited.insert(Pointer).second)
      continue;
    for (const User *Use : Pointer->users()) {
      if (const auto *Cast = dyn_cast<BitCastInst>(Use)) {
        if (Cast->getOperand(0) == Pointer) {
          Worklist.push_back(Cast);
          continue;
        }
      }
      if (const auto *GEP = dyn_cast<GetElementPtrInst>(Use)) {
        if (GEP->getPointerOperand() == Pointer) {
          Worklist.push_back(GEP);
          continue;
        }
      }
      if (const auto *Load = dyn_cast<LoadInst>(Use))
        if (Load->getPointerOperand() == Pointer)
          continue;
      if (const auto *Store = dyn_cast<StoreInst>(Use))
        if (Store->getPointerOperand() == Pointer)
          continue;
      if (const auto *Call = dyn_cast<CallBase>(Use)) {
        bool IsSupportedCallUse = false;
        for (unsigned ArgNo = 0; ArgNo != Call->arg_size(); ++ArgNo) {
          if (Call->getArgOperand(ArgNo) != Pointer)
            continue;
          if (isa<MemIntrinsic>(Call) || isa<LifetimeIntrinsic>(Call) ||
              Call->getIntrinsicID() == Intrinsic::vastart ||
              Call->getIntrinsicID() == Intrinsic::vacopy ||
              Call->getIntrinsicID() == Intrinsic::vaend ||
              Call->paramHasAttr(ArgNo, Attribute::ByVal) ||
              Call->paramHasAttr(ArgNo, Attribute::StructRet) ||
              getSHAtomicRuntimeCallKind(*Call) !=
                  SHAtomicRuntimeCallKind::None) {
            IsSupportedCallUse = true;
            break;
          }
        }
        if (IsSupportedCallUse)
          continue;
      }
      report_fatal_error("SH stack object address escape is not supported");
    }
  }
}

void llvm::validateSHAtomics(const Function &F) {
  auto RequireSupportedScope = [](SyncScope::ID Scope) {
    if (Scope != SyncScope::System && Scope != SyncScope::SingleThread)
      report_fatal_error("SH synchronization scope is not supported");
  };
  auto RequireSupportedType = [](Type *Ty, unsigned PointerAddressSpace) {
    bool Supported = Ty->isIntegerTy(32) || Ty->isIntegerTy(64) ||
                     (Ty->isPointerTy() && Ty->getPointerAddressSpace() == 0);
    if (!Supported || PointerAddressSpace != 0)
      report_fatal_error(
          "SH generic atomic operation is not supported for this type");
  };

  for (const BasicBlock &BB : F) {
    for (const Instruction &I : BB) {
      if (const auto *Fence = dyn_cast<FenceInst>(&I)) {
        RequireSupportedScope(Fence->getSyncScopeID());
        continue;
      }
      if (const auto *Load = dyn_cast<LoadInst>(&I); Load && Load->isAtomic()) {
        RequireSupportedScope(Load->getSyncScopeID());
        RequireSupportedType(Load->getType(), Load->getPointerAddressSpace());
        continue;
      }
      if (const auto *Store = dyn_cast<StoreInst>(&I);
          Store && Store->isAtomic()) {
        RequireSupportedScope(Store->getSyncScopeID());
        RequireSupportedType(Store->getValueOperand()->getType(),
                             Store->getPointerAddressSpace());
        continue;
      }
      if (const auto *RMW = dyn_cast<AtomicRMWInst>(&I)) {
        RequireSupportedScope(RMW->getSyncScopeID());
        Type *Ty = RMW->getValOperand()->getType();
        RequireSupportedType(Ty, RMW->getPointerAddressSpace());
        if (Ty->isPointerTy() && RMW->getOperation() != AtomicRMWInst::Xchg)
          report_fatal_error(
              "SH generic atomic operation is not supported for this type");
        continue;
      }
      if (const auto *CAS = dyn_cast<AtomicCmpXchgInst>(&I)) {
        RequireSupportedScope(CAS->getSyncScopeID());
        RequireSupportedType(CAS->getCompareOperand()->getType(),
                             CAS->getPointerAddressSpace());
      }
    }
  }
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

static void validateSHMemoryIntrinsic(const MemIntrinsic &MI) {
  Intrinsic::ID ID = MI.getIntrinsicID();
  if (ID != Intrinsic::memcpy && ID != Intrinsic::memmove &&
      ID != Intrinsic::memset)
    report_fatal_error("SH atomic and inline-only memory intrinsics are not "
                       "supported");
  if (MI.getRawDest()->getType()->getPointerAddressSpace() != 0)
    report_fatal_error("SH memory intrinsics only support address space zero");
  if (const auto *Transfer = dyn_cast<AnyMemTransferInst>(&MI))
    if (Transfer->getRawSource()->getType()->getPointerAddressSpace() != 0)
      report_fatal_error(
          "SH memory intrinsics only support address space zero");
  if (!MI.getLength()->getType()->isIntegerTy(32))
    report_fatal_error("SH memory intrinsic sizes must be i32");
  if (!MI.isVolatile())
    return;
  const auto *Size = dyn_cast<ConstantInt>(MI.getLength());
  if (!Size)
    report_fatal_error("SH volatile memory intrinsics require a constant size");
  uint64_t InlineLimit = ID == Intrinsic::memmove ? 16 : 60;
  if (Size->getZExtValue() > InlineLimit)
    report_fatal_error(
        Twine("SH volatile memory intrinsic size cannot exceed ") +
        Twine(InlineLimit) + " bytes");
}

static bool isSHLanguageEHInstruction(const Instruction &I) {
  return isa<InvokeInst, LandingPadInst, ResumeInst, CatchSwitchInst,
             CatchPadInst, CleanupPadInst, CatchReturnInst, CleanupReturnInst,
             CallBrInst>(I);
}

void llvm::validateSHIR(const Function &F) {
  const DataLayout &DL = F.getDataLayout();
  if (F.hasPersonalityFn())
    report_fatal_error(
        "SH language exception handling is not supported: personality");
  if (F.getReturnType()->isIntegerTy(1))
    report_fatal_error(
        "SH comparison results may only be used by conditional branches");
  if (!F.getReturnType()->isVoidTy() &&
      !isSupportedSHValueType(F.getReturnType()))
    report_fatal_error(
        "SH functions only support void, i32, i64, and pointer return values");
  unsigned SRetCount = 0;
  for (const Argument &Arg : F.args()) {
    if (Arg.hasByValAttr()) {
      if (!Arg.getType()->isPointerTy() ||
          Arg.getType()->getPointerAddressSpace() != 0)
        report_fatal_error(
            "SH byval arguments require address-space-zero pointers");
      requireSupportedSHByValType(
          DL, Arg.getParamByValType(),
          Arg.getParamAlign().value_or(
              DL.getABITypeAlign(Arg.getParamByValType())));
      continue;
    }
    if (Arg.hasStructRetAttr()) {
      ++SRetCount;
      if (!Arg.getType()->isPointerTy() ||
          Arg.getType()->getPointerAddressSpace() != 0)
        report_fatal_error(
            "SH sret arguments require address-space-zero pointers");
      requireSupportedSHSRetType(DL, Arg.getParamStructRetType());
      continue;
    }
    if (!isSupportedSHValueType(Arg.getType()))
      report_fatal_error(
          "SH function arguments must be scalar i32, i64, or pointers");
  }
  if (SRetCount > 1)
    report_fatal_error("SH functions may have only one sret parameter");
  if (F.hasFnAttribute("stackrealign") ||
      (F.getFnStackAlign() && *F.getFnStackAlign() > Align(4)))
    report_fatal_error("SH stack realignment is not supported");
  for (const BasicBlock &BB : F) {
    if (BB.isEHPad())
      report_fatal_error(
          "SH language exception handling is not supported: EH pad");
    for (const Instruction &I : BB) {
      if (isSHLanguageEHInstruction(I))
        report_fatal_error(
            "SH language exception handling is not supported: EH instruction");
      const auto *Call = dyn_cast<CallBase>(&I);
      if (const auto *Switch = dyn_cast<SwitchInst>(&I)) {
        Type *ConditionTy = Switch->getCondition()->getType();
        if (!ConditionTy->isIntegerTy(8) && !ConditionTy->isIntegerTy(16) &&
            !ConditionTy->isIntegerTy(32) && !ConditionTy->isIntegerTy(64))
          report_fatal_error(
              "SH switch conditions must be i8, i16, i32, or i64");
      }
      if (isa<SelectInst>(&I))
        report_fatal_error("SH select is not supported");
      if (isa<AtomicRMWInst>(&I) || isa<AtomicCmpXchgInst>(&I))
        report_fatal_error("SH generic atomic operation survived AtomicExpand");
      if (Call) {
        SHAtomicRuntimeCallKind AtomicCallKind =
            getSHAtomicRuntimeCallKind(*Call);
        requireSupportedCallingConvention(Call->getCallingConv());
        if (Call->isInlineAsm())
          report_fatal_error("SH inline assembly is not supported");
        if (const auto *MI = dyn_cast<MemIntrinsic>(Call)) {
          validateSHMemoryIntrinsic(*MI);
          continue;
        }
        switch (Call->getIntrinsicID()) {
        case Intrinsic::dbg_declare:
        case Intrinsic::dbg_value:
        case Intrinsic::dbg_assign:
        case Intrinsic::dbg_label:
          continue;
        case Intrinsic::sh_tas_b:
          if (Call->getArgOperand(0)->getType()->getPointerAddressSpace() != 0)
            report_fatal_error(
                "SH TAS.B intrinsic requires an address-space-zero pointer");
          continue;
        case Intrinsic::lifetime_start:
        case Intrinsic::lifetime_end:
          continue;
        case Intrinsic::vastart:
          if (!F.isVarArg())
            report_fatal_error("SH va_start requires a variadic function");
          if (llvm::none_of(F.args(), [](const Argument &Arg) {
                return !Arg.hasStructRetAttr();
              }))
            report_fatal_error(
                "SH va_start requires at least one fixed parameter");
          if (Call->getArgOperand(0)->getType()->getPointerAddressSpace() != 0)
            report_fatal_error("SH va_list only supports address space zero");
          continue;
        case Intrinsic::vacopy:
          if (Call->getArgOperand(0)->getType()->getPointerAddressSpace() !=
                  0 ||
              Call->getArgOperand(1)->getType()->getPointerAddressSpace() != 0)
            report_fatal_error("SH va_list only supports address space zero");
          continue;
        case Intrinsic::vaend:
          if (Call->getArgOperand(0)->getType()->getPointerAddressSpace() != 0)
            report_fatal_error("SH va_list only supports address space zero");
          continue;
        case Intrinsic::eh_typeid_for:
        case Intrinsic::eh_return_i32:
        case Intrinsic::eh_return_i64:
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
          report_fatal_error(
              "SH language exception handling is not supported: EH intrinsic");
        default:
          break;
        }
        if (Call->getIntrinsicID() != Intrinsic::not_intrinsic)
          report_fatal_error("SH intrinsics are not supported");
        if (const auto *CallInst = dyn_cast<llvm::CallInst>(Call)) {
          if (CallInst->isMustTailCall())
            report_fatal_error("SH musttail calls are not supported");
          if (CallInst->isTailCall())
            report_fatal_error("SH tail calls are not supported");
        }

        Type *ReturnTy = Call->getType();
        bool IsAtomicBoolean =
            AtomicCallKind == SHAtomicRuntimeCallKind::CompareExchange;
        if (!ReturnTy->isVoidTy() && !isSupportedSHValueType(ReturnTy) &&
            !(IsAtomicBoolean && ReturnTy->isIntegerTy(1)))
          report_fatal_error(
              "SH calls only support void, i32, i64, and pointer return "
              "values");
        unsigned CallSRetCount = 0;
        for (unsigned ArgNo = 0; ArgNo != Call->arg_size(); ++ArgNo) {
          Type *ArgTy = Call->getArgOperand(ArgNo)->getType();
          bool IsUnnamed = Call->getFunctionType()->isVarArg() &&
                           ArgNo >= Call->getFunctionType()->getNumParams();
          if (IsUnnamed && !isSupportedSHVarArgType(ArgTy))
            report_fatal_error(
                "SH variadic arguments must use supported default-promoted "
                "ABI types");
          if (Call->paramHasAttr(ArgNo, Attribute::ByVal)) {
            if (!ArgTy->isPointerTy() || ArgTy->getPointerAddressSpace() != 0)
              report_fatal_error(
                  "SH byval arguments require address-space-zero pointers");
            Type *ByValTy = Call->getParamByValType(ArgNo);
            requireSupportedSHByValType(DL, ByValTy,
                                        Call->getParamAlign(ArgNo).value_or(
                                            DL.getABITypeAlign(ByValTy)));
            continue;
          }
          if (Call->paramHasAttr(ArgNo, Attribute::StructRet)) {
            ++CallSRetCount;
            if (!ArgTy->isPointerTy() || ArgTy->getPointerAddressSpace() != 0)
              report_fatal_error(
                  "SH sret arguments require address-space-zero pointers");
            requireSupportedSHSRetType(DL, Call->getParamStructRetType(ArgNo));
            continue;
          }
          if (!isSupportedSHValueType(ArgTy) ||
              Call->paramHasAttr(ArgNo, Attribute::InAlloca) ||
              Call->paramHasAttr(ArgNo, Attribute::Preallocated) ||
              Call->paramHasAttr(ArgNo, Attribute::ByRef) ||
              Call->paramHasAttr(ArgNo, Attribute::Nest) ||
              Call->paramHasAttr(ArgNo, Attribute::Returned) ||
              Call->paramHasAttr(ArgNo, Attribute::SwiftSelf) ||
              Call->paramHasAttr(ArgNo, Attribute::SwiftAsync) ||
              Call->paramHasAttr(ArgNo, Attribute::SwiftError))
            report_fatal_error(
                "SH calls only support scalar i32, i64, and pointer "
                "arguments");
        }
        if (CallSRetCount > 1)
          report_fatal_error("SH calls may have only one sret parameter");
      }
      if (const auto *Phi = dyn_cast<PHINode>(&I)) {
        if (Phi->getType()->isIntegerTy(1))
          report_fatal_error("SH i1 PHIs are not supported");
        if (!Phi->getType()->isIntegerTy(8) &&
            !Phi->getType()->isIntegerTy(16) &&
            !isSupportedSHValueType(Phi->getType()))
          report_fatal_error(
              "SH only supports fixed integer aggregate, i8, i16, i32, i64, "
              "and pointer PHIs");
      }

      if (const auto *Cmp = dyn_cast<ICmpInst>(&I)) {
        Type *OperandTy = Cmp->getOperand(0)->getType();
        if (OperandTy->isPointerTy() &&
            Cmp->getPredicate() != ICmpInst::ICMP_EQ &&
            Cmp->getPredicate() != ICmpInst::ICMP_NE)
          report_fatal_error(
              "SH only supports pointer equality and inequality comparisons");
        if (!OperandTy->isIntegerTy(8) && !OperandTy->isIntegerTy(16) &&
            !OperandTy->isIntegerTy(32) && !OperandTy->isIntegerTy(64) &&
            !(OperandTy->isPointerTy() &&
              OperandTy->getPointerAddressSpace() == 0))
          report_fatal_error(
              "SH only supports integer and address-space-zero pointer "
              "comparisons");
        for (const User *Use : Cmp->users()) {
          const auto *Branch = dyn_cast<CondBrInst>(Use);
          if (!Branch || Branch->getCondition() != Cmp)
            report_fatal_error(
                "SH comparison results may only be used by conditional "
                "branches");
        }
        continue;
      }

      if (const auto *Cast = dyn_cast<PtrToIntInst>(&I)) {
        if (Cast->getOperand(0)->getType()->getPointerAddressSpace() != 0 ||
            (!Cast->getType()->isIntegerTy(32) &&
             !Cast->getType()->isIntegerTy(64)))
          report_fatal_error(
              "SH ptrtoint only supports address-space-zero pointers and i32 "
              "or i64 results");
        continue;
      }

      if (const auto *Cast = dyn_cast<IntToPtrInst>(&I)) {
        if (Cast->getType()->getPointerAddressSpace() != 0 ||
            !Cast->getOperand(0)->getType()->isIntegerTy(32))
          report_fatal_error("SH inttoptr requires an i32 source and an "
                             "address-space-zero result");
        continue;
      }

      if (const auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
        if (GEP->getPointerAddressSpace() != 0)
          report_fatal_error("SH nonzero address spaces are not supported");
        continue;
      }

      if (const auto *Load = dyn_cast<LoadInst>(&I)) {
        if (Load->getPointerAddressSpace() != 0)
          report_fatal_error("SH nonzero address spaces are not supported");
        if (!isSupportedSHMemoryType(Load->getType()))
          report_fatal_error(
              "SH only supports 8-, 16-, 32-, and 64-bit integer and pointer "
              "loads");
        if (Load->isAtomic())
          report_fatal_error("SH atomic loads are not supported");
        requireSupportedSHMemoryAlignment(Load->getType(), Load->getAlign(),
                                          true);
        continue;
      }

      if (const auto *Store = dyn_cast<StoreInst>(&I)) {
        if (Store->getPointerAddressSpace() != 0)
          report_fatal_error("SH nonzero address spaces are not supported");
        if (!isSupportedSHMemoryType(Store->getValueOperand()->getType()))
          report_fatal_error(
              "SH only supports 8-, 16-, 32-, and 64-bit integer and pointer "
              "stores");
        if (Store->isAtomic())
          report_fatal_error("SH atomic stores are not supported");
        requireSupportedSHMemoryAlignment(Store->getValueOperand()->getType(),
                                          Store->getAlign(), false);
        continue;
      }

      if (const auto *BinOp = dyn_cast<BinaryOperator>(&I)) {
        SmallPtrSet<const Value *, 8> Visited;
        if (isBlockAddressInteger(BinOp->getOperand(0), Visited) ||
            isBlockAddressInteger(BinOp->getOperand(1), Visited))
          report_fatal_error("SH block-address arithmetic is not supported");
        Type *Ty = BinOp->getType();
        if (!Ty->isIntegerTy(8) && !Ty->isIntegerTy(16) &&
            !Ty->isIntegerTy(32) && !Ty->isIntegerTy(64))
          report_fatal_error(
              "SH only supports i8, i16, i32, and selected i64 integer "
              "operations");
        if (Ty->isIntegerTy(64)) {
          switch (BinOp->getOpcode()) {
          default:
            report_fatal_error("SH i64 operation is not supported");
          case Instruction::Add:
          case Instruction::Sub:
          case Instruction::And:
          case Instruction::Or:
          case Instruction::Xor:
          case Instruction::Shl:
          case Instruction::LShr:
          case Instruction::AShr:
            break;
          case Instruction::Mul:
            report_fatal_error("SH i64 multiplication is not supported");
          case Instruction::SDiv:
          case Instruction::UDiv:
          case Instruction::SRem:
          case Instruction::URem:
            report_fatal_error(
                "SH i64 division and remainder are not supported");
          }
        }
        if (Ty->isIntegerTy(8) || Ty->isIntegerTy(16)) {
          switch (BinOp->getOpcode()) {
          default:
            report_fatal_error("SH narrow integer operation is not supported");
          case Instruction::Add:
          case Instruction::Sub:
          case Instruction::And:
          case Instruction::Or:
          case Instruction::Xor:
          case Instruction::Mul:
            break;
          case Instruction::SDiv:
          case Instruction::UDiv:
          case Instruction::SRem:
          case Instruction::URem:
            report_fatal_error(
                "SH narrow integer division and remainder are not supported");
          case Instruction::Shl:
          case Instruction::LShr:
          case Instruction::AShr:
            if (!isa<ConstantInt>(BinOp->getOperand(1)))
              report_fatal_error(
                  "SH variable narrow integer shifts are not supported");
            break;
          }
        }
      }

      const auto *Alloca = dyn_cast<AllocaInst>(&I);
      if (!Alloca)
        continue;
      if (!Alloca->isStaticAlloca())
        report_fatal_error("SH dynamic alloca is not supported");
      const auto *Count = dyn_cast<ConstantInt>(Alloca->getArraySize());
      if (!Count || !isSupportedSHStackType(Alloca->getAllocatedType()))
        report_fatal_error(
            "SH only supports fixed stack objects containing i8, i16, i32, "
            "i64, and pointers");
      if (Alloca->getAlign() > Align(4))
        report_fatal_error("SH stack object alignment cannot exceed 4 bytes");
      validateSHAllocaUses(*Alloca);
    }
  }
}

SmallVector<unsigned, 16> llvm::SH::planMemoryAccesses(uint64_t Size,
                                                       Align Alignment) {
  SmallVector<unsigned, 16> Widths;
  for (uint64_t Offset = 0; Offset != Size;) {
    uint64_t Remaining = Size - Offset;
    Align EffectiveAlignment = commonAlignment(Alignment, Offset);
    unsigned Width = Remaining >= 4 && EffectiveAlignment >= Align(4)   ? 4
                     : Remaining >= 2 && EffectiveAlignment >= Align(2) ? 2
                                                                        : 1;
    Widths.push_back(Width);
    Offset += Width;
  }
  return Widths;
}

namespace {

constexpr MCRegister SHArgumentRegisters[] = {SH::R4, SH::R5, SH::R6, SH::R7};

struct SHABIWord {
  unsigned ByteOffset;
  unsigned ValidBytes;
  unsigned WordIndex;
  MCRegister Reg;
  int64_t StackOffset;

  bool isRegister() const { return Reg != 0; }
};

struct SHABIValue {
  uint64_t Size;
  SmallVector<SHABIWord, 4> Words;
};

class SHABIState {
  unsigned NextRegister = 0;
  unsigned StackSize = 0;

public:
  SHABIValue allocate(uint64_t Size) {
    if (Size == 0)
      report_fatal_error("SH zero-sized aggregate arguments are not supported");
    if (Size > UINT_MAX)
      report_fatal_error("SH aggregate argument is too large");

    SHABIValue Value{Size, {}};
    unsigned WordCount = divideCeil(static_cast<unsigned>(Size), 4u);
    for (unsigned WordIndex = 0; WordIndex != WordCount; ++WordIndex) {
      unsigned ByteOffset = WordIndex * 4;
      unsigned ValidBytes =
          std::min<unsigned>(4, static_cast<unsigned>(Size) - ByteOffset);
      if (NextRegister != std::size(SHArgumentRegisters)) {
        Value.Words.push_back({ByteOffset, ValidBytes, WordIndex,
                               SHArgumentRegisters[NextRegister++], -1});
        continue;
      }
      Value.Words.push_back({ByteOffset, ValidBytes, WordIndex, 0, StackSize});
      if (StackSize > UINT_MAX - 4)
        report_fatal_error("SH aggregate argument area is too large");
      StackSize += 4;
    }
    return Value;
  }

  unsigned getStackSize() const { return StackSize; }
  unsigned getRegisterCursor() const { return NextRegister; }
};

struct SHValuePart {
  unsigned ByteOffset;
  unsigned ValidBytes;
};

static SmallVector<SHValuePart, 4>
getSHValueParts(const SHTargetLowering &TLI, const DataLayout &DL, Type *Ty) {
  SmallVector<EVT, 4> ValueVTs;
  SmallVector<uint64_t, 4> Offsets;
  ComputeValueVTs(TLI, DL, Ty, ValueVTs, nullptr, &Offsets, 0);
  SmallVector<SHValuePart, 4> Parts;
  for (auto [ValueVT, Offset] : zip_equal(ValueVTs, Offsets)) {
    if (!ValueVT.isInteger())
      report_fatal_error(
          "SH aggregate values may contain only integers and pointers");
    uint64_t StoreSize = ValueVT.getStoreSize().getFixedValue();
    unsigned NumParts = TLI.getNumRegistersForCallingConv(
        Ty->getContext(), CallingConv::C, ValueVT);
    for (unsigned Part = 0; Part != NumParts; ++Part) {
      unsigned PartOffset = Part * 4;
      Parts.push_back({static_cast<unsigned>(Offset + PartOffset),
                       std::min<unsigned>(4, static_cast<unsigned>(StoreSize) -
                                                 PartOffset)});
    }
  }
  return Parts;
}

static SDValue getSHByte(SelectionDAG &DAG, const SDLoc &DL, SDValue Value,
                         unsigned Shift) {
  if (Shift != 0)
    Value = DAG.getNode(ISD::SRL, DL, MVT::i32, Value,
                        DAG.getConstant(Shift, DL, MVT::i32));
  return DAG.getNode(ISD::AND, DL, MVT::i32, Value,
                     DAG.getConstant(0xff, DL, MVT::i32));
}

static SDValue putSHByte(SelectionDAG &DAG, const SDLoc &DL, SDValue Result,
                         SDValue Byte, unsigned Shift) {
  if (Shift != 0)
    Byte = DAG.getNode(ISD::SHL, DL, MVT::i32, Byte,
                       DAG.getConstant(Shift, DL, MVT::i32));
  return DAG.getNode(ISD::OR, DL, MVT::i32, Result, Byte);
}

static unsigned getSHByteShift(bool IsLittleEndian, unsigned Width,
                               unsigned Byte) {
  return 8 * (IsLittleEndian ? Byte : Width - 1 - Byte);
}

static SmallVector<SDValue, 4>
packSHABIValue(SelectionDAG &DAG, const SDLoc &DL, const SHABIValue &ABIValue,
               ArrayRef<SHValuePart> Parts, ArrayRef<SDValue> Values) {
  if (Parts.size() != Values.size())
    report_fatal_error("SH aggregate ABI part count is inconsistent");
  bool IsLittleEndian = DAG.getDataLayout().isLittleEndian();
  SmallVector<SDValue, 4> Words(ABIValue.Words.size(),
                                DAG.getConstant(0, DL, MVT::i32));
  for (unsigned PartIndex = 0; PartIndex != Parts.size(); ++PartIndex) {
    const SHValuePart &Part = Parts[PartIndex];
    if (Values[PartIndex].getValueType() != MVT::i32)
      report_fatal_error("SH aggregate ABI parts must be 32-bit values");
    for (unsigned Byte = 0; Byte != Part.ValidBytes; ++Byte) {
      unsigned AggregateByte = Part.ByteOffset + Byte;
      unsigned WordIndex = AggregateByte / 4;
      unsigned WordByte = AggregateByte % 4;
      if (WordIndex >= ABIValue.Words.size() ||
          WordByte >= ABIValue.Words[WordIndex].ValidBytes)
        report_fatal_error("SH aggregate ABI part exceeds its object");
      SDValue Piece =
          getSHByte(DAG, DL, Values[PartIndex],
                    getSHByteShift(IsLittleEndian, Part.ValidBytes, Byte));
      Words[WordIndex] = putSHByte(
          DAG, DL, Words[WordIndex], Piece,
          getSHByteShift(IsLittleEndian, ABIValue.Words[WordIndex].ValidBytes,
                         WordByte));
    }
  }
  return Words;
}

static SmallVector<SDValue, 4>
unpackSHABIValue(SelectionDAG &DAG, const SDLoc &DL, const SHABIValue &ABIValue,
                 ArrayRef<SHValuePart> Parts, ArrayRef<SDValue> Words) {
  if (ABIValue.Words.size() != Words.size())
    report_fatal_error("SH aggregate ABI word count is inconsistent");
  bool IsLittleEndian = DAG.getDataLayout().isLittleEndian();
  SmallVector<SDValue, 4> Values;
  for (const SHValuePart &Part : Parts) {
    SDValue Value = DAG.getConstant(0, DL, MVT::i32);
    for (unsigned Byte = 0; Byte != Part.ValidBytes; ++Byte) {
      unsigned AggregateByte = Part.ByteOffset + Byte;
      unsigned WordIndex = AggregateByte / 4;
      unsigned WordByte = AggregateByte % 4;
      if (WordIndex >= ABIValue.Words.size() ||
          WordByte >= ABIValue.Words[WordIndex].ValidBytes)
        report_fatal_error("SH aggregate ABI part exceeds its object");
      SDValue Piece = getSHByte(
          DAG, DL, Words[WordIndex],
          getSHByteShift(IsLittleEndian, ABIValue.Words[WordIndex].ValidBytes,
                         WordByte));
      Value = putSHByte(DAG, DL, Value, Piece,
                        getSHByteShift(IsLittleEndian, Part.ValidBytes, Byte));
    }
    Values.push_back(Value);
  }
  return Values;
}

static SDValue getSHMemoryAddress(SelectionDAG &DAG, const SDLoc &DL,
                                  SDValue Base, uint64_t Offset) {
  if (Offset == 0)
    return Base;
  return DAG.getNode(ISD::ADD, DL, MVT::i32, Base,
                     DAG.getIntPtrConstant(Offset, DL));
}

static SDValue loadSHABIWord(SelectionDAG &DAG, const SDLoc &DL, SDValue Chain,
                             SDValue Base, MachinePointerInfo PointerInfo,
                             Align Alignment, unsigned ByteOffset,
                             unsigned ValidBytes,
                             SmallVectorImpl<SDValue> &Chains) {
  bool IsLittleEndian = DAG.getDataLayout().isLittleEndian();
  SDValue Result = DAG.getConstant(0, DL, MVT::i32);
  unsigned Offset = 0;
  for (unsigned Width : SH::planMemoryAccesses(
           ValidBytes, commonAlignment(Alignment, ByteOffset))) {
    SDValue Address = getSHMemoryAddress(DAG, DL, Base, ByteOffset + Offset);
    SDValue Load;
    if (Width == 4)
      Load = DAG.getLoad(MVT::i32, DL, Chain, Address,
                         PointerInfo.getWithOffset(ByteOffset + Offset),
                         commonAlignment(Alignment, ByteOffset + Offset));
    else
      Load = DAG.getExtLoad(ISD::ZEXTLOAD, DL, MVT::i32, Chain, Address,
                            PointerInfo.getWithOffset(ByteOffset + Offset),
                            Width == 2 ? MVT::i16 : MVT::i8,
                            commonAlignment(Alignment, ByteOffset + Offset));
    Chains.push_back(Load.getValue(1));
    for (unsigned Byte = 0; Byte != Width; ++Byte) {
      SDValue Piece =
          getSHByte(DAG, DL, Load, getSHByteShift(IsLittleEndian, Width, Byte));
      Result =
          putSHByte(DAG, DL, Result, Piece,
                    getSHByteShift(IsLittleEndian, ValidBytes, Offset + Byte));
    }
    Offset += Width;
  }
  return Result;
}

static SDValue getSHMemoryChunk(SelectionDAG &DAG, const SDLoc &DL,
                                SDValue Word, unsigned WordWidth,
                                unsigned ByteOffset, unsigned Width) {
  bool IsLittleEndian = DAG.getDataLayout().isLittleEndian();
  SDValue Result = DAG.getConstant(0, DL, MVT::i32);
  for (unsigned Byte = 0; Byte != Width; ++Byte) {
    SDValue Piece =
        getSHByte(DAG, DL, Word,
                  getSHByteShift(IsLittleEndian, WordWidth, ByteOffset + Byte));
    Result = putSHByte(DAG, DL, Result, Piece,
                       getSHByteShift(IsLittleEndian, Width, Byte));
  }
  return Result;
}

static void storeSHABIWord(SelectionDAG &DAG, const SDLoc &DL, SDValue Chain,
                           SDValue Base, MachinePointerInfo PointerInfo,
                           Align Alignment, unsigned ByteOffset,
                           unsigned ValidBytes, SDValue Word,
                           SmallVectorImpl<SDValue> &Chains) {
  unsigned Offset = 0;
  for (unsigned Width : SH::planMemoryAccesses(
           ValidBytes, commonAlignment(Alignment, ByteOffset))) {
    SDValue Address = getSHMemoryAddress(DAG, DL, Base, ByteOffset + Offset);
    SDValue Chunk = getSHMemoryChunk(DAG, DL, Word, ValidBytes, Offset, Width);
    Align EffectiveAlignment = commonAlignment(Alignment, ByteOffset + Offset);
    if (Width == 4)
      Chains.push_back(DAG.getStore(
          Chain, DL, Chunk, Address,
          PointerInfo.getWithOffset(ByteOffset + Offset), EffectiveAlignment));
    else
      Chains.push_back(DAG.getTruncStore(
          Chain, DL, Chunk, Address,
          PointerInfo.getWithOffset(ByteOffset + Offset),
          Width == 2 ? MVT::i16 : MVT::i8, EffectiveAlignment));
    Offset += Width;
  }
}

static uint64_t getSHABITypeSize(const DataLayout &DL, Type *Ty) {
  if (Ty->isIntegerTy(32) ||
      (Ty->isPointerTy() && Ty->getPointerAddressSpace() == 0))
    return 4;
  if (Ty->isIntegerTy(64))
    return 8;
  if (isSupportedSHAggregateType(Ty))
    return getFixedSHTypeAllocSize(DL, Ty);
  report_fatal_error("SH ABI value type is not supported");
}

} // namespace

bool SHTargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context,
    const Type *RetTy) const {
  (void)Context;
  if (CallConv != CallingConv::C)
    return false;
  if (RetTy->isIntegerTy(1))
    return Outs.size() == 1 && Outs[0].VT == MVT::i32 && Outs[0].Flags.isZExt();
  if (RetTy->isVoidTy())
    return Outs.empty();
  if (RetTy->isAggregateType() &&
      !isDirectSHAggregateReturn(MF.getDataLayout(), const_cast<Type *>(RetTy)))
    return false;
  if (!isSupportedSHValueType(const_cast<Type *>(RetTy)))
    return false;
  SmallVector<SHValuePart, 4> Parts =
      getSHValueParts(*this, MF.getDataLayout(), const_cast<Type *>(RetTy));
  return Outs.size() == Parts.size() &&
         llvm::all_of(Outs, [](const ISD::OutputArg &Out) {
           return Out.VT == MVT::i32 && !Out.Flags.isByVal() &&
                  !Out.Flags.isSRet();
         });
}

SDValue SHTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  requireSupportedCallingConvention(CallConv);
  MachineFunction &MF = DAG.getMachineFunction();
  const Function &F = MF.getFunction();
  const DataLayout &DataLayout = DAG.getDataLayout();
  validateSHIR(F);

  MachineRegisterInfo &MRI = MF.getRegInfo();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  SHMachineFunctionInfo &FuncInfo = *MF.getInfo<SHMachineFunctionInfo>();
  SHABIState ABIState;
  SmallVector<SDValue, 8> ABIValues(Ins.size());
  SmallVector<SDValue, 8> ArgChains;

  auto copyFromRegister = [&](MCRegister Reg) {
    Register VReg = MRI.createVirtualRegister(&SH::GPRRegClass);
    MRI.addLiveIn(Reg, VReg);
    SDValue Value = DAG.getCopyFromReg(Chain, DL, VReg, MVT::i32);
    ArgChains.push_back(Value.getValue(1));
    return Value;
  };

  for (unsigned I = 0; I != Ins.size(); ++I) {
    if (Ins[I].isOrigArg() || !Ins[I].Flags.isSRet())
      continue;
    if (FuncInfo.getSRetReturnReg())
      report_fatal_error("SH functions may have only one sret parameter");
    SDValue Pointer = copyFromRegister(SH::R2);
    ABIValues[I] = Pointer;
    Register SavedPointer = MRI.createVirtualRegister(&SH::GPRRegClass);
    Chain = DAG.getCopyToReg(Chain, DL, SavedPointer, Pointer);
    ArgChains.push_back(Chain);
    FuncInfo.setSRetReturnReg(SavedPointer);
  }

  for (const Argument &Arg : F.args()) {
    SmallVector<unsigned, 4> ArgIns;
    for (unsigned I = 0; I != Ins.size(); ++I)
      if (Ins[I].isOrigArg() && Ins[I].getOrigArgIndex() == Arg.getArgNo())
        ArgIns.push_back(I);
    if (ArgIns.empty())
      report_fatal_error("SH failed to describe a formal argument");
    for (unsigned I : ArgIns)
      if (Ins[I].VT != MVT::i32 || Ins[I].Flags.isByRef() ||
          Ins[I].Flags.isInAlloca() || Ins[I].Flags.isPreallocated() ||
          Ins[I].Flags.isNest() || Ins[I].Flags.isReturned() ||
          Ins[I].Flags.isSwiftSelf() || Ins[I].Flags.isSwiftAsync() ||
          Ins[I].Flags.isSwiftError())
        report_fatal_error("SH formal argument has unsupported ABI flags");

    if (Arg.hasStructRetAttr()) {
      if (ArgIns.size() != 1 || !Ins[ArgIns[0]].Flags.isSRet())
        report_fatal_error("SH sret arguments must be one 32-bit pointer");
      if (FuncInfo.getSRetReturnReg())
        report_fatal_error("SH functions may have only one sret parameter");
      SDValue Pointer = copyFromRegister(SH::R2);
      ABIValues[ArgIns[0]] = Pointer;
      Register SavedPointer = MRI.createVirtualRegister(&SH::GPRRegClass);
      Chain = DAG.getCopyToReg(Chain, DL, SavedPointer, Pointer);
      ArgChains.push_back(Chain);
      FuncInfo.setSRetReturnReg(SavedPointer);
      continue;
    }

    Type *ABIType =
        Arg.hasByValAttr() ? Arg.getParamByValType() : Arg.getType();
    uint64_t Size = getSHABITypeSize(DataLayout, ABIType);
    SHABIValue ABIValue = ABIState.allocate(Size);

    if (Arg.hasByValAttr()) {
      if (ArgIns.size() != 1 || !Ins[ArgIns[0]].Flags.isByVal() ||
          Ins[ArgIns[0]].Flags.getByValSize() != Size)
        report_fatal_error("SH byval argument size is inconsistent");
      Align Alignment = Ins[ArgIns[0]].Flags.getNonZeroByValAlign();
      requireSupportedSHByValType(DataLayout, ABIType, Alignment);

      // Keep addressable named values below the target-owned variadic and PR
      // areas instead of constructing a fixed register/stack bridge across
      // them.
      if (F.isVarArg()) {
        int FI = MFI.CreateStackObject(Size, Alignment, /*isSpillSlot=*/false);
        MFI.setIsAliasedObjectIndex(FI, true);
        SDValue LocalBase = DAG.getFrameIndex(FI, MVT::i32);
        SmallVector<SDValue, 8> TransferChains;
        for (const SHABIWord &Word : ABIValue.Words) {
          SDValue Value;
          if (Word.isRegister()) {
            Value = copyFromRegister(Word.Reg);
          } else {
            int64_t StackOffset = Word.StackOffset;
            if (!DataLayout.isLittleEndian() && Size < 4 &&
                ABIValue.Words.size() == 1)
              StackOffset += 4 - Size;
            Align StackAlignment = commonAlignment(Align(4), StackOffset);
            int SourceFI = MFI.CreateFixedObject(Word.ValidBytes, StackOffset,
                                                 /*IsImmutable=*/true);
            MFI.setObjectAlignment(SourceFI, StackAlignment);
            SDValue SourceBase = DAG.getFrameIndex(SourceFI, MVT::i32);
            Value = loadSHABIWord(
                DAG, DL, Chain, SourceBase,
                MachinePointerInfo::getFixedStack(MF, SourceFI), StackAlignment,
                0, Word.ValidBytes, TransferChains);
          }
          storeSHABIWord(DAG, DL, Chain, LocalBase,
                         MachinePointerInfo::getStack(MF, 0), Alignment,
                         Word.ByteOffset, Word.ValidBytes, Value,
                         TransferChains);
        }
        ArgChains.append(TransferChains);
        ABIValues[ArgIns[0]] = LocalBase;
        continue;
      }

      unsigned RegisterWords =
          llvm::count_if(ABIValue.Words, [](const SHABIWord &Word) {
            return Word.isRegister();
          });
      int64_t ObjectOffset;
      if (RegisterWords != 0) {
        MCRegister FirstRegister = ABIValue.Words.front().Reg;
        auto RegisterPosition =
            llvm::find(ArrayRef(SHArgumentRegisters), FirstRegister);
        if (RegisterPosition == std::end(SHArgumentRegisters))
          report_fatal_error("SH byval argument register is invalid");
        unsigned RegisterIndex =
            std::distance(std::begin(SHArgumentRegisters), RegisterPosition);
        ObjectOffset = -static_cast<int64_t>(
            (std::size(SHArgumentRegisters) - RegisterIndex) * 4);
      } else {
        ObjectOffset = ABIValue.Words.front().StackOffset;
        if (!DataLayout.isLittleEndian() && Size < 4)
          ObjectOffset += 4 - Size;
      }
      int FI = MFI.CreateFixedObject(Size, ObjectOffset, /*IsImmutable=*/false,
                                     /*IsAliased=*/true);
      MFI.setObjectAlignment(FI, Alignment);
      SDValue FrameIndex = DAG.getFrameIndex(FI, MVT::i32);
      SmallVector<SDValue, 4> StoreChains;
      for (const SHABIWord &Word : ABIValue.Words) {
        if (!Word.isRegister())
          continue;
        SDValue Value = copyFromRegister(Word.Reg);
        storeSHABIWord(DAG, DL, Chain, FrameIndex,
                       MachinePointerInfo::getFixedStack(MF, FI), Alignment,
                       Word.ByteOffset, Word.ValidBytes, Value, StoreChains);
      }
      ArgChains.append(StoreChains);
      ABIValues[ArgIns[0]] = FrameIndex;
      continue;
    }

    if (llvm::any_of(ArgIns, [&](unsigned I) {
          return Ins[I].Flags.isByVal() || Ins[I].Flags.isSRet();
        }))
      report_fatal_error("SH direct argument has indirect ABI flags");
    SmallVector<SDValue, 4> Words;
    for (const SHABIWord &Word : ABIValue.Words) {
      if (Word.isRegister()) {
        Words.push_back(copyFromRegister(Word.Reg));
        continue;
      }
      int64_t StackOffset = Word.StackOffset;
      if (!DataLayout.isLittleEndian() && Size < 4 &&
          ABIValue.Words.size() == 1)
        StackOffset += 4 - Size;
      Align Alignment = commonAlignment(Align(4), StackOffset);
      int FI = MFI.CreateFixedObject(Word.ValidBytes, StackOffset,
                                     /*IsImmutable=*/true);
      MFI.setObjectAlignment(FI, Alignment);
      SDValue FrameIndex = DAG.getFrameIndex(FI, MVT::i32);
      SDValue LoadBase = FrameIndex;
      if (Word.ValidBytes != 4) {
        Register AddressReg = MRI.createVirtualRegister(&SH::GPRRegClass);
        SDValue AddressCopy =
            DAG.getCopyToReg(Chain, DL, AddressReg, FrameIndex);
        ArgChains.push_back(AddressCopy);
        LoadBase = DAG.getCopyFromReg(AddressCopy, DL, AddressReg, MVT::i32);
        ArgChains.push_back(LoadBase.getValue(1));
      }
      SmallVector<SDValue, 2> LoadChains;
      Words.push_back(loadSHABIWord(DAG, DL, Chain, LoadBase,
                                    MachinePointerInfo::getFixedStack(MF, FI),
                                    Alignment, 0, Word.ValidBytes, LoadChains));
      ArgChains.append(LoadChains);
    }
    SmallVector<SHValuePart, 4> Parts =
        getSHValueParts(*this, DataLayout, Arg.getType());
    if (Parts.size() != ArgIns.size())
      report_fatal_error("SH formal argument part count is inconsistent");
    SmallVector<SDValue, 4> Values;
    if (Arg.getType()->isAggregateType())
      Values = unpackSHABIValue(DAG, DL, ABIValue, Parts, Words);
    else
      Values = Words;
    for (auto [Index, Value] : zip_equal(ArgIns, Values))
      ABIValues[Index] = Value;
  }

  if (ABIState.getStackSize() > 60)
    report_fatal_error("SH incoming argument area cannot exceed 60 bytes");
  if (IsVarArg || F.isVarArg()) {
    unsigned FirstUnnamedRegister = ABIState.getRegisterCursor();
    unsigned FirstStackOffset = ABIState.getStackSize();
    unsigned SaveSize = 0;
    int VarArgsFI;
    if (FirstUnnamedRegister < std::size(SHArgumentRegisters)) {
      SaveSize = (std::size(SHArgumentRegisters) - FirstUnnamedRegister) * 4;
      VarArgsFI = MFI.CreateFixedObject(SaveSize, -static_cast<int>(SaveSize),
                                        /*IsImmutable=*/false);
      MFI.setObjectAlignment(VarArgsFI, Align(4));
      SDValue SaveBase = DAG.getFrameIndex(VarArgsFI, MVT::i32);
      SmallVector<SDValue, 4> SaveChains;
      for (unsigned I = FirstUnnamedRegister;
           I != std::size(SHArgumentRegisters); ++I) {
        SDValue Value = copyFromRegister(SHArgumentRegisters[I]);
        storeSHABIWord(DAG, DL, Chain, SaveBase,
                       MachinePointerInfo::getFixedStack(MF, VarArgsFI),
                       Align(4), (I - FirstUnnamedRegister) * 4, 4, Value,
                       SaveChains);
      }
      ArgChains.append(SaveChains);
    } else {
      VarArgsFI = MFI.CreateFixedObject(4, FirstStackOffset,
                                        /*IsImmutable=*/true);
      MFI.setObjectAlignment(VarArgsFI, Align(4));
    }
    FuncInfo.setVarArgsSaveArea(VarArgsFI, SaveSize);
  }
  if (!ArgChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, ArgChains);
  for (SDValue Value : ABIValues) {
    if (!Value)
      report_fatal_error("SH failed to assign an incoming ABI value");
    InVals.push_back(Value);
  }
  return Chain;
}

SDValue
SHTargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                              bool IsVarArg,
                              const SmallVectorImpl<ISD::OutputArg> &Outs,
                              const SmallVectorImpl<SDValue> &OutVals,
                              const SDLoc &DL, SelectionDAG &DAG) const {
  requireSupportedCallingConvention(CallConv);
  const Function &F = DAG.getMachineFunction().getFunction();
  if (Outs.size() != OutVals.size())
    report_fatal_error("SH return value part count is inconsistent");
  MachineFunction &MF = DAG.getMachineFunction();
  SHMachineFunctionInfo &FuncInfo = *MF.getInfo<SHMachineFunctionInfo>();
  bool HasSRet = FuncInfo.getSRetReturnReg().isValid();
  Type *ReturnTy = F.getReturnType();
  if (ReturnTy->isAggregateType() &&
      !isDirectSHAggregateReturn(DAG.getDataLayout(), ReturnTy)) {
    if (!Outs.empty() || !HasSRet)
      report_fatal_error("SH indirect aggregate return is missing sret");
  } else if (ReturnTy->isVoidTy()) {
    if (!Outs.empty())
      report_fatal_error("SH void returns cannot have values");
  } else {
    SmallVector<SHValuePart, 4> Parts =
        getSHValueParts(*this, DAG.getDataLayout(), ReturnTy);
    if (Outs.size() != Parts.size() ||
        llvm::any_of(Outs, [](const ISD::OutputArg &Out) {
          return Out.VT != MVT::i32 || Out.Flags.isByVal() ||
                 Out.Flags.isSRet();
        }))
      report_fatal_error("SH direct return ABI parts are inconsistent");
  }
  if (HasSRet && !Outs.empty())
    report_fatal_error("SH cannot combine direct and indirect returns");

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);
  if (!Outs.empty()) {
    SHABIState ReturnState;
    SHABIValue ABIValue =
        ReturnState.allocate(getSHABITypeSize(DAG.getDataLayout(), ReturnTy));
    SmallVector<SHValuePart, 4> Parts =
        getSHValueParts(*this, DAG.getDataLayout(), ReturnTy);
    SmallVector<SDValue, 4> Words;
    if (ReturnTy->isAggregateType())
      Words = packSHABIValue(DAG, DL, ABIValue, Parts, OutVals);
    else
      Words.assign(OutVals.begin(), OutVals.end());
    if (Words.size() > 2)
      report_fatal_error("SH direct aggregate return exceeds r0-r1");
    for (unsigned I = 0; I != Words.size(); ++I) {
      MCRegister Reg = I == 0 ? SH::R0 : SH::R1;
      Chain = DAG.getCopyToReg(Chain, DL, Reg, Words[I], Glue);
      Glue = Chain.getValue(1);
      RetOps.push_back(DAG.getRegister(Reg, MVT::i32));
    }
  } else if (HasSRet) {
    SDValue Pointer = DAG.getCopyFromReg(Chain, DL, FuncInfo.getSRetReturnReg(),
                                         MVT::i32, Glue);
    Chain = Pointer.getValue(1);
    Glue = Pointer.getValue(2);
    Chain = DAG.getCopyToReg(Chain, DL, SH::R0, Pointer, Glue);
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(SH::R0, MVT::i32));
  }
  RetOps[0] = Chain;
  if (Glue)
    RetOps.push_back(Glue);
  return DAG.getNode(SHISD::RET_GLUE, DL, MVT::Other, RetOps);
}

SDValue SHTargetLowering::LowerCall(CallLoweringInfo &CLI,
                                    SmallVectorImpl<SDValue> &InVals) const {
  requireSupportedCallingConvention(CLI.CallConv);
  if (CLI.CB && isa<InvokeInst>(CLI.CB))
    report_fatal_error("SH exception-handling calls are not supported");
  if (CLI.CB && isa<InlineAsm>(CLI.CB->getCalledOperand()))
    report_fatal_error("SH inline assembly is not supported");
  if (CLI.CB && CLI.CB->isMustTailCall())
    report_fatal_error("SH musttail calls are not supported");
  if (const auto *Call = dyn_cast_or_null<CallInst>(CLI.CB);
      Call && Call->isTailCall())
    report_fatal_error("SH tail calls are not supported");
  CLI.IsTailCall = false;

  Type *ReturnTy = CLI.OrigRetTy;
  if (!ReturnTy)
    report_fatal_error("SH call is missing its return type");
  bool IsAtomicBooleanResult = CLI.CB && ReturnTy->isIntegerTy(1) &&
                               isSHAtomicCompareExchangeCall(*CLI.CB);
  if (ReturnTy->isAggregateType() &&
      !isDirectSHAggregateReturn(CLI.DAG.getDataLayout(), ReturnTy))
    report_fatal_error("SH indirect aggregate call result is missing sret");
  if (!ReturnTy->isVoidTy() && !isSupportedSHValueType(ReturnTy) &&
      !IsAtomicBooleanResult)
    report_fatal_error(
        "SH calls only support void, i32, i64, and pointer return values");
  SmallVector<SHValuePart, 4> ReturnParts;
  if (!ReturnTy->isVoidTy() && !IsAtomicBooleanResult)
    ReturnParts = getSHValueParts(*this, CLI.DAG.getDataLayout(), ReturnTy);
  if ((IsAtomicBooleanResult ? 1 : ReturnParts.size()) != CLI.Ins.size() ||
      llvm::any_of(CLI.Ins,
                   [](const ISD::InputArg &In) { return In.VT != MVT::i32; }))
    report_fatal_error("SH call result ABI parts are inconsistent");
  if (CLI.OutVals.size() != CLI.Outs.size())
    report_fatal_error("SH call argument part count is inconsistent");

  SelectionDAG &DAG = CLI.DAG;
  MachineFunction &MF = DAG.getMachineFunction();
  const DataLayout &DataLayout = DAG.getDataLayout();
  struct SHCallArgument {
    const ArgListEntry *Arg;
    SHABIValue ABIValue;
    SmallVector<unsigned, 4> PartIndices;
    bool IsSRet;
  };
  SmallVector<SHCallArgument, 8> CallArguments;
  SHABIState ABIState;
  unsigned NumFixedArgs = CLI.CB && CLI.CB->getFunctionType()->isVarArg()
                              ? CLI.CB->getFunctionType()->getNumParams()
                              : CLI.Args.size();
  for (unsigned ArgIndex = 0; ArgIndex != CLI.Args.size(); ++ArgIndex) {
    const ArgListEntry &Arg = CLI.Args[ArgIndex];
    if (CLI.IsVarArg && ArgIndex >= NumFixedArgs &&
        (!Arg.OrigTy || !isSupportedSHVarArgType(Arg.OrigTy)))
      report_fatal_error(
          "SH variadic arguments must use supported default-promoted ABI "
          "types");
    if (!Arg.OrigTy || Arg.IsInAlloca || Arg.IsPreallocated || Arg.IsByRef ||
        Arg.IsNest || Arg.IsReturned || Arg.IsSwiftSelf || Arg.IsSwiftAsync ||
        Arg.IsSwiftError)
      report_fatal_error("SH call argument has unsupported ABI flags");
    SmallVector<unsigned, 4> PartIndices;
    for (unsigned I = 0; I != CLI.Outs.size(); ++I)
      if (CLI.Outs[I].OrigArgIndex == ArgIndex)
        PartIndices.push_back(I);
    if (PartIndices.empty())
      report_fatal_error("SH call argument has no ABI parts");
    for (unsigned I : PartIndices)
      if (CLI.Outs[I].VT != MVT::i32 || CLI.Outs[I].Flags.isByRef() ||
          CLI.Outs[I].Flags.isInAlloca() ||
          CLI.Outs[I].Flags.isPreallocated() || CLI.Outs[I].Flags.isNest() ||
          CLI.Outs[I].Flags.isReturned() || CLI.Outs[I].Flags.isSwiftSelf() ||
          CLI.Outs[I].Flags.isSwiftAsync() || CLI.Outs[I].Flags.isSwiftError())
        report_fatal_error("SH call argument ABI part is not supported");

    if (Arg.IsSRet) {
      if (PartIndices.size() != 1 || !CLI.Outs[PartIndices[0]].Flags.isSRet() ||
          CLI.Outs[PartIndices[0]].Flags.isByVal())
        report_fatal_error("SH sret call argument must be one pointer");
      CallArguments.push_back({&Arg, {0, {}}, std::move(PartIndices), true});
      continue;
    }

    Type *ABIType = Arg.IsByVal ? Arg.IndirectType : Arg.OrigTy;
    uint64_t Size = getSHABITypeSize(DataLayout, ABIType);
    if (Arg.IsByVal) {
      Align Alignment =
          Arg.Alignment.value_or(DataLayout.getABITypeAlign(ABIType));
      requireSupportedSHByValType(DataLayout, ABIType, Alignment);
      if (PartIndices.size() != 1 ||
          !CLI.Outs[PartIndices[0]].Flags.isByVal() ||
          CLI.Outs[PartIndices[0]].Flags.getByValSize() != Size)
        report_fatal_error("SH byval call argument size is inconsistent");
    } else {
      if (llvm::any_of(PartIndices, [&](unsigned I) {
            return CLI.Outs[I].Flags.isByVal() || CLI.Outs[I].Flags.isSRet();
          }))
        report_fatal_error("SH direct call argument has indirect ABI flags");
      SmallVector<SHValuePart, 4> Parts =
          getSHValueParts(*this, DataLayout, Arg.OrigTy);
      if (Parts.size() != PartIndices.size())
        report_fatal_error("SH call argument ABI parts are inconsistent");
    }
    CallArguments.push_back(
        {&Arg, ABIState.allocate(Size), std::move(PartIndices), false});
  }

  unsigned StackBytes = ABIState.getStackSize();
  if (StackBytes > 60 || StackBytes % 4 != 0)
    report_fatal_error("SH outgoing call frame size cannot exceed 60 bytes");
  SDValue Chain = DAG.getCALLSEQ_START(CLI.Chain, StackBytes, 0, CLI.DL);
  bool NeedsNarrowStackAddress =
      llvm::any_of(CallArguments, [](const SHCallArgument &CallArg) {
        return llvm::any_of(CallArg.ABIValue.Words, [](const SHABIWord &Word) {
          return !Word.isRegister() && Word.ValidBytes != 4;
        });
      });
  SDValue StackPtr;
  if (NeedsNarrowStackAddress) {
    StackPtr = DAG.getCopyFromReg(Chain, CLI.DL, SH::R15, MVT::i32);
    Chain = StackPtr.getValue(1);
  } else {
    StackPtr = DAG.getRegister(SH::R15, MVT::i32);
  }
  SmallVector<std::pair<MCRegister, SDValue>, 4> RegsToPass;
  SmallVector<SmallVector<SDValue, 4>, 8> ArgumentWords;
  SmallVector<SDValue, 8> StoreChains;
  ArgumentWords.reserve(CallArguments.size());
  for (const SHCallArgument &CallArg : CallArguments) {
    if (CallArg.IsSRet) {
      RegsToPass.emplace_back(SH::R2, CLI.OutVals[CallArg.PartIndices.front()]);
      ArgumentWords.emplace_back();
      continue;
    }
    if (CallArg.Arg->IsByVal) {
      Align Alignment = CallArg.Arg->Alignment.value_or(
          DataLayout.getABITypeAlign(CallArg.Arg->IndirectType));
      SmallVector<SDValue, 4> Words;
      for (const SHABIWord &Word : CallArg.ABIValue.Words) {
        SmallVector<SDValue, 2> LoadChains;
        SDValue Value =
            loadSHABIWord(DAG, CLI.DL, Chain, CallArg.Arg->Node,
                          MachinePointerInfo(CallArg.Arg->Val), Alignment,
                          Word.ByteOffset, Word.ValidBytes, LoadChains);
        Chain = DAG.getNode(ISD::TokenFactor, CLI.DL, MVT::Other, LoadChains);
        if (Word.isRegister()) {
          Words.push_back(Value);
          continue;
        }
        unsigned StackOffset = Word.StackOffset;
        if (!DataLayout.isLittleEndian() && CallArg.ABIValue.Size < 4 &&
            CallArg.ABIValue.Words.size() == 1)
          StackOffset += 4 - CallArg.ABIValue.Size;
        storeSHABIWord(DAG, CLI.DL, Chain, StackPtr,
                       MachinePointerInfo::getStack(MF, 0), Align(4),
                       StackOffset, Word.ValidBytes, Value, StoreChains);
        Chain = DAG.getNode(ISD::TokenFactor, CLI.DL, MVT::Other, StoreChains);
        StoreChains.clear();
        Words.push_back(SDValue());
      }
      ArgumentWords.push_back(std::move(Words));
      continue;
    }
    SmallVector<SHValuePart, 4> Parts =
        getSHValueParts(*this, DataLayout, CallArg.Arg->OrigTy);
    SmallVector<SDValue, 4> Values;
    for (unsigned I : CallArg.PartIndices)
      Values.push_back(CLI.OutVals[I]);
    if (CallArg.Arg->OrigTy->isAggregateType())
      ArgumentWords.push_back(
          packSHABIValue(DAG, CLI.DL, CallArg.ABIValue, Parts, Values));
    else
      ArgumentWords.push_back(std::move(Values));
  }

  for (unsigned ArgIndex = 0; ArgIndex != CallArguments.size(); ++ArgIndex) {
    const SHCallArgument &CallArg = CallArguments[ArgIndex];
    if (CallArg.IsSRet)
      continue;
    ArrayRef<SDValue> Words = ArgumentWords[ArgIndex];
    for (unsigned WordIndex = 0; WordIndex != CallArg.ABIValue.Words.size();
         ++WordIndex) {
      const SHABIWord &Word = CallArg.ABIValue.Words[WordIndex];
      if (Word.isRegister()) {
        RegsToPass.emplace_back(Word.Reg, Words[WordIndex]);
        continue;
      }
      if (CallArg.Arg->IsByVal)
        continue;
      unsigned StackOffset = Word.StackOffset;
      if (!DataLayout.isLittleEndian() && CallArg.ABIValue.Size < 4 &&
          CallArg.ABIValue.Words.size() == 1)
        StackOffset += 4 - CallArg.ABIValue.Size;
      storeSHABIWord(DAG, CLI.DL, Chain, StackPtr,
                     MachinePointerInfo::getStack(MF, 0), Align(4), StackOffset,
                     Word.ValidBytes, Words[WordIndex], StoreChains);
    }
  }
  if (!StoreChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, CLI.DL, MVT::Other, StoreChains);

  SDValue Glue;
  for (const auto &[Reg, Value] : RegsToPass) {
    Chain = DAG.getCopyToReg(Chain, CLI.DL, Reg, Value, Glue);
    Glue = Chain.getValue(1);
  }

  SDValue Callee = CLI.Callee;
  if (const auto *Global = dyn_cast<GlobalAddressSDNode>(Callee)) {
    const auto *CalleeFunction = dyn_cast<Function>(Global->getGlobal());
    const Function &Caller = MF.getFunction();
    auto EffectiveSection = [](const Function &F) {
      return F.getSection().empty() ? StringRef(".text") : F.getSection();
    };
    bool SameSection =
        CalleeFunction &&
        (CalleeFunction == &Caller ||
         (!getTargetMachine().getFunctionSections() &&
          EffectiveSection(Caller) == EffectiveSection(*CalleeFunction)) ||
         (!Caller.getSection().empty() &&
          Caller.getSection() == CalleeFunction->getSection()));
    bool UseDirectCall =
        getTargetMachine().getCodeModel() == CodeModel::Small &&
        CalleeFunction && Global->getOffset() == 0 &&
        !CalleeFunction->isDeclarationForLinker() &&
        !CalleeFunction->isInterposable() && CalleeFunction->isDSOLocal() &&
        SameSection;
    if (UseDirectCall)
      Callee = DAG.getTargetGlobalAddress(CalleeFunction, CLI.DL, MVT::i32);
    else
      Callee = LowerGlobalAddress(Callee, DAG);
  } else if (isa<ExternalSymbolSDNode>(Callee)) {
    const auto *External = cast<ExternalSymbolSDNode>(Callee);
    SHConstantPoolValue *CPV = SHConstantPoolValue::create(
        *DAG.getContext(), External->getSymbol(), 0);
    MF.setAlignment(std::max(MF.getAlignment(), Align(4)));
    SDValue CPAddr = DAG.getTargetConstantPool(CPV, MVT::i32, Align(4));
    Callee = DAG.getLoad(MVT::i32, CLI.DL, DAG.getEntryNode(), CPAddr,
                         MachinePointerInfo::getConstantPool(MF), Align(4),
                         MachineMemOperand::MOLoad |
                             MachineMemOperand::MODereferenceable |
                             MachineMemOperand::MOInvariant);
  } else if (Callee.getValueType() != MVT::i32) {
    report_fatal_error("SH indirect call target must be a 32-bit GPR value");
  }

  SmallVector<SDValue, 8> CallOps;
  CallOps.push_back(Chain);
  CallOps.push_back(Callee);
  const uint32_t *Mask =
      MF.getSubtarget<SHSubtarget>().getRegisterInfo()->getCallPreservedMask(
          MF, CLI.CallConv);
  if (!Mask)
    report_fatal_error("SH C calling convention has no call-preserved mask");
  CallOps.push_back(DAG.getRegisterMask(Mask));
  for (const auto &[Reg, Value] : RegsToPass)
    CallOps.push_back(DAG.getRegister(Reg, Value.getValueType()));
  if (Glue)
    CallOps.push_back(Glue);

  SDVTList CallVTs = DAG.getVTList(MVT::Other, MVT::Glue);
  Chain = DAG.getNode(SHISD::CALL, CLI.DL, CallVTs, CallOps);
  Glue = Chain.getValue(1);
  Chain = DAG.getCALLSEQ_END(Chain, StackBytes, 0, Glue, CLI.DL);
  Glue = Chain.getValue(1);

  if (!CLI.Ins.empty()) {
    SHABIState ReturnState;
    SHABIValue ABIValue = ReturnState.allocate(
        IsAtomicBooleanResult ? 4 : getSHABITypeSize(DataLayout, ReturnTy));
    if (ABIValue.Words.size() > 2)
      report_fatal_error("SH direct call result exceeds r0-r1");
    SmallVector<SDValue, 2> Words;
    for (unsigned I = 0; I != ABIValue.Words.size(); ++I) {
      MCRegister Reg = I == 0 ? SH::R0 : SH::R1;
      SDValue Result = DAG.getCopyFromReg(Chain, CLI.DL, Reg, MVT::i32, Glue);
      Words.push_back(Result);
      Chain = Result.getValue(1);
      Glue = Result.getValue(2);
    }
    SmallVector<SDValue, 4> Results;
    if (ReturnTy->isAggregateType())
      Results = unpackSHABIValue(DAG, CLI.DL, ABIValue, ReturnParts, Words);
    else
      Results.assign(Words.begin(), Words.end());
    InVals.append(Results);
  }

  return Chain;
}

void SHTargetLowering::getTgtMemIntrinsic(SmallVectorImpl<IntrinsicInfo> &Infos,
                                          const CallBase &I,
                                          MachineFunction &MF,
                                          unsigned Intrinsic) const {
  if (Intrinsic != Intrinsic::sh_tas_b)
    return;
  IntrinsicInfo Info;
  Info.opc = ISD::INTRINSIC_W_CHAIN;
  Info.memVT = MVT::i8;
  Info.ptrVal = I.getArgOperand(0);
  Info.size = 1;
  Info.align = Align(1);
  Info.flags = MachineMemOperand::MOLoad | MachineMemOperand::MOStore;
  Info.ssid = SyncScope::System;
  Info.order = AtomicOrdering::Monotonic;
  Infos.push_back(Info);
}

SDValue SHTargetLowering::LowerVASTART(SDValue Op, SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  const Function &F = MF.getFunction();
  SHMachineFunctionInfo &FuncInfo = *MF.getInfo<SHMachineFunctionInfo>();
  if (!F.isVarArg() || !FuncInfo.hasVarArgsSaveArea())
    report_fatal_error("SH va_start requires a variadic function");
  SDLoc DL(Op);
  SDValue Cursor = DAG.getFrameIndex(FuncInfo.getVarArgsFrameIndex(), MVT::i32);
  const Value *Storage = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  return DAG.getStore(Op.getOperand(0), DL, Cursor, Op.getOperand(1),
                      MachinePointerInfo(Storage), Align(4));
}

SDValue SHTargetLowering::LowerATOMIC_FENCE(SDValue Op,
                                            SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SyncScope::ID Scope = static_cast<SyncScope::ID>(Op.getConstantOperandVal(2));
  if (Scope == SyncScope::SingleThread)
    return DAG.getNode(ISD::MEMBARRIER, DL, MVT::Other, Op.getOperand(0));
  if (Scope != SyncScope::System)
    report_fatal_error("SH synchronization scope is not supported");

  ArgListTy Args;
  CallLoweringInfo CLI(DAG);
  CLI.setDebugLoc(DL)
      .setChain(Op.getOperand(0))
      .setLibCallee(CallingConv::C, Type::getVoidTy(*DAG.getContext()),
                    DAG.getExternalSymbol("__sync_synchronize",
                                          getPointerTy(DAG.getDataLayout())),
                    std::move(Args));
  return LowerCallTo(CLI).second;
}

static SDValue lowerSHSymbolAddress(SHConstantPoolValue *CPV, const SDLoc &DL,
                                    SelectionDAG &DAG) {
  MachineFunction &MF = DAG.getMachineFunction();
  MF.setAlignment(std::max(MF.getAlignment(), Align(4)));
  SDValue CPAddr = DAG.getTargetConstantPool(CPV, MVT::i32, Align(4));
  return DAG.getLoad(MVT::i32, DL, DAG.getEntryNode(), CPAddr,
                     MachinePointerInfo::getConstantPool(MF), Align(4),
                     MachineMemOperand::MOLoad |
                         MachineMemOperand::MODereferenceable |
                         MachineMemOperand::MOInvariant);
}

SDValue SHTargetLowering::LowerGlobalAddress(SDValue Op,
                                             SelectionDAG &DAG) const {
  const auto *Global = cast<GlobalAddressSDNode>(Op);
  int64_t Addend = Global->getOffset();
  if (!isInt<32>(Addend))
    report_fatal_error("SH global address addend must fit signed 32 bits");
  SHConstantPoolValue *CPV = SHConstantPoolValue::create(
      Global->getGlobal(), static_cast<int32_t>(Addend));
  return lowerSHSymbolAddress(CPV, SDLoc(Op), DAG);
}

SDValue SHTargetLowering::LowerBlockAddress(SDValue Op,
                                            SelectionDAG &DAG) const {
  const auto *Block = cast<BlockAddressSDNode>(Op);
  int64_t Addend = Block->getOffset();
  if (!isInt<32>(Addend))
    report_fatal_error("SH block address addend must fit signed 32 bits");
  SHConstantPoolValue *CPV = SHConstantPoolValue::create(
      Block->getBlockAddress(), static_cast<int32_t>(Addend));
  return lowerSHSymbolAddress(CPV, SDLoc(Op), DAG);
}

SDValue SHTargetLowering::LowerJumpTable(SDValue Op, SelectionDAG &DAG) const {
  const auto *Table = cast<JumpTableSDNode>(Op);
  if (Table->getIndex() < 0)
    report_fatal_error("SH jump-table index must be nonnegative");
  SHConstantPoolValue *CPV = SHConstantPoolValue::create(
      *DAG.getContext(), static_cast<unsigned>(Table->getIndex()), 0);
  return lowerSHSymbolAddress(CPV, SDLoc(Op), DAG);
}

SDValue SHTargetLowering::LowerBR_JT(SDValue Op, SelectionDAG &DAG) const {
  SDValue Chain = Op.getOperand(0);
  SDValue Table = Op.getOperand(1);
  SDValue Index = Op.getOperand(2);
  const auto *JT = cast<JumpTableSDNode>(Table);
  if (Index.getValueType() != MVT::i32)
    report_fatal_error("SH jump-table index must be i32");
  if (JT->getIndex() < 0)
    report_fatal_error("SH jump-table index must be nonnegative");

  MachineFunction &MF = DAG.getMachineFunction();
  const MachineJumpTableInfo *MJTI = MF.getJumpTableInfo();
  if (!MJTI ||
      static_cast<unsigned>(JT->getIndex()) >= MJTI->getJumpTables().size())
    report_fatal_error("SH jump-table branch has an invalid table index");
  if (MJTI->getEntryKind() != MachineJumpTableInfo::EK_BlockAddress)
    report_fatal_error("SH only supports absolute block-address jump tables");
  if (MJTI->getEntrySize(DAG.getDataLayout()) != 4 ||
      MJTI->getEntryAlignment(DAG.getDataLayout()) != 4)
    report_fatal_error("SH jump-table entries must be four-byte words");

  SDValue TargetJT = DAG.getTargetJumpTable(JT->getIndex(), MVT::i32);
  SDValue Ops[] = {Chain, Index, Table, TargetJT};
  MachineMemOperand::Flags Flags = MachineMemOperand::MOLoad |
                                   MachineMemOperand::MODereferenceable |
                                   MachineMemOperand::MOInvariant;
  return DAG.getMemIntrinsicNode(SHISD::BR_JT, SDLoc(Op),
                                 DAG.getVTList(MVT::Other), Ops, MVT::i32,
                                 MachinePointerInfo::getJumpTable(MF), Align(4),
                                 Flags, LocationSize::precise(4));
}

SDValue SHTargetLowering::LowerConstantPool(SDValue Op,
                                            SelectionDAG &DAG) const {
  const auto *CP = cast<ConstantPoolSDNode>(Op);
  MachineFunction &MF = DAG.getMachineFunction();
  MF.setAlignment(std::max(MF.getAlignment(), Align(4)));
  if (CP->isMachineConstantPoolEntry())
    return DAG.getTargetConstantPool(CP->getMachineCPVal(), MVT::i32,
                                     CP->getAlign(), CP->getOffset());
  return DAG.getTargetConstantPool(CP->getConstVal(), MVT::i32, CP->getAlign(),
                                   CP->getOffset());
}

static bool isSupportedSHAddress(SDValue Addr) {
  auto IsBase = [](SDValue Base) {
    return Base.getValueType() == MVT::i32 &&
           (Base.getOpcode() == ISD::FrameIndex ||
            Base.getOpcode() == ISD::CopyFromReg ||
            Base.getOpcode() == ISD::LOAD ||
            Base.getOpcode() == ISD::Register ||
            Base.getOpcode() == ISD::GlobalAddress ||
            Base.getOpcode() == ISD::TargetConstantPool);
  };

  if (IsBase(Addr))
    return true;
  if (Addr.getOpcode() != ISD::ADD)
    return false;

  const auto *Offset = dyn_cast<ConstantSDNode>(Addr.getOperand(1));
  if (!Offset || !IsBase(Addr.getOperand(0)))
    return false;
  return true;
}

static bool isSupportedSHNarrowAddress(SDValue Addr) {
  auto IsBase = [](SDValue Base) {
    return Base.getValueType() == MVT::i32 &&
           (Base.getOpcode() == ISD::FrameIndex ||
            Base.getOpcode() == ISD::CopyFromReg ||
            Base.getOpcode() == ISD::LOAD ||
            Base.getOpcode() == ISD::Register ||
            Base.getOpcode() == ISD::GlobalAddress);
  };

  if (IsBase(Addr))
    return true;
  if (Addr.getOpcode() != ISD::ADD || Addr.getValueType() != MVT::i32)
    return false;
  if (!IsBase(Addr.getOperand(0)))
    return false;
  return isa<ConstantSDNode>(Addr.getOperand(1)) || IsBase(Addr.getOperand(1));
}

static SDValue lowerSHDivRem(SDValue Op, SelectionDAG &DAG) {
  bool IsSigned = Op.getOpcode() == ISD::SDIVREM;
  bool QuotientUsed = Op->hasAnyUseOfValue(0);
  bool RemainderUsed = Op->hasAnyUseOfValue(1);
  SDLoc DL(Op);
  SDValue Dividend = Op.getOperand(0);
  SDValue Divisor = Op.getOperand(1);
  const SDNodeFlags Flags = Op->getFlags();

  if (QuotientUsed && RemainderUsed)
    return DAG.getNode(IsSigned ? SHISD::SDIVREM : SHISD::UDIVREM, DL,
                       Op->getVTList(), {Dividend, Divisor}, Flags);

  SDValue Undef = DAG.getUNDEF(MVT::i32);
  if (QuotientUsed) {
    SDValue Quotient = DAG.getNode(IsSigned ? SHISD::SDIV : SHISD::UDIV, DL,
                                   MVT::i32, Dividend, Divisor, Flags);
    return DAG.getMergeValues({Quotient, Undef}, DL);
  }
  if (RemainderUsed) {
    SDValue Remainder = DAG.getNode(IsSigned ? SHISD::SREM : SHISD::UREM, DL,
                                    MVT::i32, Dividend, Divisor, Flags);
    return DAG.getMergeValues({Undef, Remainder}, DL);
  }
  return DAG.getMergeValues({Undef, Undef}, DL);
}

static SDValue getI64Part(SDValue Value, unsigned Part, const SDLoc &DL,
                          SelectionDAG &DAG) {
  return DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i32, Value,
                     DAG.getConstant(Part, DL, MVT::i32));
}

static SDValue lowerSHUnalignedIntegerLoad(SDValue Op, SelectionDAG &DAG) {
  const auto *Load = cast<LoadSDNode>(Op);
  unsigned Width = Load->getMemoryVT().getStoreSize();
  if ((Width != 2 && Width != 4) || Load->getAlign() >= Align(Width))
    report_fatal_error("SH invalid unaligned aggregate load");
  if (Load->getBasePtr().getValueType() != MVT::i32)
    report_fatal_error("SH unaligned aggregate load address is not supported");

  SDLoc DL(Op);
  SDValue Result = DAG.getConstant(0, DL, MVT::i32);
  SmallVector<SDValue, 4> Chains;
  SDValue LoadChain = Load->getChain();
  for (unsigned Byte = 0; Byte != Width; ++Byte) {
    SDValue Address = DAG.getObjectPtrOffset(DL, Load->getBasePtr(),
                                             TypeSize::getFixed(Byte));
    SDValue Loaded = DAG.getExtLoad(
        ISD::ZEXTLOAD, DL, MVT::i32, LoadChain, Address,
        Load->getPointerInfo().getWithOffset(Byte), MVT::i8, Align(1),
        Load->getMemOperand()->getFlags(), Load->getAAInfo());
    SDValue Piece = Loaded;
    unsigned Shift =
        8 * (DAG.getDataLayout().isLittleEndian() ? Byte : Width - 1 - Byte);
    if (Shift != 0)
      Piece = DAG.getNode(ISD::SHL, DL, MVT::i32, Piece,
                          DAG.getConstant(Shift, DL, MVT::i32));
    Result = DAG.getNode(ISD::OR, DL, MVT::i32, Result, Piece);
    Chains.push_back(Loaded.getValue(1));
    if (Load->isVolatile())
      LoadChain = Loaded.getValue(1);
  }
  if (Load->getExtensionType() == ISD::SEXTLOAD)
    Result = DAG.getNode(ISD::SIGN_EXTEND_INREG, DL, MVT::i32, Result,
                         DAG.getValueType(Width == 2 ? MVT::i16 : MVT::i32));
  SDValue ResultChain = Load->isVolatile() ? LoadChain
                                           : DAG.getNode(ISD::TokenFactor, DL,
                                                         MVT::Other, Chains);
  return DAG.getMergeValues({Result, ResultChain}, DL);
}

static SDValue lowerSHUnalignedIntegerStore(SDValue Op, SelectionDAG &DAG) {
  const auto *Store = cast<StoreSDNode>(Op);
  unsigned Width = Store->getMemoryVT().getStoreSize();
  if ((Width != 2 && Width != 4) || Store->getAlign() >= Align(Width))
    report_fatal_error("SH invalid unaligned aggregate store");
  if (Store->getBasePtr().getValueType() != MVT::i32)
    report_fatal_error("SH unaligned aggregate store address is not supported");

  SDLoc DL(Op);
  SmallVector<SDValue, 4> Chains;
  SDValue StoreChain = Store->getChain();
  for (unsigned Byte = 0; Byte != Width; ++Byte) {
    unsigned Shift =
        8 * (DAG.getDataLayout().isLittleEndian() ? Byte : Width - 1 - Byte);
    SDValue Piece = Store->getValue();
    if (Shift != 0)
      Piece = DAG.getNode(ISD::SRL, DL, MVT::i32, Piece,
                          DAG.getConstant(Shift, DL, MVT::i32));
    SDValue Address = DAG.getObjectPtrOffset(DL, Store->getBasePtr(),
                                             TypeSize::getFixed(Byte));
    SDValue ByteStore = DAG.getTruncStore(
        StoreChain, DL, Piece, Address,
        Store->getPointerInfo().getWithOffset(Byte), MVT::i8, Align(1),
        Store->getMemOperand()->getFlags(), Store->getAAInfo());
    Chains.push_back(ByteStore);
    if (Store->isVolatile())
      StoreChain = ByteStore;
  }
  if (Store->isVolatile())
    return StoreChain;
  return DAG.getNode(ISD::TokenFactor, DL, MVT::Other, Chains);
}

static SDValue lowerSHI64Load(SDValue Op, SelectionDAG &DAG) {
  const auto *Load = cast<LoadSDNode>(Op);
  if (Load->getAlign() < Align(4))
    report_fatal_error("SH requires 4-byte alignment for i64 loads");
  if (!isSupportedSHAddress(Load->getBasePtr()))
    report_fatal_error(
        "SH i64 load address must be a register or supported 32-bit constant "
        "address addition");

  SDLoc DL(Op);
  SDValue Chain = Load->getChain();
  SDValue Base = Load->getBasePtr();
  SDValue Other = DAG.getObjectPtrOffset(DL, Base, TypeSize::getFixed(4));
  SDValue First = DAG.getLoad(
      MVT::i32, DL, Chain, Base, Load->getPointerInfo(), Load->getBaseAlign(),
      Load->getMemOperand()->getFlags(), Load->getAAInfo());
  SDValue SecondChain = Load->isVolatile() ? First.getValue(1) : Chain;
  SDValue Second = DAG.getLoad(
      MVT::i32, DL, SecondChain, Other, Load->getPointerInfo().getWithOffset(4),
      commonAlignment(Load->getBaseAlign(), 4),
      Load->getMemOperand()->getFlags(), Load->getAAInfo());
  SDValue ResultChain =
      Load->isVolatile() ? Second.getValue(1)
                         : DAG.getNode(ISD::TokenFactor, DL, MVT::Other,
                                       First.getValue(1), Second.getValue(1));

  SDValue Lo = DAG.getDataLayout().isLittleEndian() ? First : Second;
  SDValue Hi = DAG.getDataLayout().isLittleEndian() ? Second : First;
  SDValue Value = DAG.getNode(ISD::BUILD_PAIR, DL, MVT::i64, Lo, Hi);
  return DAG.getMergeValues({Value, ResultChain}, DL);
}

static SDValue lowerSHI64Store(SDValue Op, SelectionDAG &DAG) {
  const auto *Store = cast<StoreSDNode>(Op);
  if (Store->getAlign() < Align(4))
    report_fatal_error("SH requires 4-byte alignment for i64 stores");
  if (!isSupportedSHAddress(Store->getBasePtr()))
    report_fatal_error(
        "SH i64 store address must be a register or supported 32-bit constant "
        "address addition");

  SDLoc DL(Op);
  SDValue Lo = getI64Part(Store->getValue(), 0, DL, DAG);
  SDValue Hi = getI64Part(Store->getValue(), 1, DL, DAG);
  SDValue FirstValue = DAG.getDataLayout().isLittleEndian() ? Lo : Hi;
  SDValue SecondValue = DAG.getDataLayout().isLittleEndian() ? Hi : Lo;
  SDValue Chain = Store->getChain();
  SDValue Base = Store->getBasePtr();
  SDValue Other = DAG.getObjectPtrOffset(DL, Base, TypeSize::getFixed(4));
  SDValue First =
      DAG.getStore(Chain, DL, FirstValue, Base, Store->getPointerInfo(),
                   Store->getBaseAlign(), Store->getMemOperand()->getFlags(),
                   Store->getAAInfo());
  SDValue SecondChain = Store->isVolatile() ? First : Chain;
  SDValue Second =
      DAG.getStore(SecondChain, DL, SecondValue, Other,
                   Store->getPointerInfo().getWithOffset(4),
                   commonAlignment(Store->getBaseAlign(), 4),
                   Store->getMemOperand()->getFlags(), Store->getAAInfo());
  if (Store->isVolatile())
    return Second;
  return DAG.getNode(ISD::TokenFactor, DL, MVT::Other, First, Second);
}

static SDValue lowerSHShiftParts(SDValue Op, SelectionDAG &DAG) {
  unsigned Opcode;
  switch (Op.getOpcode()) {
  default:
    llvm_unreachable("unexpected SH shift-parts node");
  case ISD::SHL_PARTS:
    Opcode = SHISD::SHL_PARTS;
    break;
  case ISD::SRL_PARTS:
    Opcode = SHISD::SRL_PARTS;
    break;
  case ISD::SRA_PARTS:
    Opcode = SHISD::SRA_PARTS;
    break;
  }
  return DAG.getNode(Opcode, SDLoc(Op), Op->getVTList(),
                     {Op.getOperand(0), Op.getOperand(1), Op.getOperand(2)});
}

void SHTargetLowering::ReplaceNodeResults(SDNode *N,
                                          SmallVectorImpl<SDValue> &Results,
                                          SelectionDAG &DAG) const {
  if (N->getOpcode() != ISD::LOAD ||
      cast<LoadSDNode>(N)->getMemoryVT() != MVT::i64)
    return;
  SDValue Lowered = lowerSHI64Load(SDValue(N, 0), DAG);
  Results.push_back(Lowered);
  Results.push_back(Lowered.getValue(1));
}

SDValue SHTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const {
  if (Op.getOpcode() == ISD::VASTART)
    return LowerVASTART(Op, DAG);
  if (Op.getOpcode() == ISD::ATOMIC_FENCE)
    return LowerATOMIC_FENCE(Op, DAG);
  if (Op.getOpcode() == ISD::SETCC &&
      Op.getOperand(0).getValueType() == MVT::i64) {
    SDLoc DL(Op);
    SDValue LHS = Op.getOperand(0);
    SDValue RHS = Op.getOperand(1);
    ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(2))->get();
    SDValue Predicate =
        DAG.getTargetConstant(static_cast<unsigned>(CC), DL, MVT::i32);
    return DAG.getNode(
        SHISD::SETCC64, DL, MVT::i32,
        {getI64Part(LHS, 0, DL, DAG), getI64Part(LHS, 1, DL, DAG),
         getI64Part(RHS, 0, DL, DAG), getI64Part(RHS, 1, DL, DAG), Predicate});
  }
  if (Op.getOpcode() == ISD::BR_CC)
    return LowerBR_CC(Op, DAG);
  if (Op.getOpcode() == ISD::SHL_PARTS || Op.getOpcode() == ISD::SRL_PARTS ||
      Op.getOpcode() == ISD::SRA_PARTS)
    return lowerSHShiftParts(Op, DAG);
  if (Op.getOpcode() == ISD::SDIVREM || Op.getOpcode() == ISD::UDIVREM)
    return lowerSHDivRem(Op, DAG);
  if (Op.getOpcode() == ISD::LOAD || Op.getOpcode() == ISD::STORE) {
    const auto *Mem = cast<MemSDNode>(Op);
    EVT MemoryVT = Mem->getMemoryVT();
    if (MemoryVT == MVT::i64)
      return Op.getOpcode() == ISD::LOAD ? lowerSHI64Load(Op, DAG)
                                         : lowerSHI64Store(Op, DAG);
    if (MemoryVT == MVT::i32) {
      if (Mem->getAlign() < Align(4))
        return Op.getOpcode() == ISD::LOAD
                   ? lowerSHUnalignedIntegerLoad(Op, DAG)
                   : lowerSHUnalignedIntegerStore(Op, DAG);
      if (!isSupportedSHAddress(Mem->getBasePtr()))
        report_fatal_error(
            "SH memory address must be a register or supported 32-bit constant "
            "address addition");
      return Op;
    }
    if (MemoryVT != MVT::i8 && MemoryVT != MVT::i16)
      report_fatal_error(
          "SH only supports 8-, 16-, 32-, and 64-bit memory operations");
    if (MemoryVT == MVT::i16 && Mem->getAlign() < Align(2))
      return Op.getOpcode() == ISD::LOAD
                 ? lowerSHUnalignedIntegerLoad(Op, DAG)
                 : lowerSHUnalignedIntegerStore(Op, DAG);
    if (!isSupportedSHNarrowAddress(Mem->getBasePtr()))
      report_fatal_error(
          "SH byte/word memory address must be a register, frame index, or "
          "supported 32-bit address addition");
    return Op;
  }
  if (Op.getOpcode() == ISD::DYNAMIC_STACKALLOC)
    report_fatal_error("SH dynamic alloca is not supported");
  if (Op.getOpcode() == ISD::GlobalAddress)
    return LowerGlobalAddress(Op, DAG);
  if (Op.getOpcode() == ISD::BR_JT)
    return LowerBR_JT(Op, DAG);
  if (Op.getOpcode() == ISD::ConstantPool)
    return LowerConstantPool(Op, DAG);
  if (Op.getOpcode() == ISD::GlobalTLSAddress)
    report_fatal_error("SH thread-local storage is not supported");
  if (Op.getOpcode() == ISD::BlockAddress)
    return LowerBlockAddress(Op, DAG);
  if (Op.getOpcode() == ISD::JumpTable)
    return LowerJumpTable(Op, DAG);
  if (Op.getOpcode() == ISD::SETCC)
    report_fatal_error(
        "SH comparison results may only be used by conditional branches");
  if (Op.getOpcode() == ISD::SELECT || Op.getOpcode() == ISD::SELECT_CC)
    report_fatal_error("SH select is not supported");
  if (Op.getOpcode() == ISD::AND) {
    return Op;
  }
  if (Op.getOpcode() == ISD::OR) {
    if (Op->getFlags().hasDisjoint() &&
        Op.getOperand(0).getOpcode() == ISD::FrameIndex &&
        isa<ConstantSDNode>(Op.getOperand(1)))
      return DAG.getNode(ISD::ADD, SDLoc(Op), MVT::i32, Op.getOperand(0),
                         Op.getOperand(1));
    return Op;
  }
  report_fatal_error(Twine("SH operation is not supported: ") +
                     Op->getOperationName(&DAG));
}

static Register createGPR(MachineRegisterInfo &MRI) {
  return MRI.createVirtualRegister(&SH::GPRRegClass);
}

static MachineBasicBlock *emitJumpTableDispatch(MachineInstr &MI,
                                                MachineBasicBlock *MBB) {
  if (MI.getNumOperands() != 3 || !MI.getOperand(0).isReg() ||
      !MI.getOperand(1).isReg() || !MI.getOperand(2).isJTI())
    report_fatal_error(
        "malformed SH jump-table dispatch operands; expected index, base, and "
        "jump-table index");

  MachineFunction &MF = *MBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const MachineJumpTableInfo *MJTI = MF.getJumpTableInfo();
  unsigned JTI = MI.getOperand(2).getIndex();
  if (!MJTI || JTI >= MJTI->getJumpTables().size())
    report_fatal_error("SH jump-table dispatch has an invalid table index");
  if (MJTI->getEntryKind() != MachineJumpTableInfo::EK_BlockAddress ||
      MJTI->getEntrySize(MF.getDataLayout()) != 4 ||
      MJTI->getEntryAlignment(MF.getDataLayout()) != 4)
    report_fatal_error(
        "SH jump-table dispatch requires four-byte absolute block addresses");

  if (MI.memoperands().size() != 1)
    report_fatal_error(
        "SH jump-table dispatch requires one table-entry load memory operand");
  MachineMemOperand *MMO = *MI.memoperands_begin();
  const PseudoSourceValue *PSV = MMO->getPseudoValue();
  if (!MMO->isLoad() || MMO->isStore() ||
      MMO->getSize() != LocationSize::precise(4) ||
      MMO->getAlign() < Align(4) || !MMO->isInvariant() ||
      !MMO->isDereferenceable() || !PSV || !PSV->isJumpTable())
    report_fatal_error(
        "SH jump-table dispatch requires an invariant, dereferenceable, "
        "four-byte aligned jump-table load");

  const DebugLoc &DL = MI.getDebugLoc();
  Register Index = MI.getOperand(0).getReg();
  Register Base = MI.getOperand(1).getReg();
  Register IndexCopy = createGPR(MRI);
  Register Scaled = createGPR(MRI);
  Register BaseCopy = createGPR(MRI);
  Register EntryAddress = createGPR(MRI);
  Register Target = createGPR(MRI);

  BuildMI(*MBB, MI, DL, TII.get(SH::MOVrr), IndexCopy).addReg(Index);
  BuildMI(*MBB, MI, DL, TII.get(SH::SHLL2), Scaled).addReg(IndexCopy);
  BuildMI(*MBB, MI, DL, TII.get(SH::MOVrr), BaseCopy).addReg(Base);
  BuildMI(*MBB, MI, DL, TII.get(SH::ADDrr), EntryAddress)
      .addReg(BaseCopy)
      .addReg(Scaled);
  BuildMI(*MBB, MI, DL, TII.get(SH::MOVL_load_reg), Target)
      .addReg(EntryAddress)
      .addMemOperand(MMO);
  BuildMI(*MBB, MI, DL, TII.get(SH::JMP)).addReg(Target).addJumpTableIndex(JTI);

  MI.eraseFromParent();
  return MBB;
}

static MachineBasicBlock *emitMultiply(MachineInstr &MI,
                                       MachineBasicBlock *MBB) {
  MachineFunction &MF = *MBB->getParent();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  Register Dest = MI.getOperand(0).getReg();
  Register LHS = MI.getOperand(1).getReg();
  Register RHS = MI.getOperand(2).getReg();

  BuildMI(*MBB, MI, DL, TII.get(SH::MUL_L)).addReg(LHS).addReg(RHS);
  BuildMI(*MBB, MI, DL, TII.get(SH::STS_MACL), Dest);

  MI.eraseFromParent();
  return MBB;
}

static MachineBasicBlock *emitLowCarry(MachineInstr &MI,
                                       MachineBasicBlock *MBB) {
  MachineFunction &MF = *MBB->getParent();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  Register Dest = MI.getOperand(0).getReg();
  Register LHS = MI.getOperand(1).getReg();
  Register RHS = MI.getOperand(2).getReg();
  unsigned Opcode = MI.getOpcode() == SH::ADDC_LO_PSEUDO ? SH::ADDC : SH::SUBC;

  BuildMI(*MBB, MI, DL, TII.get(SH::CLRT));
  BuildMI(*MBB, MI, DL, TII.get(Opcode), Dest).addReg(LHS).addReg(RHS);

  MI.eraseFromParent();
  return MBB;
}

static Register emitDivisionQuotient(MachineInstr &MI, MachineBasicBlock &MBB,
                                     const SHInstrInfo &TII,
                                     MachineRegisterInfo &MRI, bool IsSigned,
                                     Register Dividend, Register Divisor,
                                     Register QuotientDest) {
  const DebugLoc &DL = MI.getDebugLoc();
  Register Quotient = createGPR(MRI);
  BuildMI(MBB, MI, DL, TII.get(TargetOpcode::COPY), Quotient).addReg(Dividend);

  Register Partial;
  Register Zero;
  if (!IsSigned) {
    Partial = createGPR(MRI);
    BuildMI(MBB, MI, DL, TII.get(SH::MOVri), Partial).addImm(0);
    BuildMI(MBB, MI, DL, TII.get(SH::DIV0U));
  } else {
    Register SignProbe = createGPR(MRI);
    BuildMI(MBB, MI, DL, TII.get(SH::SHLL), SignProbe).addReg(Dividend);

    Partial = createGPR(MRI);
    BuildMI(MBB, MI, DL, TII.get(SH::SUBC), Partial)
        .addReg(Dividend)
        .addReg(Dividend);

    Zero = createGPR(MRI);
    BuildMI(MBB, MI, DL, TII.get(SH::XORrr), Zero)
        .addReg(SignProbe)
        .addReg(SignProbe);

    Register Magnitude = createGPR(MRI);
    BuildMI(MBB, MI, DL, TII.get(SH::SUBC), Magnitude)
        .addReg(Quotient)
        .addReg(Zero);
    Quotient = Magnitude;
    BuildMI(MBB, MI, DL, TII.get(SH::DIV0S)).addReg(Divisor).addReg(Partial);
  }

  for (unsigned I = 0; I != 32; ++I) {
    Register NextQuotient = createGPR(MRI);
    BuildMI(MBB, MI, DL, TII.get(SH::ROTCL), NextQuotient).addReg(Quotient);
    Quotient = NextQuotient;

    Register NextPartial = createGPR(MRI);
    BuildMI(MBB, MI, DL, TII.get(SH::DIV1), NextPartial)
        .addReg(Partial)
        .addReg(Divisor);
    Partial = NextPartial;
  }

  Register FinalRotate = IsSigned ? createGPR(MRI) : QuotientDest;
  BuildMI(MBB, MI, DL, TII.get(SH::ROTCL), FinalRotate).addReg(Quotient);
  if (!IsSigned)
    return FinalRotate;

  BuildMI(MBB, MI, DL, TII.get(SH::ADDC), QuotientDest)
      .addReg(FinalRotate)
      .addReg(Zero);
  return QuotientDest;
}

static MachineBasicBlock *emitDivision(MachineInstr &MI,
                                       MachineBasicBlock *MBB) {
  MachineFunction &MF = *MBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  unsigned Opcode = MI.getOpcode();
  bool IsSigned = Opcode == SH::SDIV32_PSEUDO || Opcode == SH::SREM32_PSEUDO ||
                  Opcode == SH::SDIVREM32_PSEUDO;
  bool NeedsQuotient =
      Opcode == SH::UDIV32_PSEUDO || Opcode == SH::SDIV32_PSEUDO ||
      Opcode == SH::UDIVREM32_PSEUDO || Opcode == SH::SDIVREM32_PSEUDO;
  bool NeedsRemainder =
      Opcode == SH::UREM32_PSEUDO || Opcode == SH::SREM32_PSEUDO ||
      Opcode == SH::UDIVREM32_PSEUDO || Opcode == SH::SDIVREM32_PSEUDO;

  unsigned InputIndex = NeedsQuotient && NeedsRemainder ? 2 : 1;
  Register QuotientDest =
      NeedsQuotient ? MI.getOperand(0).getReg() : createGPR(MRI);
  Register RemainderDest =
      NeedsRemainder
          ? MI.getOperand(NeedsQuotient && NeedsRemainder ? 1 : 0).getReg()
          : Register();
  Register Dividend = MI.getOperand(InputIndex).getReg();
  Register Divisor = MI.getOperand(InputIndex + 1).getReg();

  Register Quotient = emitDivisionQuotient(MI, *MBB, TII, MRI, IsSigned,
                                           Dividend, Divisor, QuotientDest);
  if (NeedsRemainder) {
    Register Product = createGPR(MRI);
    BuildMI(*MBB, MI, DL, TII.get(SH::MUL_L)).addReg(Divisor).addReg(Quotient);
    BuildMI(*MBB, MI, DL, TII.get(SH::STS_MACL), Product);
    BuildMI(*MBB, MI, DL, TII.get(SH::SUBrr), RemainderDest)
        .addReg(Dividend)
        .addReg(Product);
  }

  MI.eraseFromParent();
  return MBB;
}

static void emitUnsignedByte(MachineBasicBlock &MBB, MachineInstr &InsertBefore,
                             const DebugLoc &DL, const SHInstrInfo &TII,
                             MachineRegisterInfo &MRI, uint8_t Byte,
                             Register Dest) {
  if (Byte <= 127) {
    BuildMI(MBB, InsertBefore, DL, TII.get(SH::MOVri), Dest).addImm(Byte);
    return;
  }

  Register SignedByte = createGPR(MRI);
  BuildMI(MBB, InsertBefore, DL, TII.get(SH::MOVri), SignedByte)
      .addImm(static_cast<int64_t>(Byte) - 256);
  BuildMI(MBB, InsertBefore, DL, TII.get(SH::EXTUB), Dest).addReg(SignedByte);
}

static MachineBasicBlock *emitI32Constant(MachineInstr &MI,
                                          MachineBasicBlock *MBB) {
  MachineFunction &MF = *MBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  Register Dest = MI.getOperand(0).getReg();
  uint32_t Value = static_cast<uint32_t>(MI.getOperand(1).getImm());
  uint8_t Bytes[] = {
      static_cast<uint8_t>(Value >> 24), static_cast<uint8_t>(Value >> 16),
      static_cast<uint8_t>(Value >> 8), static_cast<uint8_t>(Value)};

  unsigned First = 0;
  while (First != 3 && Bytes[First] == 0)
    ++First;

  Register Acc = First == 3 ? Dest : createGPR(MRI);
  emitUnsignedByte(*MBB, MI, DL, TII, MRI, Bytes[First], Acc);
  for (unsigned I = First + 1; I != 4; ++I) {
    bool IsLast = I == 3;
    Register Shifted = IsLast && Bytes[I] == 0 ? Dest : createGPR(MRI);
    BuildMI(*MBB, MI, DL, TII.get(SH::SHLL8), Shifted).addReg(Acc);
    if (Bytes[I] == 0) {
      Acc = Shifted;
      continue;
    }

    Register ByteReg = createGPR(MRI);
    emitUnsignedByte(*MBB, MI, DL, TII, MRI, Bytes[I], ByteReg);
    Register Combined = IsLast ? Dest : createGPR(MRI);
    BuildMI(*MBB, MI, DL, TII.get(SH::ORrr), Combined)
        .addReg(Shifted)
        .addReg(ByteReg);
    Acc = Combined;
  }

  MI.eraseFromParent();
  return MBB;
}

SmallVector<unsigned, 16> llvm::SH::planConstantShift(unsigned PseudoOpcode,
                                                      unsigned Amount) {
  SmallVector<unsigned, 16> Opcodes;
  if (PseudoOpcode == SH::SRAri) {
    if (Amount >= 24) {
      Opcodes.push_back(SH::SHLR16);
      Opcodes.push_back(SH::SHLR8);
      Opcodes.push_back(SH::EXTSB);
      Amount -= 24;
    } else if (Amount >= 16) {
      Opcodes.push_back(SH::SHLR16);
      Opcodes.push_back(SH::EXTSW);
      Amount -= 16;
    }
    Opcodes.append(Amount, SH::SHAR);
    return Opcodes;
  }

  unsigned Shift16 = PseudoOpcode == SH::SHLri ? SH::SHLL16 : SH::SHLR16;
  unsigned Shift8 = PseudoOpcode == SH::SHLri ? SH::SHLL8 : SH::SHLR8;
  unsigned Shift2 = PseudoOpcode == SH::SHLri ? SH::SHLL2 : SH::SHLR2;
  unsigned Shift1 = PseudoOpcode == SH::SHLri ? SH::SHLL : SH::SHLR;
  if (Amount >= 16) {
    Opcodes.push_back(Shift16);
    Amount -= 16;
  }
  if (Amount >= 8) {
    Opcodes.push_back(Shift8);
    Amount -= 8;
  }
  while (Amount >= 2) {
    Opcodes.push_back(Shift2);
    Amount -= 2;
  }
  if (Amount)
    Opcodes.push_back(Shift1);
  return Opcodes;
}

static MachineBasicBlock *emitConstantShift(MachineInstr &MI,
                                            MachineBasicBlock *MBB) {
  MachineFunction &MF = *MBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  Register Dest = MI.getOperand(0).getReg();
  Register Current = MI.getOperand(1).getReg();
  int64_t SignedAmount = MI.getOperand(2).getImm();
  if (SignedAmount < 0 || SignedAmount > 31)
    report_fatal_error("SH constant shift amount must be in [0, 31]");
  SmallVector<unsigned, 16> Opcodes = SH::planConstantShift(
      MI.getOpcode(), static_cast<unsigned>(SignedAmount));

  if (Opcodes.empty()) {
    BuildMI(*MBB, MI, DL, TII.get(TargetOpcode::COPY), Dest).addReg(Current);
  } else {
    for (unsigned I = 0; I != Opcodes.size(); ++I) {
      Register Next = I + 1 == Opcodes.size() ? Dest : createGPR(MRI);
      BuildMI(*MBB, MI, DL, TII.get(Opcodes[I]), Next).addReg(Current);
      Current = Next;
    }
  }

  MI.eraseFromParent();
  return MBB;
}

static MachineBasicBlock *emitVariableShift(MachineInstr &MI,
                                            MachineBasicBlock *EntryMBB) {
  MachineFunction &MF = *EntryMBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  const BasicBlock *IRBB = EntryMBB->getBasicBlock();
  MachineFunction::iterator Insert = std::next(EntryMBB->getIterator());

  MachineBasicBlock *LoopMBB = MF.CreateMachineBasicBlock(IRBB);
  MachineBasicBlock *DoneMBB = MF.CreateMachineBasicBlock(IRBB);
  MF.insert(Insert, LoopMBB);
  MF.insert(Insert, DoneMBB);

  DoneMBB->splice(DoneMBB->begin(), EntryMBB,
                  std::next(MachineBasicBlock::iterator(MI)), EntryMBB->end());
  DoneMBB->transferSuccessorsAndUpdatePHIs(EntryMBB);

  EntryMBB->addSuccessor(LoopMBB);
  EntryMBB->addSuccessor(DoneMBB);
  LoopMBB->addSuccessor(LoopMBB);
  LoopMBB->addSuccessor(DoneMBB);

  Register Dest = MI.getOperand(0).getReg();
  Register Source = MI.getOperand(1).getReg();
  Register Count = MI.getOperand(2).getReg();
  Register Mask = createGPR(MRI);
  Register MaskedCount = createGPR(MRI);
  Register ValuePhi = createGPR(MRI);
  Register CountPhi = createGPR(MRI);
  Register NextValue = createGPR(MRI);
  Register NextCount = createGPR(MRI);

  BuildMI(*EntryMBB, MI, DL, TII.get(SH::MOVri), Mask).addImm(31);
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::ANDrr), MaskedCount)
      .addReg(Mask)
      .addReg(Count);
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::TST))
      .addReg(MaskedCount)
      .addReg(MaskedCount);
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::BT)).addMBB(DoneMBB);

  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TargetOpcode::PHI), ValuePhi)
      .addReg(Source)
      .addMBB(EntryMBB)
      .addReg(NextValue)
      .addMBB(LoopMBB);
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TargetOpcode::PHI), CountPhi)
      .addReg(MaskedCount)
      .addMBB(EntryMBB)
      .addReg(NextCount)
      .addMBB(LoopMBB);

  unsigned ShiftOpcode;
  switch (MI.getOpcode()) {
  default:
    llvm_unreachable("unexpected SH variable shift pseudo");
  case SH::SHLrr:
    ShiftOpcode = SH::SHLL;
    break;
  case SH::SRLrr:
    ShiftOpcode = SH::SHLR;
    break;
  case SH::SRArr:
    ShiftOpcode = SH::SHAR;
    break;
  }
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(ShiftOpcode), NextValue)
      .addReg(ValuePhi);
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::DT), NextCount)
      .addReg(CountPhi);
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::BF)).addMBB(LoopMBB);

  BuildMI(*DoneMBB, DoneMBB->begin(), DL, TII.get(TargetOpcode::PHI), Dest)
      .addReg(Source)
      .addMBB(EntryMBB)
      .addReg(NextValue)
      .addMBB(LoopMBB);

  MI.eraseFromParent();
  return DoneMBB;
}

static MachineBasicBlock *emitVariableI64Shift(MachineInstr &MI,
                                               MachineBasicBlock *EntryMBB) {
  MachineFunction &MF = *EntryMBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  const BasicBlock *IRBB = EntryMBB->getBasicBlock();
  MachineFunction::iterator Insert = std::next(EntryMBB->getIterator());

  MachineBasicBlock *LoopMBB = MF.CreateMachineBasicBlock(IRBB);
  MachineBasicBlock *DoneMBB = MF.CreateMachineBasicBlock(IRBB);
  MF.insert(Insert, LoopMBB);
  MF.insert(Insert, DoneMBB);

  DoneMBB->splice(DoneMBB->begin(), EntryMBB,
                  std::next(MachineBasicBlock::iterator(MI)), EntryMBB->end());
  DoneMBB->transferSuccessorsAndUpdatePHIs(EntryMBB);

  EntryMBB->addSuccessor(LoopMBB);
  EntryMBB->addSuccessor(DoneMBB);
  LoopMBB->addSuccessor(LoopMBB);
  LoopMBB->addSuccessor(DoneMBB);

  Register DestLo = MI.getOperand(0).getReg();
  Register DestHi = MI.getOperand(1).getReg();
  Register SourceLo = MI.getOperand(2).getReg();
  Register SourceHi = MI.getOperand(3).getReg();
  Register Count = MI.getOperand(4).getReg();
  Register Mask = createGPR(MRI);
  Register MaskedCount = createGPR(MRI);
  Register LoPhi = createGPR(MRI);
  Register HiPhi = createGPR(MRI);
  Register CountPhi = createGPR(MRI);
  Register NextLo = createGPR(MRI);
  Register NextHi = createGPR(MRI);
  Register NextCount = createGPR(MRI);

  BuildMI(*EntryMBB, MI, DL, TII.get(SH::MOVri), Mask).addImm(63);
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::ANDrr), MaskedCount)
      .addReg(Mask)
      .addReg(Count);
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::TST))
      .addReg(MaskedCount)
      .addReg(MaskedCount);
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::BT)).addMBB(DoneMBB);

  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TargetOpcode::PHI), LoPhi)
      .addReg(SourceLo)
      .addMBB(EntryMBB)
      .addReg(NextLo)
      .addMBB(LoopMBB);
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TargetOpcode::PHI), HiPhi)
      .addReg(SourceHi)
      .addMBB(EntryMBB)
      .addReg(NextHi)
      .addMBB(LoopMBB);
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TargetOpcode::PHI), CountPhi)
      .addReg(MaskedCount)
      .addMBB(EntryMBB)
      .addReg(NextCount)
      .addMBB(LoopMBB);

  switch (MI.getOpcode()) {
  default:
    llvm_unreachable("unexpected SH i64 variable shift pseudo");
  case SH::SHL64_PSEUDO:
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::SHLL), NextLo)
        .addReg(LoPhi);
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::ROTCL), NextHi)
        .addReg(HiPhi);
    break;
  case SH::SRL64_PSEUDO:
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::SHLR), NextHi)
        .addReg(HiPhi);
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::ROTCR), NextLo)
        .addReg(LoPhi);
    break;
  case SH::SRA64_PSEUDO:
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::SHAR), NextHi)
        .addReg(HiPhi);
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::ROTCR), NextLo)
        .addReg(LoPhi);
    break;
  }
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::DT), NextCount)
      .addReg(CountPhi);
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::BF)).addMBB(LoopMBB);

  BuildMI(*DoneMBB, DoneMBB->begin(), DL, TII.get(TargetOpcode::PHI), DestLo)
      .addReg(SourceLo)
      .addMBB(EntryMBB)
      .addReg(NextLo)
      .addMBB(LoopMBB);
  BuildMI(*DoneMBB, DoneMBB->begin(), DL, TII.get(TargetOpcode::PHI), DestHi)
      .addReg(SourceHi)
      .addMBB(EntryMBB)
      .addReg(NextHi)
      .addMBB(LoopMBB);

  MI.eraseFromParent();
  return DoneMBB;
}

struct SHCompareBranch {
  unsigned CompareOpcode;
  bool BranchOnSet;
};

static SHCompareBranch getSHCompareBranch(ISD::CondCode CC) {
  switch (CC) {
  default:
    report_fatal_error("SH i64 comparison predicate is not supported");
  case ISD::SETEQ:
    return {SH::CMP_EQ, true};
  case ISD::SETNE:
    return {SH::CMP_EQ, false};
  case ISD::SETGE:
    return {SH::CMP_GE, true};
  case ISD::SETLT:
    return {SH::CMP_GE, false};
  case ISD::SETGT:
    return {SH::CMP_GT, true};
  case ISD::SETLE:
    return {SH::CMP_GT, false};
  case ISD::SETUGE:
    return {SH::CMP_HS, true};
  case ISD::SETULT:
    return {SH::CMP_HS, false};
  case ISD::SETUGT:
    return {SH::CMP_HI, true};
  case ISD::SETULE:
    return {SH::CMP_HI, false};
  }
}

static ISD::CondCode getSHUnsignedLowPredicate(ISD::CondCode CC) {
  switch (CC) {
  default:
    return CC;
  case ISD::SETGE:
    return ISD::SETUGE;
  case ISD::SETLT:
    return ISD::SETULT;
  case ISD::SETGT:
    return ISD::SETUGT;
  case ISD::SETLE:
    return ISD::SETULE;
  }
}

static void
emitCompareBranch(MachineBasicBlock &MBB, const DebugLoc &DL,
                  const SHInstrInfo &TII, Register LHS, Register RHS,
                  ISD::CondCode CC, MachineBasicBlock *TrueMBB,
                  MachineBasicBlock *FalseMBB,
                  BranchProbability TrueProbability = BranchProbability(1, 2)) {
  SHCompareBranch Info = getSHCompareBranch(CC);
  BuildMI(MBB, MBB.end(), DL, TII.get(Info.CompareOpcode))
      .addReg(RHS)
      .addReg(LHS);
  BuildMI(MBB, MBB.end(), DL, TII.get(Info.BranchOnSet ? SH::BT : SH::BF))
      .addMBB(TrueMBB);
  BuildMI(MBB, MBB.end(), DL, TII.get(SH::BRA)).addMBB(FalseMBB);
  MBB.addSuccessor(TrueMBB, TrueProbability);
  MBB.addSuccessor(FalseMBB, TrueProbability.getCompl());
}

static MachineBasicBlock *emitI64CompareBranch(MachineInstr &MI,
                                               MachineBasicBlock *EntryMBB) {
  MachineFunction &MF = *EntryMBB->getParent();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  const BasicBlock *IRBB = EntryMBB->getBasicBlock();
  MachineBasicBlock *BranchDest = MI.getOperand(5).getMBB();
  ISD::CondCode CC = static_cast<ISD::CondCode>(MI.getOperand(4).getImm());

  MachineBasicBlock *OtherDest = nullptr;
  BranchProbability BranchDestProbability = BranchProbability(1, 2);
  for (auto Succ = EntryMBB->succ_begin(); Succ != EntryMBB->succ_end();
       ++Succ) {
    MachineBasicBlock *Successor = *Succ;
    if (Successor == BranchDest)
      BranchDestProbability = EntryMBB->getSuccProbability(Succ);
    if (Successor != BranchDest) {
      if (OtherDest)
        report_fatal_error("SH i64 comparison has too many false successors");
      OtherDest = Successor;
    }
  }
  if (!EntryMBB->isSuccessor(BranchDest) || !OtherDest)
    report_fatal_error("SH i64 comparison requires true and false successors");

  MachineBasicBlock *LowMBB = MF.CreateMachineBasicBlock(IRBB);
  MachineBasicBlock *HighMBB = nullptr;
  bool IsEquality = CC == ISD::SETEQ || CC == ISD::SETNE;
  if (!IsEquality)
    HighMBB = MF.CreateMachineBasicBlock(IRBB);
  MachineBasicBlock *TrueMBB = MF.CreateMachineBasicBlock(IRBB);
  MachineBasicBlock *FalseMBB = MF.CreateMachineBasicBlock(IRBB);

  MachineFunction::iterator Insert = std::next(EntryMBB->getIterator());
  if (HighMBB)
    MF.insert(Insert, HighMBB);
  MF.insert(Insert, LowMBB);
  MF.insert(Insert, TrueMBB);
  MF.insert(Insert, FalseMBB);

  FalseMBB->splice(FalseMBB->begin(), EntryMBB,
                   std::next(MachineBasicBlock::iterator(MI)), EntryMBB->end());

  BranchDest->replacePhiUsesWith(EntryMBB, TrueMBB);
  OtherDest->replacePhiUsesWith(EntryMBB, FalseMBB);
  while (!EntryMBB->succ_empty())
    EntryMBB->removeSuccessor(*EntryMBB->succ_begin());
  TrueMBB->addSuccessor(BranchDest, BranchProbability::getOne());
  FalseMBB->addSuccessor(OtherDest, BranchProbability::getOne());

  Register LHSLo = MI.getOperand(0).getReg();
  Register LHSHi = MI.getOperand(1).getReg();
  Register RHSLo = MI.getOperand(2).getReg();
  Register RHSHi = MI.getOperand(3).getReg();

  BuildMI(*TrueMBB, TrueMBB->end(), DL, TII.get(SH::BRA)).addMBB(BranchDest);

  if (IsEquality) {
    MachineBasicBlock *HighEqualMBB = LowMBB;
    MachineBasicBlock *HighUnequalMBB = CC == ISD::SETEQ ? FalseMBB : TrueMBB;
    BranchProbability HighEqualityProbability =
        CC == ISD::SETEQ ? BranchDestProbability
                         : BranchDestProbability.getCompl();
    emitCompareBranch(*EntryMBB, DL, TII, LHSHi, RHSHi, ISD::SETEQ,
                      HighEqualMBB, HighUnequalMBB, HighEqualityProbability);
    emitCompareBranch(*LowMBB, DL, TII, LHSLo, RHSLo, CC, TrueMBB, FalseMBB,
                      BranchDestProbability);
  } else {
    emitCompareBranch(*EntryMBB, DL, TII, LHSHi, RHSHi, ISD::SETEQ, LowMBB,
                      HighMBB);
    emitCompareBranch(*HighMBB, DL, TII, LHSHi, RHSHi, CC, TrueMBB, FalseMBB,
                      BranchDestProbability);
    emitCompareBranch(*LowMBB, DL, TII, LHSLo, RHSLo,
                      getSHUnsignedLowPredicate(CC), TrueMBB, FalseMBB,
                      BranchDestProbability);
  }

  MI.eraseFromParent();
  return FalseMBB;
}

MachineBasicBlock *
SHTargetLowering::EmitInstrWithCustomInserter(MachineInstr &MI,
                                              MachineBasicBlock *MBB) const {
  switch (MI.getOpcode()) {
  default:
    llvm_unreachable("unexpected SH custom inserter opcode");
  case SH::ADDC_LO_PSEUDO:
  case SH::SUBC_LO_PSEUDO:
    return emitLowCarry(MI, MBB);
  case SH::MUL32_PSEUDO:
    return emitMultiply(MI, MBB);
  case SH::UDIV32_PSEUDO:
  case SH::UREM32_PSEUDO:
  case SH::UDIVREM32_PSEUDO:
  case SH::SDIV32_PSEUDO:
  case SH::SREM32_PSEUDO:
  case SH::SDIVREM32_PSEUDO:
    return emitDivision(MI, MBB);
  case SH::MOVi32:
    return emitI32Constant(MI, MBB);
  case SH::SHLri:
  case SH::SRLri:
  case SH::SRAri:
    return emitConstantShift(MI, MBB);
  case SH::SHLrr:
  case SH::SRLrr:
  case SH::SRArr:
    return emitVariableShift(MI, MBB);
  case SH::SHL64_PSEUDO:
  case SH::SRL64_PSEUDO:
  case SH::SRA64_PSEUDO:
    return emitVariableI64Shift(MI, MBB);
  case SH::BR_CC64_PSEUDO:
    return emitI64CompareBranch(MI, MBB);
  case SH::SH_JT_DISPATCH:
    return emitJumpTableDispatch(MI, MBB);
  }
}

void SHTargetLowering::AdjustInstrPostInstrSelection(MachineInstr &MI,
                                                     SDNode *Node) const {
  if (MI.getOpcode() != SH::MOVB_store_frame &&
      MI.getOpcode() != SH::MOVW_store_frame)
    return;
  MachineOperand *R0Def = MI.findRegisterDefOperand(SH::R0, /*TRI=*/nullptr);
  if (!R0Def)
    report_fatal_error("SH narrow frame store is missing its r0 clobber");
  R0Def->setIsEarlyClobber(true);
}

SDValue SHTargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const {
  SDValue Chain = Op.getOperand(0);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue Dest = Op.getOperand(4);
  SDLoc DL(Op);

  if ((CC == ISD::SETEQ || CC == ISD::SETNE) &&
      ((isa<ConstantSDNode>(RHS) && cast<ConstantSDNode>(RHS)->isZero()) ||
       (isa<ConstantSDNode>(LHS) && cast<ConstantSDNode>(LHS)->isZero()))) {
    SDValue Value = isa<ConstantSDNode>(RHS) ? LHS : RHS;
    SDValue Glue = DAG.getNode(SHISD::TST, DL, MVT::Glue, Value, Value);
    return DAG.getNode(CC == ISD::SETEQ ? SHISD::BT : SHISD::BF, DL, MVT::Other,
                       Chain, Dest, Glue);
  }

  unsigned CompareOpcode;
  bool BranchOnSet;
  switch (CC) {
  default:
    report_fatal_error("SH comparison predicate is not supported");
  case ISD::SETEQ:
    CompareOpcode = SHISD::CMP_EQ;
    BranchOnSet = true;
    break;
  case ISD::SETNE:
    CompareOpcode = SHISD::CMP_EQ;
    BranchOnSet = false;
    break;
  case ISD::SETGE:
    CompareOpcode = SHISD::CMP_GE;
    BranchOnSet = true;
    break;
  case ISD::SETLT:
    CompareOpcode = SHISD::CMP_GE;
    BranchOnSet = false;
    break;
  case ISD::SETGT:
    CompareOpcode = SHISD::CMP_GT;
    BranchOnSet = true;
    break;
  case ISD::SETLE:
    CompareOpcode = SHISD::CMP_GT;
    BranchOnSet = false;
    break;
  case ISD::SETUGE:
    CompareOpcode = SHISD::CMP_HS;
    BranchOnSet = true;
    break;
  case ISD::SETULT:
    CompareOpcode = SHISD::CMP_HS;
    BranchOnSet = false;
    break;
  case ISD::SETUGT:
    CompareOpcode = SHISD::CMP_HI;
    BranchOnSet = true;
    break;
  case ISD::SETULE:
    CompareOpcode = SHISD::CMP_HI;
    BranchOnSet = false;
    break;
  }

  SDValue Glue = DAG.getNode(CompareOpcode, DL, MVT::Glue, LHS, RHS);
  return DAG.getNode(BranchOnSet ? SHISD::BT : SHISD::BF, DL, MVT::Other, Chain,
                     Dest, Glue);
}

bool SHTargetLowering::isIntDivCheap(EVT VT, AttributeList Attr) const {
  return VT == MVT::i32;
}

bool SHTargetLowering::allowsMisalignedMemoryAccesses(
    EVT VT, unsigned AddrSpace, Align Alignment, MachineMemOperand::Flags Flags,
    unsigned *Fast) const {
  if (Fast)
    *Fast = 0;
  return false;
}

EVT SHTargetLowering::getOptimalMemOpType(
    LLVMContext &Context, const MemOp &Op,
    const AttributeList &FuncAttributes) const {
  if (Op.isAligned(Align(4)))
    return MVT::i32;
  if (Op.isAligned(Align(2)))
    return MVT::i16;
  return MVT::i8;
}

const char *SHTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case SHISD::RET_GLUE:
    return "SHISD::RET_GLUE";
  case SHISD::CALL:
    return "SHISD::CALL";
  case SHISD::CMP_EQ:
    return "SHISD::CMP_EQ";
  case SHISD::CMP_HS:
    return "SHISD::CMP_HS";
  case SHISD::CMP_GE:
    return "SHISD::CMP_GE";
  case SHISD::CMP_HI:
    return "SHISD::CMP_HI";
  case SHISD::CMP_GT:
    return "SHISD::CMP_GT";
  case SHISD::TST:
    return "SHISD::TST";
  case SHISD::BT:
    return "SHISD::BT";
  case SHISD::BF:
    return "SHISD::BF";
  case SHISD::UDIV:
    return "SHISD::UDIV";
  case SHISD::UREM:
    return "SHISD::UREM";
  case SHISD::UDIVREM:
    return "SHISD::UDIVREM";
  case SHISD::SDIV:
    return "SHISD::SDIV";
  case SHISD::SREM:
    return "SHISD::SREM";
  case SHISD::SDIVREM:
    return "SHISD::SDIVREM";
  case SHISD::SHL_PARTS:
    return "SHISD::SHL_PARTS";
  case SHISD::SRL_PARTS:
    return "SHISD::SRL_PARTS";
  case SHISD::SRA_PARTS:
    return "SHISD::SRA_PARTS";
  case SHISD::SETCC64:
    return "SHISD::SETCC64";
  case SHISD::BR_JT:
    return "SHISD::BR_JT";
  default:
    return nullptr;
  }
}

unsigned SHTargetLowering::getJumpTableEncoding() const {
  if (getTargetMachine().getRelocationModel() != Reloc::Static)
    report_fatal_error("SH jump tables require static relocation");
  return MachineJumpTableInfo::EK_BlockAddress;
}

bool SHTargetLowering::isSuitableForJumpTable(const SwitchInst *SI,
                                              uint64_t NumCases, uint64_t Range,
                                              ProfileSummaryInfo *PSI,
                                              BlockFrequencyInfo *BFI) const {
  if (SI->getCondition()->getType()->getIntegerBitWidth() > 32)
    return false;
  return TargetLowering::isSuitableForJumpTable(SI, NumCases, Range, PSI, BFI);
}
