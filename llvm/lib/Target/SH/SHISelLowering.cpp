//===-- SHISelLowering.cpp - SH SelectionDAG lowering --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHISelLowering.h"
#include "SHSubtarget.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
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

  setOperationAction(ISD::ADD, MVT::i32, Legal);
  setOperationAction(ISD::Constant, MVT::i32, Legal);

  for (unsigned Opcode :
       {ISD::LOAD,      ISD::STORE,         ISD::SUB,
        ISD::MUL,       ISD::MULHU,         ISD::MULHS,
        ISD::SDIV,      ISD::UDIV,          ISD::SREM,
        ISD::UREM,      ISD::AND,           ISD::OR,
        ISD::XOR,       ISD::SHL,           ISD::SRA,
        ISD::SRL,       ISD::ROTL,          ISD::ROTR,
        ISD::BR_CC,     ISD::SELECT,        ISD::SELECT_CC,
        ISD::SETCC,     ISD::GlobalAddress, ISD::BlockAddress,
        ISD::JumpTable, ISD::ConstantPool,  ISD::DYNAMIC_STACKALLOC})
    setOperationAction(Opcode, MVT::i32, Custom);

  computeRegisterProperties(STI.getRegisterInfo());
}

static void requireSupportedCallingConvention(CallingConv::ID CallConv) {
  if (CallConv != CallingConv::C)
    report_fatal_error("SH only supports the C calling convention");
}

static bool isSupportedSHMemoryType(Type *Ty) {
  return Ty->isIntegerTy(32) || Ty->isPointerTy();
}

static void validateSHIR(const Function &F) {
  if (F.hasFnAttribute("stackrealign") ||
      (F.getFnStackAlign() && *F.getFnStackAlign() > Align(4)))
    report_fatal_error("SH stack realignment is not supported");
  for (const BasicBlock &BB : F) {
    if (BB.isEHPad())
      report_fatal_error("SH exception-handling pads are not supported");
    for (const Instruction &I : BB) {
      if (isa<SwitchInst>(&I))
        report_fatal_error("SH switch is not supported");
      if (isa<IndirectBrInst>(&I))
        report_fatal_error("SH indirectbr is not supported");
      if (isa<CallBrInst>(&I))
        report_fatal_error("SH callbr is not supported");
      if (isa<SelectInst>(&I))
        report_fatal_error("SH select is not supported");
      if (const auto *Call = dyn_cast<CallBase>(&I)) {
        requireSupportedCallingConvention(Call->getCallingConv());
        if (isa<InvokeInst>(Call))
          report_fatal_error("SH exception-handling calls are not supported");
        if (Call->isInlineAsm())
          report_fatal_error("SH inline assembly is not supported");
        if (Call->getFunctionType()->isVarArg())
          report_fatal_error("SH varargs calls are not supported");
        if (const auto *CallInst = dyn_cast<llvm::CallInst>(Call)) {
          if (CallInst->isMustTailCall())
            report_fatal_error("SH musttail calls are not supported");
          if (CallInst->isTailCall())
            report_fatal_error("SH tail calls are not supported");
        }

        Type *ReturnTy = Call->getType();
        if (!ReturnTy->isVoidTy() && !ReturnTy->isIntegerTy(32) &&
            !ReturnTy->isPointerTy())
          report_fatal_error(
              "SH calls only support void, i32, and pointer return values");
        for (unsigned ArgNo = 0; ArgNo != Call->arg_size(); ++ArgNo) {
          Type *ArgTy = Call->getArgOperand(ArgNo)->getType();
          if ((!ArgTy->isIntegerTy(32) && !ArgTy->isPointerTy()) ||
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
                "SH calls only support scalar i32 and pointer arguments");
        }
      }
      if (const auto *Phi = dyn_cast<PHINode>(&I)) {
        if (Phi->getType()->isIntegerTy(1))
          report_fatal_error("SH i1 PHIs are not supported");
        if (!Phi->getType()->isIntegerTy(32))
          report_fatal_error("SH only supports i32 PHIs");
      }

      if (const auto *Cmp = dyn_cast<ICmpInst>(&I)) {
        Type *OperandTy = Cmp->getOperand(0)->getType();
        if (OperandTy->isPointerTy())
          report_fatal_error("SH pointer comparisons are not supported");
        if (!OperandTy->isIntegerTy(32))
          report_fatal_error("SH only supports i32 integer comparisons");
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
              "SH only supports 32-bit integer and pointer loads");
        if (Load->isAtomic())
          report_fatal_error("SH atomic loads are not supported");
        if (Load->getAlign() < Align(4))
          report_fatal_error("SH requires 4-byte alignment for 32-bit loads");
        continue;
      }

      if (const auto *Store = dyn_cast<StoreInst>(&I)) {
        if (!isSupportedSHMemoryType(Store->getValueOperand()->getType()))
          report_fatal_error(
              "SH only supports 32-bit integer and pointer stores");
        if (Store->isAtomic())
          report_fatal_error("SH atomic stores are not supported");
        if (Store->getAlign() < Align(4))
          report_fatal_error("SH requires 4-byte alignment for 32-bit stores");
        continue;
      }

      const auto *Alloca = dyn_cast<AllocaInst>(&I);
      if (!Alloca)
        continue;
      if (!Alloca->isStaticAlloca())
        report_fatal_error("SH dynamic alloca is not supported");
      const auto *Count = dyn_cast<ConstantInt>(Alloca->getArraySize());
      if (!Alloca->getAllocatedType()->isIntegerTy(32) || !Count ||
          !Count->isOne())
        report_fatal_error("SH only supports fixed scalar i32 allocas");
      if (Alloca->getAlign() > Align(4))
        report_fatal_error("SH stack object alignment cannot exceed 4 bytes");
      for (const User *Use : Alloca->users()) {
        const auto *Load = dyn_cast<LoadInst>(Use);
        if (Load && Load->getPointerOperand() == Alloca)
          continue;
        const auto *Store = dyn_cast<StoreInst>(Use);
        if (Store && Store->getPointerOperand() == Alloca)
          continue;
        report_fatal_error("SH stack object address escape is not supported");
      }
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
    return false;
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
  if (F.arg_size() != Ins.size())
    report_fatal_error("SH only supports scalar i32 and pointer arguments");

  unsigned Index = 0;
  for (const Argument &Arg : F.args()) {
    const ISD::InputArg &In = Ins[Index++];
    if ((!Arg.getType()->isIntegerTy(32) && !Arg.getType()->isPointerTy()) ||
        In.VT != MVT::i32 || In.Flags.isByVal() || In.Flags.isSRet() ||
        In.Flags.isByRef() || In.Flags.isInAlloca() ||
        In.Flags.isPreallocated() || In.Flags.isNest() ||
        In.Flags.isReturned() || In.Flags.isSwiftSelf() ||
        In.Flags.isSwiftAsync() || In.Flags.isSwiftError())
      report_fatal_error("SH only supports scalar i32 and pointer arguments");
  }

  SmallVector<CCValAssign, 4> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_SH);
  if (ArgLocs.size() != Ins.size())
    report_fatal_error("SH failed to assign all formal arguments");

  MachineRegisterInfo &MRI = MF.getRegInfo();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  SmallVector<SDValue, 4> ArgChains;
  for (const CCValAssign &VA : ArgLocs) {
    if (VA.getLocVT() != MVT::i32 || VA.getLocInfo() != CCValAssign::Full)
      report_fatal_error("SH only supports unextended i32 arguments");
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
    InVals.push_back(Value);
    ArgChains.push_back(Value.getValue(1));
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
  if ((!IsSupportedVoid && !IsSupportedScalar) || Outs.size() != OutVals.size())
    report_fatal_error(
        "SH only supports zero or one scalar i32 or pointer return value");

  SmallVector<CCValAssign, 1> RetLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RetLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeReturn(Outs, RetCC_SH);

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);
  for (unsigned I = 0; I != RetLocs.size(); ++I) {
    const CCValAssign &VA = RetLocs[I];
    if (!VA.isRegLoc() || VA.getLocReg() != SH::R0 ||
        VA.getLocVT() != MVT::i32 || VA.getLocInfo() != CCValAssign::Full)
      report_fatal_error("SH only supports 32-bit returns in r0");
    Chain = DAG.getCopyToReg(Chain, DL, SH::R0, OutVals[I], Glue);
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
  if (!SupportedVoidReturn && !SupportedScalarReturn)
    report_fatal_error(
        "SH calls only support void, i32, and pointer return values");

  for (const ArgListEntry &Arg : CLI.Args) {
    if (!Arg.OrigTy ||
        (!Arg.OrigTy->isIntegerTy(32) && !Arg.OrigTy->isPointerTy()) ||
        Arg.IsByVal || Arg.IsSRet || Arg.IsInAlloca || Arg.IsPreallocated ||
        Arg.IsByRef || Arg.IsNest || Arg.IsReturned || Arg.IsSwiftSelf ||
        Arg.IsSwiftAsync || Arg.IsSwiftError)
      report_fatal_error(
          "SH calls only support scalar i32 and pointer arguments");
  }
  if (CLI.Outs.size() != CLI.Args.size() ||
      CLI.OutVals.size() != CLI.Outs.size())
    report_fatal_error(
        "SH calls only support unsplit scalar i32 and pointer arguments");
  for (const ISD::OutputArg &Out : CLI.Outs) {
    if (Out.VT != MVT::i32 || Out.Flags.isByVal() || Out.Flags.isSRet() ||
        Out.Flags.isByRef() || Out.Flags.isInAlloca() ||
        Out.Flags.isPreallocated() || Out.Flags.isNest() ||
        Out.Flags.isReturned() || Out.Flags.isSwiftSelf() ||
        Out.Flags.isSwiftAsync() || Out.Flags.isSwiftError())
      report_fatal_error(
          "SH calls only support unextended scalar i32 and pointer arguments");
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
          "SH calls only support unextended scalar i32 and pointer arguments");
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

  for (const CCValAssign &VA : RetLocs) {
    if (!VA.isRegLoc() || VA.getLocReg() != SH::R0 ||
        VA.getLocVT() != MVT::i32 || VA.getLocInfo() != CCValAssign::Full)
      report_fatal_error("SH calls only support 32-bit return values in r0");
    SDValue Result = DAG.getCopyFromReg(Chain, CLI.DL, SH::R0, MVT::i32, Glue);
    InVals.push_back(Result);
    Chain = Result.getValue(1);
    Glue = Result.getValue(2);
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
  int64_t ByteDisp = Offset->getSExtValue();
  return ByteDisp >= 0 && ByteDisp <= 60 && ByteDisp % 4 == 0;
}

SDValue SHTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const {
  if (Op.getOpcode() == ISD::BR_CC)
    return LowerBR_CC(Op, DAG);
  if (Op.getOpcode() == ISD::LOAD || Op.getOpcode() == ISD::STORE) {
    const auto *Mem = cast<MemSDNode>(Op);
    if (Mem->getMemoryVT() != MVT::i32)
      report_fatal_error("SH only supports 32-bit memory operations");
    if (Mem->getAlign() < Align(4))
      report_fatal_error("SH requires 4-byte alignment for mov.l");
    if (!isSupportedSHAddress(Mem->getBasePtr()))
      report_fatal_error(
          "SH memory address must be a register or frame index with a "
          "nonnegative aligned byte displacement no greater than 60");
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
  report_fatal_error("SH operation is not supported");
}

SDValue SHTargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const {
  SDValue Chain = Op.getOperand(0);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue Dest = Op.getOperand(4);
  SDLoc DL(Op);

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
  case SHISD::BT:
    return "SHISD::BT";
  case SHISD::BF:
    return "SHISD::BF";
  default:
    return nullptr;
  }
}
