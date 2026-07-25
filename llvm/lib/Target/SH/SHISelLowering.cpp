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

  for (unsigned Opcode : {ISD::LOAD,         ISD::STORE,
                          ISD::SUB,          ISD::MUL,
                          ISD::MULHU,        ISD::MULHS,
                          ISD::SDIV,         ISD::UDIV,
                          ISD::SREM,         ISD::UREM,
                          ISD::AND,          ISD::OR,
                          ISD::XOR,          ISD::SHL,
                          ISD::SRA,          ISD::SRL,
                          ISD::ROTL,         ISD::ROTR,
                          ISD::BR,           ISD::BR_CC,
                          ISD::SELECT,       ISD::SELECT_CC,
                          ISD::SETCC,        ISD::GlobalAddress,
                          ISD::BlockAddress, ISD::JumpTable,
                          ISD::ConstantPool, ISD::DYNAMIC_STACKALLOC})
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

static void validateSHMemoryIR(const Function &F) {
  for (const BasicBlock &BB : F) {
    for (const Instruction &I : BB) {
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
  validateSHMemoryIR(F);
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
        In.Flags.isInAlloca())
      report_fatal_error("SH only supports scalar i32 and pointer arguments");
  }

  SmallVector<CCValAssign, 4> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_SH);
  if (ArgLocs.size() != Ins.size())
    report_fatal_error("SH failed to assign all formal arguments");

  MachineRegisterInfo &MRI = MF.getRegInfo();
  SmallVector<SDValue, 4> CopyChains;
  for (const CCValAssign &VA : ArgLocs) {
    if (!VA.isRegLoc())
      report_fatal_error("SH stack-passed arguments are not supported");
    if (VA.getLocVT() != MVT::i32 || VA.getLocInfo() != CCValAssign::Full)
      report_fatal_error("SH only supports unextended i32 arguments");
    Register VReg = MRI.createVirtualRegister(&SH::GPRRegClass);
    MRI.addLiveIn(VA.getLocReg(), VReg);
    SDValue Value = DAG.getCopyFromReg(Chain, DL, VReg, MVT::i32);
    InVals.push_back(Value);
    CopyChains.push_back(Value.getValue(1));
  }
  if (!CopyChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, CopyChains);
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
  report_fatal_error("SH function calls are not supported");
}

static bool isSupportedSHAddress(SDValue Addr) {
  auto IsBase = [](SDValue Base) {
    return Base.getValueType() == MVT::i32 &&
           (Base.getOpcode() == ISD::FrameIndex ||
            Base.getOpcode() == ISD::CopyFromReg ||
            Base.getOpcode() == ISD::LOAD);
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
  report_fatal_error("SH operation is not supported");
}

bool SHTargetLowering::allowsMisalignedMemoryAccesses(
    EVT VT, unsigned AddrSpace, Align Alignment, MachineMemOperand::Flags Flags,
    unsigned *Fast) const {
  if (Fast)
    *Fast = 0;
  return false;
}

const char *SHTargetLowering::getTargetNodeName(unsigned Opcode) const {
  if (Opcode == SHISD::RET_GLUE)
    return "SHISD::RET_GLUE";
  return nullptr;
}
