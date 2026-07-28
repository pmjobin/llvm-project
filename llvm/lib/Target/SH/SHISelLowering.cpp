//===-- SHISelLowering.cpp - SH SelectionDAG lowering --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHISelLowering.h"
#include "SH.h"
#include "SHSubtarget.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_CALLING_CONV_IMPL
#include "SHGenCallingConv.inc"

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
        ISD::ConstantPool, ISD::DYNAMIC_STACKALLOC})
    setOperationAction(Opcode, MVT::i32, Custom);

  computeRegisterProperties(STI.getRegisterInfo());
}

static void requireSupportedCallingConvention(CallingConv::ID CallConv) {
  if (CallConv != CallingConv::C)
    report_fatal_error("SH only supports the C calling convention");
}

static bool isSupportedSHMemoryType(Type *Ty) {
  return Ty->isIntegerTy(8) || Ty->isIntegerTy(16) || Ty->isIntegerTy(32) ||
         Ty->isIntegerTy(64) || Ty->isPointerTy();
}

static bool isSupportedSHScalarType(Type *Ty) {
  return Ty->isIntegerTy(32) || Ty->isIntegerTy(64) || Ty->isPointerTy();
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
      report_fatal_error("SH stack object address escape is not supported");
    }
  }
}

static bool containsUnsupportedSHAddressConstant(const Value *V) {
  if (isa<GlobalValue>(V) || isa<BlockAddress>(V))
    return true;
  const auto *C = dyn_cast<Constant>(V);
  if (!C)
    return false;
  for (const Use &Operand : C->operands())
    if (containsUnsupportedSHAddressConstant(Operand.get()))
      return true;
  return false;
}

void llvm::validateSHIR(const Function &F) {
  if (F.getReturnType()->isIntegerTy(1))
    report_fatal_error(
        "SH comparison results may only be used by conditional branches");
  if (!F.getReturnType()->isVoidTy() &&
      !isSupportedSHScalarType(F.getReturnType()))
    report_fatal_error(
        "SH functions only support void, i32, i64, and pointer return values");
  for (const Argument &Arg : F.args())
    if (!isSupportedSHScalarType(Arg.getType()))
      report_fatal_error(
          "SH function arguments must be scalar i32, i64, or pointers");
  if (F.isVarArg())
    report_fatal_error("SH varargs are not supported");
  if (F.hasFnAttribute("stackrealign") ||
      (F.getFnStackAlign() && *F.getFnStackAlign() > Align(4)))
    report_fatal_error("SH stack realignment is not supported");
  for (const BasicBlock &BB : F) {
    if (BB.isEHPad())
      report_fatal_error("SH exception-handling pads are not supported");
    for (const Instruction &I : BB) {
      const auto *Call = dyn_cast<CallBase>(&I);
      for (const Use &Operand : I.operands()) {
        if (Call && Operand.get() == Call->getCalledOperand())
          continue;
        if (containsUnsupportedSHAddressConstant(Operand.get()))
          report_fatal_error(
              "SH global, function, and block address constants are not "
              "supported");
      }
      if (isa<SwitchInst>(&I))
        report_fatal_error("SH switch is not supported");
      if (isa<IndirectBrInst>(&I))
        report_fatal_error("SH indirectbr is not supported");
      if (isa<CallBrInst>(&I))
        report_fatal_error("SH callbr is not supported");
      if (isa<SelectInst>(&I))
        report_fatal_error("SH select is not supported");
      if (isa<AtomicRMWInst>(&I) || isa<AtomicCmpXchgInst>(&I))
        report_fatal_error(
            "SH atomic read-modify-write operations are not supported");
      if (Call) {
        requireSupportedCallingConvention(Call->getCallingConv());
        if (isa<InvokeInst>(Call))
          report_fatal_error("SH exception-handling calls are not supported");
        if (Call->isInlineAsm())
          report_fatal_error("SH inline assembly is not supported");
        if (Call->getIntrinsicID() != Intrinsic::not_intrinsic)
          report_fatal_error("SH intrinsics are not supported");
        if (Call->getFunctionType()->isVarArg())
          report_fatal_error("SH varargs calls are not supported");
        if (const auto *CallInst = dyn_cast<llvm::CallInst>(Call)) {
          if (CallInst->isMustTailCall())
            report_fatal_error("SH musttail calls are not supported");
          if (CallInst->isTailCall())
            report_fatal_error("SH tail calls are not supported");
        }

        Type *ReturnTy = Call->getType();
        if (!ReturnTy->isVoidTy() && !isSupportedSHScalarType(ReturnTy))
          report_fatal_error(
              "SH calls only support void, i32, i64, and pointer return "
              "values");
        for (unsigned ArgNo = 0; ArgNo != Call->arg_size(); ++ArgNo) {
          Type *ArgTy = Call->getArgOperand(ArgNo)->getType();
          if (!isSupportedSHScalarType(ArgTy) ||
              Call->paramHasAttr(ArgNo, Attribute::ByVal) ||
              Call->paramHasAttr(ArgNo, Attribute::StructRet) ||
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
      }
      if (const auto *Phi = dyn_cast<PHINode>(&I)) {
        if (Phi->getType()->isIntegerTy(1))
          report_fatal_error("SH i1 PHIs are not supported");
        if (!Phi->getType()->isIntegerTy(8) &&
            !Phi->getType()->isIntegerTy(16) &&
            !Phi->getType()->isIntegerTy(32) &&
            !Phi->getType()->isIntegerTy(64))
          report_fatal_error("SH only supports i8, i16, i32, and i64 PHIs");
      }

      if (const auto *Cmp = dyn_cast<ICmpInst>(&I)) {
        Type *OperandTy = Cmp->getOperand(0)->getType();
        if (OperandTy->isPointerTy())
          report_fatal_error("SH pointer comparisons are not supported");
        if (!OperandTy->isIntegerTy(8) && !OperandTy->isIntegerTy(16) &&
            !OperandTy->isIntegerTy(32) && !OperandTy->isIntegerTy(64))
          report_fatal_error(
              "SH only supports i8, i16, i32, and i64 integer comparisons");
        for (const User *Use : Cmp->users()) {
          const auto *Branch = dyn_cast<CondBrInst>(Use);
          if (!Branch || Branch->getCondition() != Cmp)
            report_fatal_error(
                "SH comparison results may only be used by conditional "
                "branches");
        }
        continue;
      }

      if (const auto *Load = dyn_cast<LoadInst>(&I)) {
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

bool SHTargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context,
    const Type *RetTy) const {
  if (RetTy->isIntegerTy(1))
    report_fatal_error(
        "SH comparison results may only be used by conditional branches");
  if (CallConv != CallingConv::C || IsVarArg || Outs.size() > 1)
    return RetTy->isIntegerTy(64) && CallConv == CallingConv::C && !IsVarArg &&
           Outs.size() == 2 &&
           llvm::all_of(Outs, [](const ISD::OutputArg &Out) {
             return Out.VT == MVT::i32;
           });
  if (RetTy->isVoidTy())
    return Outs.empty();
  return (RetTy->isIntegerTy(32) || RetTy->isPointerTy()) && Outs.size() == 1 &&
         Outs[0].VT == MVT::i32;
}

SDValue SHTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  requireSupportedCallingConvention(CallConv);
  MachineFunction &MF = DAG.getMachineFunction();
  const Function &F = MF.getFunction();
  validateSHIR(F);
  if (IsVarArg || F.isVarArg())
    report_fatal_error("SH varargs are not supported");
  if (F.hasStructRetAttr())
    report_fatal_error("SH structure returns are not supported");
  unsigned ExpectedParts = 0;
  for (const Argument &Arg : F.args())
    ExpectedParts += Arg.getType()->isIntegerTy(64) ? 2 : 1;
  if (ExpectedParts != Ins.size())
    report_fatal_error(
        "SH failed to split scalar arguments into 32-bit ABI words");

  for (const ISD::InputArg &In : Ins) {
    if (!isSupportedSHScalarType(In.OrigTy) || In.VT != MVT::i32 ||
        In.Flags.isByVal() || In.Flags.isSRet() || In.Flags.isByRef() ||
        In.Flags.isInAlloca() || In.Flags.isPreallocated() ||
        In.Flags.isNest() || In.Flags.isReturned() || In.Flags.isSwiftSelf() ||
        In.Flags.isSwiftAsync() || In.Flags.isSwiftError())
      report_fatal_error(
          "SH only supports scalar i32, i64, and pointer arguments");
  }

  SmallVector<CCValAssign, 4> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_SH);
  if (ArgLocs.size() != Ins.size())
    report_fatal_error("SH failed to assign all formal arguments");

  MachineRegisterInfo &MRI = MF.getRegInfo();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  SmallVector<SDValue, 4> ArgChains;
  SmallVector<SDValue, 8> ABIValues(Ins.size());
  for (unsigned I = 0; I != ArgLocs.size(); ++I) {
    const CCValAssign &VA = ArgLocs[I];
    if (VA.getLocVT() != MVT::i32 || VA.getLocInfo() != CCValAssign::Full)
      report_fatal_error("SH only supports unextended 32-bit ABI words");
    SDValue Value;
    if (VA.isRegLoc()) {
      Register VReg = MRI.createVirtualRegister(&SH::GPRRegClass);
      MRI.addLiveIn(VA.getLocReg(), VReg);
      Value = DAG.getCopyFromReg(Chain, DL, VReg, MVT::i32);
    } else {
      if (!VA.isMemLoc() || VA.getLocMemOffset() < 0 ||
          VA.getLocMemOffset() % 4 != 0)
        report_fatal_error(
            "SH stack arguments must use four-byte aligned nonnegative "
            "offsets");
      int FI =
          MFI.CreateFixedObject(4, VA.getLocMemOffset(), /*IsImmutable=*/true);
      SDValue FrameIndex = DAG.getFrameIndex(FI, MVT::i32);
      Value = DAG.getLoad(MVT::i32, DL, Chain, FrameIndex,
                          MachinePointerInfo::getFixedStack(MF, FI), Align(4));
    }
    ABIValues[I] = Value;
    ArgChains.push_back(Value.getValue(1));
  }
  for (SDValue Value : ABIValues) {
    if (!Value)
      report_fatal_error("SH failed to assign an incoming ABI word");
    InVals.push_back(Value);
  }
  if (!ArgChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, ArgChains);
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
  if (IsVarArg || F.isVarArg())
    report_fatal_error("SH varargs are not supported");
  bool IsSupportedVoid = F.getReturnType()->isVoidTy() && Outs.empty();
  bool IsSupportedScalar = (F.getReturnType()->isIntegerTy(32) ||
                            F.getReturnType()->isPointerTy()) &&
                           Outs.size() == 1;
  IsSupportedScalar |= F.getReturnType()->isIntegerTy(64) && Outs.size() == 2;
  if ((!IsSupportedVoid && !IsSupportedScalar) || Outs.size() != OutVals.size())
    report_fatal_error(
        "SH only supports scalar i32, i64, and pointer return values");

  SmallVector<CCValAssign, 1> RetLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RetLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeReturn(Outs, RetCC_SH);

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);
  for (unsigned I = 0; I != RetLocs.size(); ++I) {
    const CCValAssign &VA = RetLocs[I];
    MCRegister ExpectedReg = I == 0 ? SH::R0 : SH::R1;
    if (!VA.isRegLoc() || VA.getLocReg() != ExpectedReg ||
        VA.getLocVT() != MVT::i32 || VA.getLocInfo() != CCValAssign::Full)
      report_fatal_error("SH scalar returns must use r0 and optionally r1");
    Chain = DAG.getCopyToReg(Chain, DL, ExpectedReg, OutVals[I], Glue);
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(ExpectedReg, MVT::i32));
  }
  RetOps[0] = Chain;
  if (Glue)
    RetOps.push_back(Glue);
  return DAG.getNode(SHISD::RET_GLUE, DL, MVT::Other, RetOps);
}

SDValue SHTargetLowering::LowerCall(CallLoweringInfo &CLI,
                                    SmallVectorImpl<SDValue> &InVals) const {
  requireSupportedCallingConvention(CLI.CallConv);
  if (CLI.IsVarArg)
    report_fatal_error("SH varargs calls are not supported");
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
  bool SupportedVoidReturn =
      ReturnTy && ReturnTy->isVoidTy() && CLI.Ins.empty();
  bool SupportedScalarReturn =
      ReturnTy && (ReturnTy->isIntegerTy(32) || ReturnTy->isPointerTy()) &&
      CLI.Ins.size() == 1 && CLI.Ins[0].VT == MVT::i32;
  SupportedScalarReturn |=
      ReturnTy && ReturnTy->isIntegerTy(64) && CLI.Ins.size() == 2 &&
      llvm::all_of(CLI.Ins,
                   [](const ISD::InputArg &In) { return In.VT == MVT::i32; });
  if (!SupportedVoidReturn && !SupportedScalarReturn)
    report_fatal_error(
        "SH calls only support void, i32, i64, and pointer return values");

  for (const ArgListEntry &Arg : CLI.Args) {
    if (!Arg.OrigTy || !isSupportedSHScalarType(Arg.OrigTy) || Arg.IsByVal ||
        Arg.IsSRet || Arg.IsInAlloca || Arg.IsPreallocated || Arg.IsByRef ||
        Arg.IsNest || Arg.IsReturned || Arg.IsSwiftSelf || Arg.IsSwiftAsync ||
        Arg.IsSwiftError)
      report_fatal_error(
          "SH calls only support scalar i32, i64, and pointer arguments");
  }
  if (CLI.OutVals.size() != CLI.Outs.size())
    report_fatal_error(
        "SH failed to split call arguments into 32-bit ABI words");
  for (const ISD::OutputArg &Out : CLI.Outs) {
    if (!isSupportedSHScalarType(Out.OrigTy) || Out.VT != MVT::i32 ||
        Out.Flags.isByVal() || Out.Flags.isSRet() || Out.Flags.isByRef() ||
        Out.Flags.isInAlloca() || Out.Flags.isPreallocated() ||
        Out.Flags.isNest() || Out.Flags.isReturned() ||
        Out.Flags.isSwiftSelf() || Out.Flags.isSwiftAsync() ||
        Out.Flags.isSwiftError())
      report_fatal_error(
          "SH calls only support unextended 32-bit scalar ABI words");
  }

  SelectionDAG &DAG = CLI.DAG;
  MachineFunction &MF = DAG.getMachineFunction();
  SmallVector<CCValAssign, 4> ArgLocs;
  CCState ArgCCInfo(CLI.CallConv, CLI.IsVarArg, MF, ArgLocs, *DAG.getContext());
  ArgCCInfo.AnalyzeCallOperands(CLI.Outs, CC_SH);
  if (ArgLocs.size() != CLI.Outs.size())
    report_fatal_error("SH failed to assign all call arguments");
  unsigned StackBytes = ArgCCInfo.getStackSize();
  if (StackBytes > 60)
    report_fatal_error("SH outgoing call frame size cannot exceed 60 bytes");
  if (StackBytes % 4 != 0)
    report_fatal_error("SH outgoing call frame size must be four-byte aligned");

  SmallVector<CCValAssign, 1> RetLocs;
  CCState RetCCInfo(CLI.CallConv, CLI.IsVarArg, MF, RetLocs, *DAG.getContext());
  RetCCInfo.AnalyzeCallResult(CLI.Ins, RetCC_SH);
  if (RetLocs.size() != CLI.Ins.size())
    report_fatal_error("SH failed to assign the call return value");

  SDValue Chain = DAG.getCALLSEQ_START(CLI.Chain, StackBytes, 0, CLI.DL);
  SmallVector<std::pair<MCRegister, SDValue>, 4> RegsToPass;
  SmallVector<SDValue, 4> StoreChains;
  SDValue StackPtr;
  for (unsigned I = 0; I != ArgLocs.size(); ++I) {
    const CCValAssign &VA = ArgLocs[I];
    if (VA.getLocVT() != MVT::i32 || VA.getLocInfo() != CCValAssign::Full)
      report_fatal_error(
          "SH calls only support unextended 32-bit scalar ABI words");
    if (VA.isRegLoc()) {
      RegsToPass.emplace_back(VA.getLocReg(), CLI.OutVals[I]);
      continue;
    }
    if (!VA.isMemLoc() || VA.getLocMemOffset() < 0 ||
        VA.getLocMemOffset() > 56 || VA.getLocMemOffset() % 4 != 0)
      report_fatal_error(
          "SH outgoing stack argument offset must be four-byte aligned and "
          "in [0, 56]");
    if (!StackPtr)
      StackPtr = DAG.getRegister(SH::R15, MVT::i32);
    SDValue Address =
        DAG.getNode(ISD::ADD, CLI.DL, MVT::i32, StackPtr,
                    DAG.getIntPtrConstant(VA.getLocMemOffset(), CLI.DL));
    StoreChains.push_back(DAG.getStore(
        Chain, CLI.DL, CLI.OutVals[I], Address,
        MachinePointerInfo::getStack(MF, VA.getLocMemOffset()), Align(4)));
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
    if (!CalleeFunction || CalleeFunction->isDeclarationForLinker() ||
        CalleeFunction->isInterposable() || !CalleeFunction->isDSOLocal())
      report_fatal_error(
          "SH unresolved or interposable direct calls are not supported");

    auto EffectiveSection = [](const Function &F) {
      return F.getSection().empty() ? StringRef(".text") : F.getSection();
    };
    if (EffectiveSection(Caller) != EffectiveSection(*CalleeFunction))
      report_fatal_error("SH cross-section direct calls are not supported");

    Callee = DAG.getTargetGlobalAddress(CalleeFunction, CLI.DL, MVT::i32,
                                        Global->getOffset());
  } else if (isa<ExternalSymbolSDNode>(Callee)) {
    report_fatal_error("SH unresolved external direct calls are not supported");
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

  SmallVector<SDValue, 2> ABIResults(CLI.Ins.size());
  for (unsigned I = 0; I != RetLocs.size(); ++I) {
    const CCValAssign &VA = RetLocs[I];
    MCRegister ExpectedReg = I == 0 ? SH::R0 : SH::R1;
    if (!VA.isRegLoc() || VA.getLocReg() != ExpectedReg ||
        VA.getLocVT() != MVT::i32 || VA.getLocInfo() != CCValAssign::Full)
      report_fatal_error(
          "SH scalar call results must use r0 and optionally r1");
    SDValue Result =
        DAG.getCopyFromReg(Chain, CLI.DL, ExpectedReg, MVT::i32, Glue);
    ABIResults[I] = Result;
    Chain = Result.getValue(1);
    Glue = Result.getValue(2);
  }
  for (SDValue Result : ABIResults) {
    if (!Result)
      report_fatal_error("SH failed to assign a returned ABI word");
    InVals.push_back(Result);
  }

  return Chain;
}

static bool isSupportedSHAddress(SDValue Addr) {
  auto IsBase = [](SDValue Base) {
    return Base.getValueType() == MVT::i32 &&
           (Base.getOpcode() == ISD::FrameIndex ||
            Base.getOpcode() == ISD::CopyFromReg ||
            Base.getOpcode() == ISD::LOAD || Base.getOpcode() == ISD::Register);
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
            Base.getOpcode() == ISD::LOAD || Base.getOpcode() == ISD::Register);
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
        report_fatal_error("SH requires 4-byte alignment for mov.l");
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
      report_fatal_error("SH requires 2-byte alignment for mov.w");
    if (!isSupportedSHNarrowAddress(Mem->getBasePtr()))
      report_fatal_error(
          "SH byte/word memory address must be a register, frame index, or "
          "supported 32-bit address addition");
    return Op;
  }
  if (Op.getOpcode() == ISD::DYNAMIC_STACKALLOC)
    report_fatal_error("SH dynamic alloca is not supported");
  if (Op.getOpcode() == ISD::GlobalAddress ||
      Op.getOpcode() == ISD::BlockAddress || Op.getOpcode() == ISD::JumpTable ||
      Op.getOpcode() == ISD::ConstantPool)
    report_fatal_error("SH symbolic memory addresses are not supported");
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
  default:
    return nullptr;
  }
}
