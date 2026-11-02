//===-- SHISelLowering.cpp - SH SelectionDAG lowering --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHISelLowering.h"
#include "SH.h"
#include "SHCallFrameState.h"
#include "SHConstantPoolValue.h"
#include "SHIRValidation.h"
#include "SHMachineFunctionInfo.h"
#include "SHSubtarget.h"
#include "SHTargetMachine.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/CodeGen/Analysis.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/EHPersonalities.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicsSH.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

SHTargetLowering::SHTargetLowering(const TargetMachine &TM,
                                   const SHSubtarget &STI)
    : TargetLowering(TM, STI), Subtarget(STI),
      OriginalOptLevel(TM.getOptLevel()) {
  addRegisterClass(MVT::i32, &SH::GPRRegClass);
  if (STI.hasFPUVector()) {
    addRegisterClass(MVT::v4f32, &SH::FVRegClass);
    // Transport is native; ordinary vector arithmetic is scalarized.
    for (unsigned Op = 0; Op != ISD::BUILTIN_OP_END; ++Op)
      setOperationAction(Op, MVT::v4f32, Expand);
    for (unsigned Op : {ISD::LOAD, ISD::STORE, ISD::BUILD_VECTOR, ISD::UNDEF,
                        ISD::INTRINSIC_W_CHAIN})
      setOperationAction(Op, MVT::v4f32, Legal);
    setOperationAction(ISD::EXTRACT_VECTOR_ELT, MVT::v4f32, Custom);
    setOperationAction(ISD::INSERT_VECTOR_ELT, MVT::v4f32, Custom);
  }
  if (STI.hasSingleFPU()) {
    addRegisterClass(MVT::f32, &SH::FRRegClass);
    for (unsigned Opcode :
         {ISD::FADD, ISD::FSUB, ISD::FMUL, ISD::FDIV, ISD::FNEG, ISD::FABS})
      setOperationAction(Opcode, MVT::f32, Legal);
    setOperationAction(ISD::FSQRT, MVT::f32, STI.hasFSQRT() ? Legal : LibCall);
    setOperationAction(ISD::SETCC, MVT::f32, Custom);
    setOperationAction(ISD::SELECT, MVT::f32, Custom);
    setOperationAction(ISD::BR_CC, MVT::f32, Expand);
    setOperationAction(ISD::SELECT_CC, MVT::f32, Expand);
    setOperationAction(ISD::FREM, MVT::f32, LibCall);
    setOperationAction(ISD::FMA, MVT::f32, Legal);
    setOperationAction(ISD::FCOPYSIGN, MVT::f32, Expand);
    setOperationAction(ISD::ConstantFP, MVT::f32, Custom);
    setOperationAction(ISD::LOAD, MVT::f32, Legal);
    setOperationAction(ISD::STORE, MVT::f32, Legal);
    setOperationAction(ISD::SINT_TO_FP, MVT::i32, Custom);
    setOperationAction(ISD::FP_TO_SINT, MVT::i32, Custom);
    setOperationAction(ISD::UINT_TO_FP, MVT::i32, LibCall);
    setOperationAction(ISD::FP_TO_UINT, MVT::i32, Custom);
  }
  if (STI.hasDoubleFPU()) {
    addRegisterClass(MVT::f64, &SH::DRRegClass);
    for (unsigned Opcode :
         {ISD::FADD, ISD::FSUB, ISD::FMUL, ISD::FDIV, ISD::FNEG, ISD::FABS})
      setOperationAction(Opcode, MVT::f64, Legal);
    setOperationAction(ISD::FSQRT, MVT::f64, STI.hasFSQRT() ? Legal : LibCall);
    setOperationAction(ISD::SETCC, MVT::f64, Custom);
    setOperationAction(ISD::SELECT, MVT::f64, Custom);
    setOperationAction(ISD::BR_CC, MVT::f64, Expand);
    setOperationAction(ISD::SELECT_CC, MVT::f64, Expand);
    setOperationAction(ISD::FREM, MVT::f64, LibCall);
    setOperationAction(ISD::FMA, MVT::f64, Expand);
    setOperationAction(ISD::FCOPYSIGN, MVT::f64, Expand);
    setOperationAction(ISD::ConstantFP, MVT::f64, Custom);
    setOperationAction(ISD::LOAD, MVT::f64, STI.hasSH2A() ? Legal : Custom);
    setOperationAction(ISD::STORE, MVT::f64, STI.hasSH2A() ? Legal : Custom);
    setOperationAction(ISD::FP_EXTEND, MVT::f64, Custom);
    setOperationAction(ISD::FP_ROUND, MVT::f32, Custom);
    setLoadExtAction(ISD::EXTLOAD, MVT::f64, MVT::f32, Expand);
    setTruncStoreAction(MVT::f64, MVT::f32, Expand);
  }
  setStackPointerRegisterToSaveRestore(SH::R15);
  setMinFunctionAlignment(Align(2));
  setPrefFunctionAlignment(Align(4));

  for (unsigned Opcode : {ISD::ADD, ISD::SUB, ISD::XOR})
    setOperationAction(Opcode, MVT::i32, Legal);
  setOperationAction(ISD::MUL, MVT::i32, STI.hasSH2() ? Legal : Custom);
  for (unsigned Opcode : {ISD::SHL, ISD::SRA, ISD::SRL})
    setOperationAction(Opcode, MVT::i32,
                       STI.hasDynamicShift() ||
                               TM.getOptLevel() == CodeGenOptLevel::None
                           ? Legal
                           : Custom);
  for (unsigned Opcode : {ISD::ADDC, ISD::ADDE, ISD::SUBC, ISD::SUBE})
    setOperationAction(Opcode, MVT::i32, Legal);
  for (unsigned Opcode : {ISD::UADDO, ISD::SADDO, ISD::USUBO, ISD::SSUBO})
    setOperationAction(Opcode, MVT::i32, Custom);
  for (unsigned Opcode : {ISD::SHL_PARTS, ISD::SRL_PARTS, ISD::SRA_PARTS})
    setOperationAction(Opcode, MVT::i32, Custom);
  setOperationAction(ISD::SETCC, MVT::i64, Custom);
  setOperationAction(ISD::LOAD, MVT::i64, Custom);
  setOperationAction(ISD::STORE, MVT::i64, Custom);
  setOperationAction(ISD::Constant, MVT::i32, Legal);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1, Custom);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i32, Legal);
  setBooleanContents(ZeroOrOneBooleanContent);
  for (unsigned Opcode :
       {ISD::BITREVERSE, ISD::BSWAP, ISD::CTLZ, ISD::CTLZ_ZERO_POISON,
        ISD::CTTZ, ISD::CTTZ_ZERO_POISON, ISD::SMIN, ISD::SMAX, ISD::UMIN,
        ISD::UMAX})
    setOperationAction(Opcode, MVT::i32, Expand);
  setOperationAction({ISD::ABS, ISD::ABS_MIN_POISON}, MVT::i32, Expand);
  setOperationAction({ISD::SADDSAT, ISD::SSUBSAT, ISD::UADDSAT, ISD::USUBSAT},
                     MVT::i32, Expand);
  // A custom scalar CTPOP keeps generic CTTZ expansion away from its
  // constant-pool table path, which requires an address-valued constant pool.
  setOperationAction(ISD::CTPOP, MVT::i32, Custom);
  setOperationAction(ISD::SMULO, MVT::i32, Custom);
  setOperationAction(ISD::UMULO, MVT::i32, Custom);
  setOperationAction(ISD::SMUL_LOHI, MVT::i32, Custom);
  setOperationAction(ISD::UMUL_LOHI, MVT::i32, Custom);
  for (unsigned Opcode : {ISD::SDIV, ISD::UDIV})
    setOperationAction(Opcode, MVT::i32, STI.hasSH2A() ? Legal : Expand);
  for (unsigned Opcode : {ISD::SREM, ISD::UREM})
    setOperationAction(Opcode, MVT::i32, Expand);
  setOperationAction(ISD::SDIVREM, MVT::i32, Custom);
  setOperationAction(ISD::UDIVREM, MVT::i32, Custom);
  // Preserve i64 div/rem for SelectionDAG's standard runtime-libcall
  // expansion instead of expanding it prematurely in IR.
  setMaxDivRemBitWidthSupported(64);
  setOperationAction(ISD::FSHL, MVT::i32, Expand);
  setOperationAction(ISD::FSHR, MVT::i32, Expand);

  for (MVT MemVT : {MVT::i8, MVT::i16}) {
    LegalizeAction Action = MemVT == MVT::i16 ? Custom : Legal;
    setLoadExtAction(ISD::EXTLOAD, MVT::i32, MemVT, Action);
    setLoadExtAction(ISD::SEXTLOAD, MVT::i32, MemVT, Action);
    setLoadExtAction(ISD::ZEXTLOAD, MVT::i32, MemVT, Action);
    setTruncStoreAction(MVT::i32, MemVT, Action);
  }
  for (unsigned Opcode : {ISD::EXTLOAD, ISD::SEXTLOAD, ISD::ZEXTLOAD})
    setLoadExtAction(Opcode, MVT::i32, MVT::i1, Promote);
  setTruncStoreAction(MVT::i32, MVT::i1, Expand);
  for (MVT MemVT : {MVT::i8, MVT::i16, MVT::i32}) {
    setIndexedLoadAction(ISD::POST_INC, MemVT, Legal);
    setIndexedStoreAction(ISD::PRE_DEC, MemVT, Legal);
  }

  for (unsigned Opcode :
       {ISD::LOAD, ISD::STORE, ISD::MULHU, ISD::MULHS, ISD::AND, ISD::OR,
        ISD::ROTL, ISD::ROTR, ISD::BR_CC, ISD::SELECT, ISD::SETCC,
        ISD::GlobalAddress, ISD::BlockAddress, ISD::JumpTable,
        ISD::ConstantPool, ISD::GlobalTLSAddress})
    setOperationAction(Opcode, MVT::i32, Custom);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i32, Expand);
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);
  setOperationAction(ISD::FRAMEADDR, MVT::i32, Custom);
  setOperationAction(ISD::RETURNADDR, MVT::i32, Custom);
  setOperationAction(ISD::SELECT_CC, MVT::i32,
                     TM.getOptLevel() == CodeGenOptLevel::None ? Expand
                                                               : Custom);
  setOperationAction(ISD::BR_JT, MVT::Other, Custom);
  setOperationAction(ISD::VASTART, MVT::Other, Custom);
  setOperationAction(ISD::ATOMIC_FENCE, MVT::Other, Custom);
  setOperationAction(ISD::EH_RETURN, MVT::Other, Custom);
  // Lower the complete argument before type legalization splits i64/f64.
  for (MVT VT : {MVT::Other, MVT::i64, MVT::f64})
    setOperationAction(ISD::VAARG, VT, Custom);
  setOperationAction(ISD::VACOPY, MVT::Other,
                     STI.useHardFloatABI() ? Custom : Expand);
  setOperationAction(ISD::VAEND, MVT::Other, Expand);
  setOperationAction(ISD::TRAP, MVT::Other, Expand);
  setOperationAction(ISD::INTRINSIC_WO_CHAIN, MVT::Other, Custom);
  setMinimumJumpTableEntries(4);

  setTargetDAGCombine(ISD::BSWAP);
  setTargetDAGCombine(ISD::ADD);
  setTargetDAGCombine(ISD::MUL);
  setTargetDAGCombine(ISD::SIGN_EXTEND_INREG);
  setTargetDAGCombine(ISD::FREEZE);
  setTargetDAGCombine(ISD::SHL);
  setTargetDAGCombine(ISD::SRL);
  // Use the standard alignment-aware memory expansion limits: eight stores,
  // or four for size optimization. SelectionDAG owns the libcall decision.
  setMaxAtomicSizeInBitsSupported(0);
  computeRegisterProperties(STI.getRegisterInfo());
}

bool SHTargetLowering::isFMAFasterThanFMulAndFAdd(const MachineFunction &,
                                                  EVT VT) const {
  return VT == MVT::f32 && Subtarget.hasSingleFPU();
}

MVT SHTargetLowering::getRegisterTypeForCallingConv(LLVMContext &Context,
                                                    CallingConv::ID CC,
                                                    EVT VT) const {
  if (VT == MVT::v4f32 || VT == MVT::v2f32)
    return MVT::i32;
  if ((VT == MVT::f32 && !Subtarget.useHardFloatABI()) ||
      (VT == MVT::f64 && !Subtarget.usesSH4HardFloatABI()))
    return MVT::i32;
  return TargetLowering::getRegisterTypeForCallingConv(Context, CC, VT);
}

unsigned SHTargetLowering::getNumRegistersForCallingConv(LLVMContext &Context,
                                                         CallingConv::ID CC,
                                                         EVT VT) const {
  if (VT == MVT::v4f32 || VT == MVT::v2f32)
    return VT.getVectorNumElements();
  if (!Subtarget.useHardFloatABI() && VT == MVT::f32)
    return 1;
  if (!Subtarget.usesSH4HardFloatABI() && VT == MVT::f64)
    return 2;
  return TargetLowering::getNumRegistersForCallingConv(Context, CC, VT);
}

bool SHTargetLowering::splitValueIntoRegisterParts(
    SelectionDAG &DAG, const SDLoc &DL, SDValue Val, SDValue *Parts,
    unsigned NumParts, MVT PartVT, std::optional<CallingConv::ID> CC) const {
  if (CC &&
      (Val.getValueType() == MVT::v4f32 || Val.getValueType() == MVT::v2f32) &&
      NumParts == Val.getValueType().getVectorNumElements() &&
      PartVT == MVT::i32) {
    for (unsigned I = 0; I != NumParts; ++I)
      Parts[I] =
          DAG.getNode(ISD::BITCAST, DL, MVT::i32,
                      DAG.getNode(ISD::EXTRACT_VECTOR_ELT, DL, MVT::f32, Val,
                                  DAG.getConstant(I, DL, MVT::i32)));
    return true;
  }
  if (!CC || !Subtarget.hasDoubleFPU() || Subtarget.useHardFloatABI() ||
      PartVT != MVT::i32)
    return false;
  if (Val.getValueType() == MVT::f32 && NumParts == 1) {
    Parts[0] = DAG.getNode(SHISD::FR_TO_GPR, DL, MVT::i32, Val);
    return true;
  }
  if (Val.getValueType() != MVT::f64 || NumParts != 2)
    return false;
  SDValue Pair = DAG.getNode(SHISD::DR_TO_GPR_PAIR, DL,
                             DAG.getVTList(MVT::i32, MVT::i32), Val);
  SDValue MSW = Pair.getValue(0);
  SDValue LSW = Pair.getValue(1);
  Parts[0] = DAG.getDataLayout().isBigEndian() ? MSW : LSW;
  Parts[1] = DAG.getDataLayout().isBigEndian() ? LSW : MSW;
  return true;
}

SDValue SHTargetLowering::joinRegisterPartsIntoValue(
    SelectionDAG &DAG, const SDLoc &DL, const SDValue *Parts, unsigned NumParts,
    MVT PartVT, EVT ValueVT, std::optional<CallingConv::ID> CC) const {
  if (CC && (ValueVT == MVT::v4f32 || ValueVT == MVT::v2f32) &&
      NumParts == ValueVT.getVectorNumElements() && PartVT == MVT::i32) {
    SmallVector<SDValue, 4> Lanes;
    for (unsigned I = 0; I != NumParts; ++I)
      Lanes.push_back(DAG.getNode(ISD::BITCAST, DL, MVT::f32, Parts[I]));
    return DAG.getBuildVector(ValueVT, DL, Lanes);
  }
  if (!CC || !Subtarget.hasDoubleFPU() || Subtarget.useHardFloatABI() ||
      PartVT != MVT::i32)
    return SDValue();
  if (ValueVT == MVT::f32 && NumParts == 1)
    return DAG.getNode(SHISD::GPR_TO_FR, DL, MVT::f32, Parts[0]);
  if (ValueVT != MVT::f64 || NumParts != 2)
    return SDValue();
  SDValue MSW = DAG.getDataLayout().isBigEndian() ? Parts[0] : Parts[1];
  SDValue LSW = DAG.getDataLayout().isBigEndian() ? Parts[1] : Parts[0];
  return DAG.getNode(SHISD::GPR_PAIR_TO_DR, DL, MVT::f64, MSW, LSW);
}

TargetLowering::ConstraintType
SHTargetLowering::getConstraintType(StringRef Constraint) const {
  if (Constraint == "r" || Constraint == "z" || Constraint == "l" ||
      Constraint == "t" || Constraint == "u" || Constraint == "x")
    return C_RegisterClass;
  if (Constraint == "I08" || Constraint == "K04" || Constraint == "K05" ||
      Constraint == "K08" || Constraint == "M" || Constraint == "N" ||
      Constraint == "Jmb" || Constraint == "Jmw" || Constraint == "Jhb" ||
      Constraint == "P27")
    return C_Immediate;
  if (Constraint == "m")
    return C_Memory;
  return TargetLowering::getConstraintType(Constraint);
}

std::pair<unsigned, const TargetRegisterClass *>
SHTargetLowering::getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                                               StringRef Code, MVT VT) const {
  if (Code == "r" && VT == MVT::i64)
    return {0U, &SH::GPRRegClass};
  if (VT.isInteger() && VT.getSizeInBits() <= 32) {
    if (Code == "r" || Code == "u")
      return {0U, &SH::GPRRegClass};
    if (Code == "z")
      return {SH::R0, &SH::GPRRegClass};
  }
  if (VT == MVT::i32) {
    if (Code == "l")
      return {SH::PR, &SH::PRRegRegClass};
    if (Code == "x")
      return {SH::MACL, &SH::MACRegRegClass};
    if (Code == "t")
      return {SH::TBit, &SH::TRegRegClass};
  }
  return TargetLowering::getRegForInlineAsmConstraint(TRI, Code, VT);
}

InlineAsm::ConstraintCode
SHTargetLowering::getInlineAsmMemConstraint(StringRef ConstraintCode) const {
  if (ConstraintCode == "V")
    return InlineAsm::ConstraintCode::v;
  return TargetLowering::getInlineAsmMemConstraint(ConstraintCode);
}

void SHTargetLowering::LowerAsmOperandForConstraint(SDValue Op,
                                                    StringRef Constraint,
                                                    std::vector<SDValue> &Ops,
                                                    SelectionDAG &DAG) const {
  if (getConstraintType(Constraint) != C_Immediate)
    return TargetLowering::LowerAsmOperandForConstraint(Op, Constraint, Ops,
                                                        DAG);

  const auto *Constant = dyn_cast<ConstantSDNode>(Op);
  if (!Constant)
    return;
  int64_t Value = Constant->getSExtValue();
  bool Accepted = false;
  if (Constraint == "I08")
    Accepted = isInt<8>(Value);
  else if (Constraint == "K04")
    Accepted = isUInt<4>(Value);
  else if (Constraint == "K05")
    Accepted = isUInt<5>(Value);
  else if (Constraint == "K08")
    Accepted = isUInt<8>(Value);
  else if (Constraint == "M")
    Accepted = Value == 1;
  else if (Constraint == "N")
    Accepted = Value == 0;
  else if (Constraint == "Jmb")
    Accepted = Value == 0xff;
  else if (Constraint == "Jmw")
    Accepted = Value == 0xffff;
  else if (Constraint == "Jhb")
    Accepted = static_cast<uint32_t>(Value) == 0x80000000U;
  else if (Constraint == "P27")
    Accepted = Value == 1 || Value == 2 || Value == 8 || Value == 16;
  if (Accepted)
    Ops.push_back(
        DAG.getSignedTargetConstant(Value, SDLoc(Op), Op.getValueType()));
}

static int64_t getSHAddressUpdate(const SDNode *Op) {
  if (Op->getOpcode() != ISD::ADD && Op->getOpcode() != ISD::SUB)
    return 0;
  const auto *Amount = dyn_cast<ConstantSDNode>(Op->getOperand(1));
  if (!Amount)
    return 0;
  int64_t Update = Amount->getSExtValue();
  return Op->getOpcode() == ISD::SUB ? -Update : Update;
}

static int64_t getSHAddressUpdateSize(EVT MemVT) {
  if (MemVT == MVT::i8)
    return 1;
  if (MemVT == MVT::i16)
    return 2;
  if (MemVT == MVT::i32)
    return 4;
  return 0;
}

bool SHTargetLowering::getPreIndexedAddressParts(SDNode *N, SDValue &Base,
                                                 SDValue &Offset,
                                                 ISD::MemIndexedMode &AM,
                                                 SelectionDAG &DAG) const {
  const auto *Store = dyn_cast<StoreSDNode>(N);
  if (!Store || Store->isAtomic())
    return false;
  int64_t Size = getSHAddressUpdateSize(Store->getMemoryVT());
  SDValue Address = Store->getBasePtr();
  if (Size == 0 || getSHAddressUpdate(Address.getNode()) != -Size)
    return false;
  Base = Address.getOperand(0);
  if (Base.getOpcode() == ISD::FrameIndex)
    return false;
  Offset = DAG.getSignedConstant(-Size, SDLoc(N), MVT::i32);
  AM = ISD::PRE_DEC;
  return true;
}

bool SHTargetLowering::getPostIndexedAddressParts(SDNode *N, SDNode *Op,
                                                  SDValue &Base,
                                                  SDValue &Offset,
                                                  ISD::MemIndexedMode &AM,
                                                  SelectionDAG &DAG) const {
  const auto *Load = dyn_cast<LoadSDNode>(N);
  if (!Load || Load->isAtomic())
    return false;
  EVT MemVT = Load->getMemoryVT();
  ISD::LoadExtType Ext = Load->getExtensionType();
  const MachineFunction &MF = DAG.getMachineFunction();
  const Function &F = MF.getFunction();
  bool AllowAnyExt = Ext == ISD::EXTLOAD && Subtarget.hasSH2() &&
                     Subtarget.getSchedModel().IssueWidth == 1 &&
                     !F.hasOptSize() && !F.hasMinSize() && !F.hasOptNone() &&
                     MF.getTarget().getOptLevel() != CodeGenOptLevel::None;
  if ((MemVT == MVT::i32 && Ext != ISD::NON_EXTLOAD) ||
      (MemVT != MVT::i32 && Ext != ISD::SEXTLOAD && Ext != ISD::ZEXTLOAD &&
       !AllowAnyExt))
    return false;
  int64_t Size = getSHAddressUpdateSize(MemVT);
  if (Size == 0 || getSHAddressUpdate(Op) != Size)
    return false;
  Base = Op->getOperand(0);
  if (Base.getOpcode() == ISD::FrameIndex || Load->getBasePtr() != Base)
    return false;
  Offset = DAG.getSignedConstant(Size, SDLoc(N), MVT::i32);
  AM = ISD::POST_INC;
  return true;
}

static bool isKnownUnsignedWord(SDValue Value, SelectionDAG &DAG) {
  return DAG.computeKnownBits(Value).countMaxActiveBits() <= 16;
}

static bool isKnownPositiveWord(SDValue Value, SelectionDAG &DAG) {
  if (!isKnownUnsignedWord(Value, DAG))
    return false;
  if (DAG.isKnownNeverZero(Value))
    return true;
  // Moving freeze through an add drops its no-wrap flags. Prove that the
  // addition cannot wrap from the operands rather than relying on those flags.
  return Value.getOpcode() == ISD::ADD &&
         DAG.willNotOverflowAdd(false, Value.getOperand(0),
                                Value.getOperand(1)) &&
         (DAG.isKnownNeverZero(Value.getOperand(0)) ||
          DAG.isKnownNeverZero(Value.getOperand(1)));
}

static bool isKnownSignedWord(SDValue Value, SelectionDAG &DAG) {
  return DAG.ComputeNumSignBits(Value) >= 17;
}

static bool isAnyExtendedWord(SDValue Value) {
  if (Value.getOpcode() == ISD::ANY_EXTEND)
    return Value.getOperand(0).getValueSizeInBits() <= 16;
  const auto *Load = dyn_cast<LoadSDNode>(Value);
  return Load && Load->getExtensionType() == ISD::EXTLOAD &&
         Load->getMemoryVT().getSizeInBits() <= 16;
}

static SDValue stripWordExtension(SDValue Value, bool IsSigned,
                                  SelectionDAG &DAG) {
  while (true) {
    if (Value.getOpcode() == (IsSigned ? ISD::SIGN_EXTEND : ISD::ZERO_EXTEND) &&
        Value.getOperand(0).getValueSizeInBits() == 16) {
      SDValue Narrow = Value.getOperand(0);
      if (Narrow.getOpcode() == ISD::TRUNCATE &&
          Narrow.getOperand(0).getValueType() == MVT::i32)
        return Narrow.getOperand(0);
      return DAG.getNode(ISD::ANY_EXTEND, SDLoc(Value), MVT::i32, Narrow);
    }
    if (Value.getOpcode() == (IsSigned ? ISD::AssertSext : ISD::AssertZext) &&
        cast<VTSDNode>(Value.getOperand(1))->getVT().getSizeInBits() <= 16) {
      Value = Value.getOperand(0);
      continue;
    }
    if (IsSigned && Value.getOpcode() == ISD::SIGN_EXTEND_INREG &&
        cast<VTSDNode>(Value.getOperand(1))->getVT() == MVT::i16) {
      Value = Value.getOperand(0);
      continue;
    }
    if (!IsSigned && Value.getOpcode() == ISD::AND) {
      bool Stripped = false;
      for (unsigned Index : {0, 1}) {
        const auto *Mask = dyn_cast<ConstantSDNode>(Value.getOperand(Index));
        if (Mask && Mask->getZExtValue() == 0xffff) {
          Value = Value.getOperand(Index ^ 1);
          Stripped = true;
          break;
        }
      }
      if (Stripped)
        continue;
    }
    return Value;
  }
}

static SDValue combineConstantMultiply(SDNode *N, SelectionDAG &DAG,
                                       const SHSubtarget &STI) {
  const Function &F = DAG.getMachineFunction().getFunction();
  const MCSchedModel &Model = STI.getSchedModel();
  if (!STI.hasSH4Cache() || Model.IssueWidth < 2 || !STI.hasDynamicShift() ||
      DAG.getMachineFunction().getTarget().getOptLevel() ==
          CodeGenOptLevel::None ||
      F.hasOptSize() || F.hasMinSize() || F.hasOptNone())
    return SDValue();
  SDValue Value = N->getOperand(0);
  const auto *Constant = dyn_cast<ConstantSDNode>(N->getOperand(1));
  if (!Constant)
    return SDValue();
  APInt C = Constant->getAPIntValue();
  bool Negative = C.isNegative();
  C = C.abs();
  // Keep the expansion to one shifted copy and an add or subtract. More
  // general addition chains can require substantially longer live ranges.
  bool Subtract = (C + 1).isPowerOf2();
  if (!(C - 1).isPowerOf2() && !Subtract)
    return SDValue();
  unsigned Amount = (Subtract ? C + 1 : C - 1).logBase2();
  if (!Amount || Amount >= 32)
    return SDValue();
  SmallVector<unsigned, 16> Shifts = SH::planConstantShift(SH::SHLri, Amount);
  bool Dynamic = Shifts.size() > 2;
  const SHInstrInfo &TII = *STI.getInstrInfo();
  auto Latency = [&](unsigned Opcode) {
    return Model.computeInstrLatency(STI, TII.get(Opcode).getSchedClass());
  };
  int MulLatency = Latency(SH::MUL_L);
  int TransferLatency = Latency(SH::STS_MACL);
  if (MulLatency <= 0 || TransferLatency <= 0)
    return SDValue();
  unsigned ShiftLatency = 0;
  if (Dynamic)
    ShiftLatency = std::max(1, Latency(SH::SHLD));
  else
    for (unsigned Opcode : Shifts)
      ShiftLatency += std::max(1, Latency(Opcode));
  bool Negate = Negative && !Subtract;
  unsigned Instructions = 2 + (Dynamic ? 2 : Shifts.size()) + Negate;
  unsigned PathLatency =
      1 + ShiftLatency + std::max(1, Latency(Subtract ? SH::SUBrr : SH::ADDrr));
  if (Negate)
    PathLatency += std::max(1, Latency(SH::NEG));
  // Charge both serial instruction demand and result latency. Conservatively
  // treat constant materialization as free for MUL, and charge an extra GPR
  // for a dynamic shift count. No issue pairing is assumed for the expansion.
  unsigned MultiplyCost = 2 + MulLatency + TransferLatency;
  if (Instructions + PathLatency + Dynamic >= MultiplyCost)
    return SDValue();
  SDLoc DL(N);
  SDValue Shifted = DAG.getNode(ISD::SHL, DL, MVT::i32, Value,
                                DAG.getConstant(Amount, DL, MVT::i32));
  if (Subtract)
    return DAG.getNode(ISD::SUB, DL, MVT::i32, Negative ? Value : Shifted,
                       Negative ? Shifted : Value);
  SDValue Result = DAG.getNode(ISD::ADD, DL, MVT::i32, Shifted, Value);
  return Negate ? DAG.getNode(ISD::SUB, DL, MVT::i32,
                              DAG.getConstant(0, DL, MVT::i32), Result)
                : Result;
}

static SDValue combineNarrowMultiply(SDNode *N, SelectionDAG &DAG) {
  SDValue LHS = N->getOperand(0);
  SDValue RHS = N->getOperand(1);
  bool LHSAny = isAnyExtendedWord(LHS);
  bool RHSAny = isAnyExtendedWord(RHS);
  bool IsSigned;
  unsigned Opcode;
  if ((isKnownUnsignedWord(LHS, DAG) || LHSAny) &&
      (isKnownUnsignedWord(RHS, DAG) || RHSAny)) {
    IsSigned = false;
    Opcode = SHISD::MULU_WORD;
  } else if ((isKnownSignedWord(LHS, DAG) || LHSAny) &&
             (isKnownSignedWord(RHS, DAG) || RHSAny)) {
    IsSigned = true;
    Opcode = SHISD::MULS_WORD;
  } else {
    return SDValue();
  }
  return DAG.getNode(Opcode, SDLoc(N), MVT::i32,
                     stripWordExtension(LHS, IsSigned, DAG),
                     stripWordExtension(RHS, IsSigned, DAG));
}

static SDValue lowerSHBooleanSignExtend(SDValue Value, SDLoc DL,
                                        SelectionDAG &DAG) {
  if (!DAG.MaskedValueIsZero(Value, APInt::getHighBitsSet(32, 31)))
    Value = DAG.getNode(ISD::AND, DL, MVT::i32, Value,
                        DAG.getConstant(1, DL, MVT::i32));
  return DAG.getNode(SHISD::BOOL_SEXT, DL, MVT::i32, Value);
}

static SDValue combineSignBitAdd(SDNode *N, SelectionDAG &DAG) {
  if (N->getValueType(0) != MVT::i32)
    return SDValue();
  for (unsigned BitIndex : {0U, 1U}) {
    SDValue Bit = N->getOperand(BitIndex);
    if (Bit.getOpcode() != ISD::SRL || !Bit.hasOneUse())
      continue;
    const auto *Shift = dyn_cast<ConstantSDNode>(Bit.getOperand(1));
    if (!Shift || Shift->getZExtValue() != 31)
      continue;
    SDLoc DL(N);
    SDValue Compare =
        DAG.getNode(SHISD::CMP_PZ, DL, MVT::Glue, Bit.getOperand(0));
    SDValue Subtract =
        DAG.getNode(ISD::SUBE, DL, DAG.getVTList(MVT::i32, MVT::Glue),
                    N->getOperand(BitIndex ^ 1U),
                    DAG.getSignedConstant(-1, DL, MVT::i32), Compare);
    return Subtract;
  }
  return SDValue();
}

static SDValue combineBooleanAdd(SDNode *N, SelectionDAG &DAG) {
  if (N->getValueType(0) != MVT::i32)
    return SDValue();
  for (unsigned BooleanIndex : {0U, 1U}) {
    SDValue Boolean = N->getOperand(BooleanIndex);
    bool Inverted = false;
    if (Boolean.getOpcode() == ISD::XOR && Boolean.hasOneUse()) {
      for (unsigned ConstantIndex : {0U, 1U}) {
        const auto *Constant =
            dyn_cast<ConstantSDNode>(Boolean.getOperand(ConstantIndex));
        if (!Constant || !Constant->isOne())
          continue;
        Boolean = Boolean.getOperand(ConstantIndex ^ 1U);
        Inverted = true;
        break;
      }
    }
    if (Boolean.getOpcode() != SHISD::MOVT || !Boolean.hasOneUse())
      continue;
    SDLoc DL(N);
    SDValue Carry = Boolean.getOperand(0);
    SDValue Accumulator = N->getOperand(BooleanIndex ^ 1U);
    if (Carry.getNode() == Accumulator.getNode() ||
        Carry.getNode()->isPredecessorOf(Accumulator.getNode()))
      continue;
    SDValue Constant = DAG.getSignedConstant(Inverted ? -1 : 0, DL, MVT::i32);
    return DAG.getNode(Inverted ? ISD::SUBE : ISD::ADDE, DL,
                       DAG.getVTList(MVT::i32, MVT::Glue), Accumulator,
                       Constant, Carry);
  }
  return SDValue();
}

static bool shouldKeepGlobalAddressBase(const GlobalAddressSDNode *GA) {
  const auto *Variable = dyn_cast<GlobalVariable>(GA->getGlobal());
  if (!Variable)
    return false;
  const DataLayout &DL = Variable->getParent()->getDataLayout();
  SmallVector<int64_t, 8> Offsets;
  for (const User *U : Variable->users()) {
    const auto *GEP = dyn_cast<GEPOperator>(U);
    if (!GEP) {
      Offsets.push_back(0);
      continue;
    }
    APInt Offset(DL.getIndexTypeSizeInBits(GEP->getType()), 0);
    if (GEP->accumulateConstantOffset(DL, Offset) && Offset.isSignedIntN(64))
      Offsets.push_back(Offset.getSExtValue());
  }
  llvm::sort(Offsets);
  Offsets.erase(std::unique(Offsets.begin(), Offsets.end()), Offsets.end());
  // Keeping one nearby base is smaller than loading three or more absolute
  // addresses.  Bound the span so large objects do not trade address literals
  // for equally expensive full-width offset materializations.
  if (Offsets.size() < 3)
    return false;
  int64_t Span;
  if (SubOverflow(Offsets.back(), Offsets.front(), Span))
    return false;
  return isInt<16>(Span);
}

bool SHTargetLowering::isOffsetFoldingLegal(
    const GlobalAddressSDNode *GA) const {
  return !shouldKeepGlobalAddressBase(GA) &&
         TargetLowering::isOffsetFoldingLegal(GA);
}

static SDValue combineSmallCountShift(SDNode *N, SelectionDAG &DAG,
                                      const SHSubtarget &STI) {
  const MachineFunction &MF = DAG.getMachineFunction();
  const Function &F = MF.getFunction();
  if (N->getValueType(0) != MVT::i32 ||
      N->getOperand(1).getValueType() != MVT::i32 ||
      isa<ConstantSDNode>(N->getOperand(1)) ||
      MF.getTarget().getOptLevel() == CodeGenOptLevel::None || F.hasOptNone() ||
      F.hasOptSize() || F.hasMinSize() || STI.getCPU() != "sh2" ||
      STI.getTuneCPU() != "sh2" || !STI.hasSH2() || STI.hasDynamicShift() ||
      STI.hasSH2A() || STI.hasSingleFPU() || STI.hasDSP() ||
      STI.getABI() != SH::ABIKind::GNU ||
      MF.getTarget().isPositionIndependent() ||
      F.getCallingConv() != CallingConv::C)
    return SDValue();
  for (StringRef Name : {"sh-interrupt-handler", "sh-nosave-low-regs",
                         "sh-resbank", "sh-sp-switch", "sh-trap-exit"})
    if (F.hasFnAttribute(Name))
      return SDValue();

  SDValue Source = N->getOperand(0);
  SDValue Count = N->getOperand(1);
  KnownBits Known = DAG.computeKnownBits(Count);
  APInt Unknown = ~(Known.Zero | Known.One);
  if (Known.hasConflict() || Known.isConstant() ||
      Known.getMaxValue().uge(32) || Unknown.popcount() > 2 ||
      !DAG.isGuaranteedNotToBeUndefOrPoison(Count))
    return SDValue();
  unsigned Base = Known.One.getZExtValue();
  unsigned Mask = Unknown.getZExtValue();
  // Enumerate the independent unknown bits, including combinations that an
  // unavailable relational proof might have excluded. Never mask a count.
  SmallVector<unsigned, 4> Counts;
  for (unsigned Subset = Mask;; Subset = (Subset - 1) & Mask) {
    Counts.push_back(Base | Subset);
    if (!Subset)
      break;
  }
  if (Counts.size() < 2 || Counts.size() > 4 ||
      llvm::any_of(Counts, [](unsigned C) { return C >= 32; }))
    return SDValue();

  unsigned Shift = N->getOpcode() == ISD::SHL ? SH::SHLri : SH::SRLri;
  unsigned Fixed = SH::planConstantShift(Shift, Base).size();
  unsigned Words = Fixed + 1;
  // Quarter-cycle estimate: fixed primitives, a result copy and a lifetime
  // allowance. Additional uses retain source/count lifetimes across stages.
  unsigned Cost = 4 * (Fixed + 2 + !Source.hasOneUse() + !Count.hasOneUse());
  for (unsigned Bit = 1; Bit != 32; Bit <<= 1) {
    if (!(Mask & Bit))
      continue;
    unsigned Steps = SH::planConstantShift(Shift, Bit).size();
    // A GPR mask, TST, and BT (1/3 cycles), then a half-probability
    // fixed shift. No R0 constraint or assumed useful delay slot.
    Cost += 16 + 2 * Steps;
    Words += 3 + Steps;
  }
  // Charge code growth above a seven-word call/setup/result/literal form.
  Cost += Words > 7 ? Words - 7 : 0;
  // Public helper estimate: eight dispatch/body/return instructions, one
  // caller transfer and three argument/result copies. Context may coalesce
  // copies or add address, frame and helper-path costs; this is not a bound
  // on every call site. Prefer H on ties and leave the planner unchanged.
  if (Cost >= 4 * 12)
    return SDValue();
  SDLoc DL(N);
  return DAG.getNode(N->getOpcode() == ISD::SHL ? SHISD::SHL_SMALL
                                                : SHISD::SRL_SMALL,
                     DL, MVT::i32,
                     {Source, Count, DAG.getTargetConstant(Base, DL, MVT::i32),
                      DAG.getTargetConstant(Mask, DL, MVT::i32)});
}

SDValue SHTargetLowering::PerformDAGCombine(SDNode *N,
                                            DAGCombinerInfo &DCI) const {
  // Assemble a byte-reversed unaligned word directly instead of constructing
  // its native-endian value and then expanding a separate byte swap. Restrict
  // this to one value use so other users never require a second memory read.
  if (N->getOpcode() == ISD::BSWAP && DCI.isBeforeLegalize() &&
      N->getValueType(0) == MVT::i32 &&
      OriginalOptLevel != CodeGenOptLevel::None &&
      !DCI.DAG.getMachineFunction().getFunction().hasOptNone()) {
    SDValue Input = N->getOperand(0);
    auto *Load = dyn_cast<LoadSDNode>(Input);
    if (Load && Load->isSimple() && Load->isUnindexed() &&
        Load->getExtensionType() == ISD::NON_EXTLOAD &&
        Load->getMemoryVT() == MVT::i32 && Load->getAlign() == Align(1) &&
        Input.hasOneUse()) {
      SelectionDAG &DAG = DCI.DAG;
      SDLoc DL(N);
      SDValue Result = DAG.getConstant(0, DL, MVT::i32);
      SmallVector<SDValue, 4> Chains;
      for (unsigned Byte = 0; Byte != 4; ++Byte) {
        unsigned Shift =
            8 * (DAG.getDataLayout().isLittleEndian() ? 3 - Byte : Byte);
        SDValue Address = DAG.getObjectPtrOffset(DL, Load->getBasePtr(),
                                                 TypeSize::getFixed(Byte));
        SDValue Piece = DAG.getExtLoad(
            Shift == 24 ? ISD::SEXTLOAD : ISD::ZEXTLOAD, DL, MVT::i32,
            Load->getChain(), Address,
            Load->getPointerInfo().getWithOffset(Byte), MVT::i8, Align(1),
            Load->getMemOperand()->getFlags(), Load->getAAInfo());
        Chains.push_back(Piece.getValue(1));
        if (Shift)
          Piece = DAG.getNode(ISD::SHL, DL, MVT::i32, Piece,
                              DAG.getConstant(Shift, DL, MVT::i32));
        Result = DAG.getNode(ISD::OR, DL, MVT::i32, Result, Piece);
      }
      DAG.ReplaceAllUsesOfValueWith(
          Input.getValue(1),
          DAG.getNode(ISD::TokenFactor, DL, MVT::Other, Chains));
      return Result;
    }
  }

  // Native unaligned expansion creates zero-extending byte loads late. The
  // byte shifted into bits 31:24 needs no clearing of its high sign bits.
  // Keep other users and memory contracts unchanged, and retain O0/optnone.
  if (N->getOpcode() == ISD::SHL && DCI.isAfterLegalizeDAG() &&
      N->getValueType(0) == MVT::i32 &&
      OriginalOptLevel != CodeGenOptLevel::None &&
      !DCI.DAG.getMachineFunction().getFunction().hasOptNone()) {
    SDValue Input = N->getOperand(0);
    const auto *Shift = dyn_cast<ConstantSDNode>(N->getOperand(1));
    auto *Load = dyn_cast<LoadSDNode>(Input);
    if (Shift && Shift->getZExtValue() == 24 && Load &&
        Load->getExtensionType() == ISD::ZEXTLOAD &&
        Load->getMemoryVT() == MVT::i8 && Load->isSimple() &&
        Load->isUnindexed() && Input.hasOneUse()) {
      SDValue Signed = DCI.DAG.getExtLoad(
          ISD::SEXTLOAD, SDLoc(Load), MVT::i32, Load->getChain(),
          Load->getBasePtr(), MVT::i8, Load->getMemOperand());
      DCI.DAG.ReplaceAllUsesOfValueWith(Input.getValue(1), Signed.getValue(1));
      // No-wrap flags do not describe the sign-extended intermediate value.
      return DCI.DAG.getNode(ISD::SHL, SDLoc(N), MVT::i32, Signed,
                             N->getOperand(1));
    }
  }

  // Run before type/operation legalization so shifts synthesized by rotate
  // and i64 lowering retain their existing selection policies.
  if (N->getOpcode() == ISD::SHL || N->getOpcode() == ISD::SRL) {
    // The action table captures the original codegen level. SH's selector
    // has its own default level, and visiting optnone functions can change
    // the shared TargetMachine level. Only replace the original helper path.
    bool HelperPath = getOperationAction(N->getOpcode(), MVT::i32) == Custom;
    return DCI.isBeforeLegalize() && HelperPath
               ? combineSmallCountShift(N, DCI.DAG, Subtarget)
               : SDValue();
  }
  if (N->getOpcode() == ISD::FREEZE && N->getOperand(0).isUndef()) {
    EVT VT = N->getValueType(0);
    // A whole literal undef or poison may choose zero once. Do this before
    // promotion/splitting or selection turns freeze into an undef COPY;
    // such a COPY does not preserve one chosen value across its uses.
    if (VT.isInteger() && !VT.isVector())
      return DCI.DAG.getConstant(0, SDLoc(N), VT);
    // Select one value before a floating freeze becomes an undef COPY.
    // Otherwise its uses can read unrelated registers across a call. This
    // also runs at O0, before software-FP legalization splits the value.
    if (VT == MVT::f32 || VT == MVT::f64)
      return DCI.DAG.getConstantFP(0.0, SDLoc(N), VT);
  }
  if (N->getOpcode() == ISD::SIGN_EXTEND_INREG &&
      N->getValueType(0) == MVT::i32 &&
      cast<VTSDNode>(N->getOperand(1))->getVT() == MVT::i1)
    return lowerSHBooleanSignExtend(N->getOperand(0), SDLoc(N), DCI.DAG);
  if (N->getOpcode() == ISD::MUL && N->getValueType(0) == MVT::i32) {
    if (SDValue Combined = combineConstantMultiply(N, DCI.DAG, Subtarget))
      return Combined;
    return combineNarrowMultiply(N, DCI.DAG);
  }
  if (N->getOpcode() != ISD::ADD)
    return SDValue();
  if (SDValue Combined = combineSignBitAdd(N, DCI.DAG))
    return Combined;
  if (SDValue Combined = combineBooleanAdd(N, DCI.DAG))
    return Combined;

  SDValue Address = N->getOperand(0);
  SDValue Addend = N->getOperand(1);
  if (!isa<GlobalAddressSDNode>(Address) && isa<GlobalAddressSDNode>(Addend))
    std::swap(Address, Addend);
  const auto *Global = dyn_cast<GlobalAddressSDNode>(Address);
  const auto *Constant = dyn_cast<ConstantSDNode>(Addend);
  if (!Global || !Constant ||
      (Address.getOpcode() != ISD::GlobalAddress &&
       Address.getOpcode() != ISD::GlobalTLSAddress))
    return SDValue();
  if (Address.getOpcode() == ISD::GlobalAddress &&
      shouldKeepGlobalAddressBase(Global))
    return SDValue();

  auto [Offset, Overflow] =
      AddOverflow<int64_t>(Global->getOffset(), Constant->getSExtValue());
  if (Overflow || !isInt<32>(Offset))
    return SDValue();

  SelectionDAG &DAG = DCI.DAG;
  if (Address.getOpcode() == ISD::GlobalTLSAddress)
    return DAG.getGlobalAddress(Global->getGlobal(), SDLoc(N),
                                N->getValueType(0), Offset);
  return DAG.getGlobalAddress(Global->getGlobal(), SDLoc(N), N->getValueType(0),
                              Offset);
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
constexpr MCRegister SHLittleEndianF32ArgumentRegisters[] = {
    SH::FR5, SH::FR4, SH::FR7, SH::FR6, SH::FR9, SH::FR8, SH::FR11, SH::FR10};
constexpr MCRegister SHBigEndianF32ArgumentRegisters[] = {
    SH::FR4, SH::FR5, SH::FR6, SH::FR7, SH::FR8, SH::FR9, SH::FR10, SH::FR11};
constexpr MCRegister SHF64ArgumentRegisters[] = {SH::DR4, SH::DR6, SH::DR8,
                                                 SH::DR10};
constexpr MCRegister SHNaturalF32ArgumentRegisters[] = {
    SH::FR4, SH::FR5, SH::FR6, SH::FR7, SH::FR8, SH::FR9, SH::FR10, SH::FR11};

static MVT getSHHardFloatComplexElementType(Type *Ty) {
  const auto *StructTy = dyn_cast<StructType>(Ty);
  if (!StructTy || StructTy->isPacked() || StructTy->getNumElements() != 2 ||
      StructTy->getElementType(0) != StructTy->getElementType(1))
    return MVT::Other;
  Type *ElementTy = StructTy->getElementType(0);
  return ElementTy->isFloatTy()    ? MVT::f32
         : ElementTy->isDoubleTy() ? MVT::f64
                                   : MVT::Other;
}

static SmallVector<MVT, 2> getSHFPUABIValueTypes(Type *Ty, bool AllowDouble) {
  if (Ty->isFloatTy())
    return {MVT::f32};
  if (Ty->isDoubleTy() && AllowDouble)
    return {MVT::f64};
  MVT ElementVT = getSHHardFloatComplexElementType(Ty);
  if (ElementVT == MVT::f64 && !AllowDouble)
    return {};
  return ElementVT == MVT::Other ? SmallVector<MVT, 2>()
                                 : SmallVector<MVT, 2>({ElementVT, ElementVT});
}

static bool isSHFPUABIValueType(const SHSubtarget &Subtarget, MVT VT) {
  return (VT == MVT::f32 && Subtarget.useHardFloatABI()) ||
         (VT == MVT::f64 && Subtarget.usesSH4HardFloatABI());
}

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
  MVT ValueVT = MVT::Other;

  bool isFloatingPoint() const {
    return ValueVT == MVT::f32 || ValueVT == MVT::f64;
  }
};

class SHABIState {
  unsigned NextRegister = 0;
  unsigned NextFPURegister = 0;
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

  SHABIValue allocateWhole(uint64_t Size, bool ExhaustRegisters = false) {
    if (Size == 0 || Size > UINT_MAX)
      return allocate(Size);
    unsigned WordCount = divideCeil(static_cast<unsigned>(Size), 4u);
    unsigned RemainingRegisters = std::size(SHArgumentRegisters) - NextRegister;
    if (WordCount <= RemainingRegisters)
      return allocate(Size);
    unsigned SavedNextRegister = NextRegister;
    NextRegister = std::size(SHArgumentRegisters);
    SHABIValue Value = allocate(Size);
    if (!ExhaustRegisters)
      NextRegister = SavedNextRegister;
    return Value;
  }

  SHABIValue allocateFloat(MVT VT, bool IsLittleEndian,
                           bool UseNaturalF32Order) {
    if (VT != MVT::f32 && VT != MVT::f64)
      report_fatal_error("SH hard-float ABI value has an invalid type");
    unsigned RegisterCount = VT == MVT::f32 ? 1 : 2;
    unsigned SavedNextFPURegister = NextFPURegister;
    if (VT == MVT::f64)
      NextFPURegister = alignTo(NextFPURegister, 2u);
    SHABIValue Value{RegisterCount * 4u, {}, VT};
    if (NextFPURegister + RegisterCount <= 8) {
      MCRegister Reg;
      if (VT == MVT::f32)
        Reg = (UseNaturalF32Order ? SHNaturalF32ArgumentRegisters
               : IsLittleEndian
                   ? SHLittleEndianF32ArgumentRegisters
                   : SHBigEndianF32ArgumentRegisters)[NextFPURegister];
      else
        Reg = SHF64ArgumentRegisters[NextFPURegister / 2];
      Value.Words.push_back({0, RegisterCount * 4u, 0, Reg, -1});
    } else {
      Value.Words.push_back({0, RegisterCount * 4u, 0, 0, StackSize});
      StackSize += RegisterCount * 4u;
      NextFPURegister = SavedNextFPURegister;
      return Value;
    }
    NextFPURegister += RegisterCount;
    return Value;
  }

  SHABIValue allocateComplex(MVT ElementVT, bool IsLittleEndian,
                             bool UseNaturalF32Order) {
    if (ElementVT != MVT::f32 && ElementVT != MVT::f64)
      report_fatal_error("SH hard-float complex value has an invalid type");
    unsigned RegisterCount = ElementVT == MVT::f32 ? 2 : 4;
    unsigned SavedNextFPURegister = NextFPURegister;
    if (ElementVT == MVT::f64)
      NextFPURegister = alignTo(NextFPURegister, 2u);
    SHABIValue Value{RegisterCount * 4u, {}, ElementVT};
    if (NextFPURegister + RegisterCount <= 8) {
      if (ElementVT == MVT::f32) {
        unsigned First = NextFPURegister;
        unsigned Real = UseNaturalF32Order || !IsLittleEndian || First % 2 == 0
                            ? First
                            : First - 1;
        unsigned Imaginary =
            UseNaturalF32Order || !IsLittleEndian || First % 2 == 0 ? First + 1
                                                                    : First + 2;
        Value.Words.push_back(
            {0, 4, 0, SHNaturalF32ArgumentRegisters[Real], -1});
        Value.Words.push_back(
            {4, 4, 1, SHNaturalF32ArgumentRegisters[Imaginary], -1});
      } else {
        Value.Words.push_back(
            {0, 8, 0, SHF64ArgumentRegisters[NextFPURegister / 2], -1});
        Value.Words.push_back(
            {8, 8, 1, SHF64ArgumentRegisters[NextFPURegister / 2 + 1], -1});
      }
    } else {
      unsigned ElementSize = ElementVT == MVT::f32 ? 4 : 8;
      Value.Words.push_back({0, ElementSize, 0, 0, StackSize});
      Value.Words.push_back(
          {ElementSize, ElementSize, 1, 0, StackSize + ElementSize});
      StackSize += RegisterCount * 4u;
      NextFPURegister = SavedNextFPURegister;
      return Value;
    }
    NextFPURegister += RegisterCount;
    return Value;
  }

  unsigned getStackSize() const { return StackSize; }
  unsigned getRegisterCursor() const { return NextRegister; }
  unsigned getFPURegisterCursor() const { return NextFPURegister; }
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
    if (!ValueVT.isInteger() && ValueVT != MVT::f32 && ValueVT != MVT::f64 &&
        ValueVT != MVT::v2f32 && ValueVT != MVT::v4f32)
      report_fatal_error(
          "SH aggregate values may contain only integers, pointers, f32, and "
          "f64");
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

static SDValue getSHAlignedStackAddress(SelectionDAG &DAG, const SDLoc &DL,
                                        SDValue Base, Align Alignment) {
  if (Alignment <= Align(4))
    return Base;
  SDValue Biased =
      DAG.getNode(ISD::ADD, DL, MVT::i32, Base,
                  DAG.getIntPtrConstant(Alignment.value() - 1, DL));
  return DAG.getNode(
      ISD::AND, DL, MVT::i32, Biased,
      DAG.getSignedConstant(-static_cast<int64_t>(Alignment.value()), DL,
                            MVT::i32));
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
  if ((Ty->isIntegerTy() && Ty->getIntegerBitWidth() <= 32) ||
      Ty->isFloatTy() ||
      (Ty->isPointerTy() && Ty->getPointerAddressSpace() == 0))
    return 4;
  if ((Ty->isIntegerTy() && Ty->getIntegerBitWidth() <= 64) || Ty->isDoubleTy())
    return 8;
  if (isSupportedSHAggregateType(Ty) ||
      (Ty->isVectorTy() && isSupportedSHValueType(Ty)))
    return getFixedSHTypeAllocSize(DL, Ty);
  report_fatal_error("SH ABI value type is not supported");
}

} // namespace

const TargetRegisterClass *SHTargetLowering::getRepRegClassFor(MVT VT) const {
  // FSCA's even pair contains two singles, including on single-only targets
  // where f64 is deliberately illegal. Keep its machine tuple untyped.
  return VT == MVT::Untyped ? &SH::DRRegClass
                           : TargetLowering::getRepRegClassFor(VT);
}

uint8_t SHTargetLowering::getRepRegClassCostFor(MVT VT) const {
  return VT == MVT::Untyped ? 1 : TargetLowering::getRepRegClassCostFor(VT);
}

bool SHTargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context,
    const Type *RetTy) const {
  (void)Context;
  if (!isSupportedSHCallingConvention(CallConv) ||
      (CallConv == CallingConv::Fast && IsVarArg))
    return false;
  if (RetTy->isIntegerTy(8) || RetTy->isIntegerTy(16))
    return Outs.size() == 1 && Outs[0].VT == MVT::i32 &&
           !Outs[0].Flags.isByVal() && !Outs[0].Flags.isSRet();
  if (RetTy->isIntegerTy(1))
    return Outs.size() == 1 && Outs[0].VT == MVT::i32 &&
           !Outs[0].Flags.isByVal() && !Outs[0].Flags.isSRet();
  if (RetTy->isVoidTy())
    return Outs.empty();
  SmallVector<MVT, 2> HardFloatVTs =
      Subtarget.useHardFloatABI()
          ? getSHFPUABIValueTypes(const_cast<Type *>(RetTy),
                                  Subtarget.usesSH4HardFloatABI())
          : SmallVector<MVT, 2>();
  if (!HardFloatVTs.empty()) {
    if (Outs.size() != HardFloatVTs.size())
      return false;
    for (unsigned I = 0; I != Outs.size(); ++I)
      if (Outs[I].VT != HardFloatVTs[I] || Outs[I].Flags.isByVal() ||
          Outs[I].Flags.isSRet())
        return false;
    return true;
  }
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
  requireSupportedSHCallingConvention(CallConv);
  if (CallConv == CallingConv::Fast && IsVarArg)
    report_fatal_error(
        "SH fast calling convention does not support variadic functions");
  MachineFunction &MF = DAG.getMachineFunction();
  const Function &F = MF.getFunction();
  const DataLayout &DataLayout = DAG.getDataLayout();

  MachineRegisterInfo &MRI = MF.getRegInfo();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  SHMachineFunctionInfo &FuncInfo = *MF.getInfo<SHMachineFunctionInfo>();
  SHABIState ABIState;
  SmallVector<SDValue, 8> ABIValues(Ins.size());
  SmallVector<SDValue, 8> ArgChains;
  bool HasNestArgument = false;

  auto copyFromRegister = [&](MCRegister Reg, MVT VT = MVT::i32) {
    const TargetRegisterClass *RC = VT == MVT::f32   ? &SH::FRRegClass
                                    : VT == MVT::f64 ? &SH::DRRegClass
                                                     : &SH::GPRRegClass;
    Register VReg = MRI.createVirtualRegister(RC);
    MRI.addLiveIn(Reg, VReg);
    SDValue Value = DAG.getCopyFromReg(Chain, DL, VReg, VT);
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
      if ((Ins[I].VT != MVT::i32 &&
           !isSHFPUABIValueType(Subtarget, Ins[I].VT)) ||
          Ins[I].Flags.isByRef() || Ins[I].Flags.isInAlloca() ||
          Ins[I].Flags.isPreallocated() || Ins[I].Flags.isSwiftSelf() ||
          Ins[I].Flags.isSwiftAsync() || Ins[I].Flags.isSwiftError())
        report_fatal_error("SH formal argument has unsupported ABI flags");

    if (Arg.hasNestAttr()) {
      if (HasNestArgument || ArgIns.size() != 1 ||
          Ins[ArgIns[0]].VT != MVT::i32 || !Ins[ArgIns[0]].Flags.isNest() ||
          !Arg.getType()->isPointerTy())
        report_fatal_error("SH nest argument must be one 32-bit pointer");
      HasNestArgument = true;
      ABIValues[ArgIns[0]] = copyFromRegister(SH::R3);
      continue;
    }

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
    MVT ComplexElementVT = getSHHardFloatComplexElementType(ABIType);
    if (ComplexElementVT == MVT::f64 && !Subtarget.usesSH4HardFloatABI())
      ComplexElementVT = MVT::Other;
    SHABIValue ABIValue =
        Subtarget.useHardFloatABI() &&
                (Arg.hasByValAttr() ||
                 (Subtarget.usesEFloatABI() &&
                  (ABIType->isDoubleTy() || (!ABIType->isFloatingPointTy() &&
                                             ComplexElementVT == MVT::Other))))
            ? ABIState.allocateWhole(Size, Subtarget.usesEFloatABI())
        : !Arg.hasByValAttr() &&
                ((ABIType->isFloatTy() && Subtarget.useHardFloatABI()) ||
                 (ABIType->isDoubleTy() && Subtarget.usesSH4HardFloatABI()))
            ? ABIState.allocateFloat(ABIType->isFloatTy() ? MVT::f32 : MVT::f64,
                                     DataLayout.isLittleEndian(),
                                     Subtarget.usesEFloatABI())
        : Subtarget.useHardFloatABI() && !Arg.hasByValAttr() &&
                ComplexElementVT != MVT::Other
            ? ABIState.allocateComplex(ComplexElementVT,
                                       DataLayout.isLittleEndian(),
                                       Subtarget.usesEFloatABI())
        : Subtarget.useHardFloatABI() ? ABIState.allocateWhole(Size)
                                      : ABIState.allocate(Size);
    if (!Arg.hasByValAttr() && ABIValue.Size == 4 &&
        (Arg.getType()->isIntegerTy(32) || Arg.getType()->isPointerTy()) &&
        ABIValue.Words.size() == 1 && !ABIValue.Words.front().isRegister() &&
        ABIValue.Words.front().ValidBytes == 4)
      FuncInfo.setIncomingStackArgumentOffset(
          Arg.getArgNo(), ABIValue.Words.front().StackOffset);

    if (Arg.hasByValAttr()) {
      if (ArgIns.size() != 1 || !Ins[ArgIns[0]].Flags.isByVal() ||
          Ins[ArgIns[0]].Flags.getByValSize() != Size)
        report_fatal_error("SH byval argument size is inconsistent");
      Align Alignment = Ins[ArgIns[0]].Flags.getNonZeroByValAlign();
      requireSupportedSHByValType(DataLayout, ABIType, Alignment);

      // Keep addressable named values that need copying below the target-owned
      // variadic and PR areas. Over-aligned byval values also need a private
      // copy because the physical incoming argument area is only word-aligned.
      if (F.isVarArg() || Alignment > Align(4)) {
        uint64_t LocalSize = Size;
        Align StackAlignment = Alignment;
        if (Alignment > Align(4)) {
          LocalSize += Alignment.value() - 1;
          StackAlignment = Align(4);
        }
        int FI = MFI.CreateStackObject(LocalSize, StackAlignment,
                                       /*isSpillSlot=*/false);
        MFI.setIsAliasedObjectIndex(FI, true);
        SDValue LocalBase = DAG.getFrameIndex(FI, MVT::i32);
        LocalBase = getSHAlignedStackAddress(DAG, DL, LocalBase, Alignment);
        if (Alignment > Align(4)) {
          Register AddressReg = MRI.createVirtualRegister(&SH::GPRRegClass);
          SDValue AddressCopy =
              DAG.getCopyToReg(Chain, DL, AddressReg, LocalBase);
          ArgChains.push_back(AddressCopy);
          LocalBase = DAG.getCopyFromReg(AddressCopy, DL, AddressReg, MVT::i32);
          ArgChains.push_back(LocalBase.getValue(1));
        }
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
          // This initializes the address returned for the IR byval argument.
          // Generic stack provenance is non-aliased and lets the scheduler
          // move argument loads ahead of their initialization. Use the
          // argument's provenance; rounding LocalBase also means its offset
          // within FI is not statically known.
          storeSHABIWord(DAG, DL, Chain, LocalBase, MachinePointerInfo(&Arg),
                         Alignment, Word.ByteOffset, Word.ValidBytes, Value,
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
    if (ABIValue.isFloatingPoint()) {
      if (ArgIns.size() != ABIValue.Words.size())
        report_fatal_error(
            "SH hard-float formal argument parts are inconsistent");
      for (unsigned Part = 0; Part != ArgIns.size(); ++Part) {
        if (Ins[ArgIns[Part]].VT != ABIValue.ValueVT)
          report_fatal_error(
              "SH hard-float formal argument type is inconsistent");
        const SHABIWord &Location = ABIValue.Words[Part];
        SDValue Value;
        if (Location.isRegister()) {
          Value = copyFromRegister(Location.Reg, ABIValue.ValueVT);
        } else {
          int FI = MFI.CreateFixedObject(
              Location.ValidBytes, Location.StackOffset, /*IsImmutable=*/true);
          MFI.setObjectAlignment(FI, Align(4));
          SDValue FrameIndex = DAG.getFrameIndex(FI, MVT::i32);
          Value =
              DAG.getLoad(ABIValue.ValueVT, DL, Chain, FrameIndex,
                          MachinePointerInfo::getFixedStack(MF, FI), Align(4));
          ArgChains.push_back(Value.getValue(1));
        }
        ABIValues[ArgIns[Part]] = Value;
      }
      continue;
    }
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

  if (ABIState.getStackSize() > SHMaxCallFrameSize)
    report_fatal_error(Twine("SH incoming argument area cannot exceed ") +
                       Twine(SHMaxCallFrameSize) + " bytes");
  if ((IsVarArg || F.isVarArg()) && Subtarget.useHardFloatABI()) {
    unsigned GPRCursor = ABIState.getRegisterCursor();
    unsigned FPRCursor = std::min(ABIState.getFPURegisterCursor(), 8u);
    int FPRFI = MFI.CreateStackObject(32, Align(8), /*isSpillSlot=*/false);
    int GPRFI = MFI.CreateStackObject(16, Align(4), /*isSpillSlot=*/false);
    int StackFI =
        MFI.CreateFixedObject(4, ABIState.getStackSize(), /*IsImmutable=*/true);
    SDValue FPRBase = DAG.getFrameIndex(FPRFI, MVT::i32);
    SDValue GPRBase = DAG.getFrameIndex(GPRFI, MVT::i32);
    SmallVector<SDValue, 12> SaveChains;
    ArrayRef<MCRegister> F32Regs =
        Subtarget.usesEFloatABI()
            ? ArrayRef<MCRegister>(SHNaturalF32ArgumentRegisters)
        : DataLayout.isLittleEndian()
            ? ArrayRef<MCRegister>(SHLittleEndianF32ArgumentRegisters)
            : ArrayRef<MCRegister>(SHBigEndianF32ArgumentRegisters);
    for (unsigned I = FPRCursor; I != 8; ++I) {
      SDValue Value = copyFromRegister(F32Regs[I], MVT::f32);
      SDValue Address = getSHMemoryAddress(DAG, DL, FPRBase, I * 4);
      SaveChains.push_back(DAG.getStore(Chain, DL, Value, Address,
                                        MachinePointerInfo::getStack(MF, I * 4),
                                        Align(4)));
    }
    for (unsigned I = GPRCursor; I != std::size(SHArgumentRegisters); ++I) {
      SDValue Value = copyFromRegister(SHArgumentRegisters[I]);
      storeSHABIWord(DAG, DL, Chain, GPRBase,
                     MachinePointerInfo::getStack(MF, 0), Align(4), I * 4, 4,
                     Value, SaveChains);
    }
    ArgChains.append(SaveChains);
    FuncInfo.setFPUVarArgsAreas(GPRFI, FPRFI, StackFI, GPRCursor, FPRCursor);
  } else if (IsVarArg || F.isVarArg()) {
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
  requireSupportedSHCallingConvention(CallConv);
  if (CallConv == CallingConv::Fast && IsVarArg)
    report_fatal_error(
        "SH fast calling convention does not support variadic functions");
  const Function &F = DAG.getMachineFunction().getFunction();
  if (Outs.size() != OutVals.size())
    report_fatal_error("SH return value part count is inconsistent");
  MachineFunction &MF = DAG.getMachineFunction();
  SHMachineFunctionInfo &FuncInfo = *MF.getInfo<SHMachineFunctionInfo>();
  bool HasSRet = FuncInfo.getSRetReturnReg().isValid();
  Type *ReturnTy = F.getReturnType();
  SmallVector<MVT, 2> HardFloatVTs =
      Subtarget.useHardFloatABI()
          ? getSHFPUABIValueTypes(ReturnTy, Subtarget.usesSH4HardFloatABI())
          : SmallVector<MVT, 2>();
  if (HardFloatVTs.empty() && ReturnTy->isAggregateType() &&
      !isDirectSHAggregateReturn(DAG.getDataLayout(), ReturnTy)) {
    if (!Outs.empty() || !HasSRet)
      report_fatal_error("SH indirect aggregate return is missing sret");
  } else if (ReturnTy->isVoidTy()) {
    if (!Outs.empty())
      report_fatal_error("SH void returns cannot have values");
  } else if (!HardFloatVTs.empty()) {
    if (Outs.size() != HardFloatVTs.size())
      report_fatal_error("SH hard-float return ABI parts are inconsistent");
    for (unsigned I = 0; I != Outs.size(); ++I)
      if (Outs[I].VT != HardFloatVTs[I] || Outs[I].Flags.isByVal() ||
          Outs[I].Flags.isSRet())
        report_fatal_error("SH hard-float return ABI type is inconsistent");
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
  if (!HardFloatVTs.empty() && !Outs.empty()) {
    for (unsigned I = 0; I != HardFloatVTs.size(); ++I) {
      MVT VT = HardFloatVTs[I];
      MCRegister Reg = VT == MVT::f32 ? (I == 0 ? SH::FR0 : SH::FR1)
                                      : (I == 0 ? SH::DR0 : SH::DR2);
      Chain = DAG.getCopyToReg(Chain, DL, Reg, OutVals[I], Glue);
      Glue = Chain.getValue(1);
      RetOps.push_back(DAG.getRegister(Reg, VT));
    }
  } else if (!Outs.empty()) {
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
  if (FuncInfo.hasTrapExit()) {
    RetOps.insert(RetOps.begin() + 1,
                  DAG.getConstant(FuncInfo.getTrapExitNumber(), DL, MVT::i32));
    return DAG.getNode(SHISD::TRAP_EXIT, DL, MVT::Other, RetOps);
  }
  if (FuncInfo.isHandler())
    return DAG.getNode(SHISD::RTE_GLUE, DL, MVT::Other, RetOps);
  return DAG.getNode(SHISD::RET_GLUE, DL, MVT::Other, RetOps);
}

static SDValue lowerSHSymbolAddress(SHConstantPoolValue *CPV, const SDLoc &DL,
                                    SelectionDAG &DAG);
static SDValue lowerSHPICPairedAddress(SHConstantPoolValue *CPV,
                                       const SDLoc &DL, SelectionDAG &DAG);
static void requireSHPICBase(SelectionDAG &DAG);

static SDValue getSHFDPICBase(const SDLoc &DL, SelectionDAG &DAG) {
  MachineRegisterInfo &MRI = DAG.getMachineFunction().getRegInfo();
  Register VReg = MRI.getLiveInVirtReg(SH::R12);
  if (!VReg) {
    VReg = MRI.createVirtualRegister(&SH::GPRNoR12RegClass);
    MRI.addLiveIn(SH::R12, VReg);
  }
  return DAG.getCopyFromReg(DAG.getEntryNode(), DL, VReg, MVT::i32);
}

SDValue SHTargetLowering::LowerCall(CallLoweringInfo &CLI,
                                    SmallVectorImpl<SDValue> &InVals) const {
  requireSupportedSHCallingConvention(CLI.CallConv);
  if (CLI.CallConv == CallingConv::Fast && CLI.IsVarArg)
    report_fatal_error(
        "SH fast calling convention does not support variadic calls");
  if (CLI.CB && CLI.CB->isMustTailCall())
    report_fatal_error("SH musttail calls are not supported");
  bool IsVariableShiftLibcall = false;
  bool IsArithmeticShiftLibcall = false;
  if (!CLI.CB) {
    if (const auto *External = dyn_cast<ExternalSymbolSDNode>(CLI.Callee)) {
      StringRef Name = External->getSymbol();
      IsArithmeticShiftLibcall = Name == "__ashrsi3";
      IsVariableShiftLibcall = Name == "__ashlsi3" || Name == "__lshrsi3" ||
                               IsArithmeticShiftLibcall;
    }
  }

  Type *ReturnTy = CLI.OrigRetTy;
  if (!ReturnTy)
    report_fatal_error("SH call is missing its return type");
  SHAtomicRuntimeCallKind AtomicCallKind =
      CLI.CB ? getSHAtomicRuntimeCallKind(*CLI.CB)
             : SHAtomicRuntimeCallKind::None;
  bool IsAtomicBooleanResult =
      ReturnTy->isIntegerTy(1) &&
      AtomicCallKind == SHAtomicRuntimeCallKind::CompareExchange;
  bool IsNarrowAtomicResult =
      AtomicCallKind == SHAtomicRuntimeCallKind::Operation &&
      (ReturnTy->isIntegerTy(8) || ReturnTy->isIntegerTy(16));
  SmallVector<MVT, 2> HardFloatReturnVTs =
      Subtarget.useHardFloatABI()
          ? getSHFPUABIValueTypes(ReturnTy, Subtarget.usesSH4HardFloatABI())
          : SmallVector<MVT, 2>();
  if (HardFloatReturnVTs.empty() && ReturnTy->isAggregateType() &&
      !isDirectSHAggregateReturn(CLI.DAG.getDataLayout(), ReturnTy))
    report_fatal_error("SH indirect aggregate call result is missing sret");
  if (!ReturnTy->isVoidTy() && !isSupportedSHValueType(ReturnTy) &&
      !IsAtomicBooleanResult && !IsNarrowAtomicResult)
    report_fatal_error(
        "SH calls only support void, i32, i64, f32, f64, and pointer return "
        "values");
  SmallVector<SHValuePart, 4> ReturnParts;
  if (!ReturnTy->isVoidTy() && !IsAtomicBooleanResult)
    ReturnParts = getSHValueParts(*this, CLI.DAG.getDataLayout(), ReturnTy);
  if ((IsAtomicBooleanResult ? 1 : ReturnParts.size()) != CLI.Ins.size() ||
      llvm::any_of(enumerate(CLI.Ins), [&](auto IndexedIn) {
        MVT Expected = HardFloatReturnVTs.empty()
                           ? MVT::i32
                           : HardFloatReturnVTs[IndexedIn.index()];
        return IndexedIn.value().VT != Expected;
      }))
    report_fatal_error("SH call result ABI parts are inconsistent");
  if (CLI.OutVals.size() != CLI.Outs.size())
    report_fatal_error("SH call argument part count is inconsistent");

  SelectionDAG &DAG = CLI.DAG;
  MachineFunction &MF = DAG.getMachineFunction();
  SHMachineFunctionInfo &FuncInfo = *MF.getInfo<SHMachineFunctionInfo>();
  const DataLayout &DataLayout = DAG.getDataLayout();
  struct SHCallArgument {
    const ArgListEntry *Arg;
    SHABIValue ABIValue;
    SmallVector<unsigned, 4> PartIndices;
    bool IsSRet;
    unsigned ArgIndex;
  };
  SmallVector<SHCallArgument, 8> CallArguments;
  SHABIState ABIState;
  bool HasNestArgument = false;
  unsigned NumFixedArgs = CLI.CB && CLI.CB->getFunctionType()->isVarArg()
                              ? CLI.CB->getFunctionType()->getNumParams()
                              : CLI.Args.size();
  for (unsigned ArgIndex = 0; ArgIndex != CLI.Args.size(); ++ArgIndex) {
    const ArgListEntry &Arg = CLI.Args[ArgIndex];
    bool IsUnnamedVarArg = CLI.IsVarArg && ArgIndex >= NumFixedArgs;
    if (IsUnnamedVarArg &&
        (!Arg.OrigTy ||
         (!isSupportedSHVarArgType(Arg.OrigTy) &&
          !(Subtarget.useHardFloatABI() && Arg.OrigTy->isFloatTy()))))
      report_fatal_error(
          "SH variadic arguments must use supported default-promoted ABI "
          "types");
    if (!Arg.OrigTy || Arg.IsInAlloca || Arg.IsPreallocated || Arg.IsByRef ||
        Arg.IsSwiftSelf || Arg.IsSwiftAsync || Arg.IsSwiftError)
      report_fatal_error("SH call argument has unsupported ABI flags");
    SmallVector<unsigned, 4> PartIndices;
    for (unsigned I = 0; I != CLI.Outs.size(); ++I)
      if (CLI.Outs[I].OrigArgIndex == ArgIndex)
        PartIndices.push_back(I);
    if (PartIndices.empty())
      report_fatal_error("SH call argument has no ABI parts");
    for (unsigned I : PartIndices)
      if ((CLI.Outs[I].VT != MVT::i32 &&
           !isSHFPUABIValueType(Subtarget, CLI.Outs[I].VT)) ||
          CLI.Outs[I].Flags.isByRef() || CLI.Outs[I].Flags.isInAlloca() ||
          CLI.Outs[I].Flags.isPreallocated() ||
          CLI.Outs[I].Flags.isSwiftSelf() || CLI.Outs[I].Flags.isSwiftAsync() ||
          CLI.Outs[I].Flags.isSwiftError())
        report_fatal_error("SH call argument ABI part is not supported");

    if (Arg.IsNest) {
      if (HasNestArgument || PartIndices.size() != 1 ||
          CLI.Outs[PartIndices[0]].VT != MVT::i32 ||
          !CLI.Outs[PartIndices[0]].Flags.isNest() ||
          !Arg.OrigTy->isPointerTy())
        report_fatal_error("SH nest call argument must be one 32-bit pointer");
      HasNestArgument = true;
      SHABIValue ABIValue{4, {}};
      ABIValue.Words.push_back({0, 4, 0, SH::R3, -1});
      CallArguments.push_back(
          {&Arg, std::move(ABIValue), std::move(PartIndices), false, ArgIndex});
      continue;
    }

    if (Arg.IsSRet) {
      if (PartIndices.size() != 1 || !CLI.Outs[PartIndices[0]].Flags.isSRet() ||
          CLI.Outs[PartIndices[0]].Flags.isByVal())
        report_fatal_error("SH sret call argument must be one pointer");
      CallArguments.push_back(
          {&Arg, {0, {}}, std::move(PartIndices), true, ArgIndex});
      continue;
    }

    Type *ABIType = Arg.IsByVal ? Arg.IndirectType : Arg.OrigTy;
    bool IsNarrowAtomicArgument =
        AtomicCallKind != SHAtomicRuntimeCallKind::None && !Arg.IsByVal &&
        (ABIType->isIntegerTy(8) || ABIType->isIntegerTy(16));
    uint64_t Size =
        IsNarrowAtomicArgument ? 4 : getSHABITypeSize(DataLayout, ABIType);
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
    MVT ComplexElementVT = getSHHardFloatComplexElementType(ABIType);
    if (ComplexElementVT == MVT::f64 && !Subtarget.usesSH4HardFloatABI())
      ComplexElementVT = MVT::Other;
    SHABIValue ABIValue =
        Subtarget.useHardFloatABI() &&
                (Arg.IsByVal ||
                 (Subtarget.usesEFloatABI() && ABIType->isDoubleTy()) ||
                 (!ABIType->isFloatingPointTy() &&
                  ComplexElementVT == MVT::Other))
            ? ABIState.allocateWhole(Size, Subtarget.usesEFloatABI())
        : !Arg.IsByVal &&
                ((ABIType->isFloatTy() && Subtarget.useHardFloatABI()) ||
                 (ABIType->isDoubleTy() && Subtarget.usesSH4HardFloatABI()))
            ? ABIState.allocateFloat(ABIType->isFloatTy() ? MVT::f32 : MVT::f64,
                                     DataLayout.isLittleEndian(),
                                     Subtarget.usesEFloatABI())
        : Subtarget.useHardFloatABI() && !Arg.IsByVal &&
                ComplexElementVT != MVT::Other
            ? ABIState.allocateComplex(ComplexElementVT,
                                       DataLayout.isLittleEndian(),
                                       Subtarget.usesEFloatABI())
            : ABIState.allocate(Size);
    CallArguments.push_back(
        {&Arg, std::move(ABIValue), std::move(PartIndices), false, ArgIndex});
  }

  unsigned StackBytes = ABIState.getStackSize();
  if (StackBytes > SHMaxCallFrameSize || StackBytes % 4 != 0)
    report_fatal_error(Twine("SH outgoing call frame size cannot exceed ") +
                       Twine(SHMaxCallFrameSize) + " bytes");
  bool CanReuseStackArguments =
      StackBytes == 0 ||
      (CLI.IsTailCall &&
       llvm::all_of(CallArguments, [&](const SHCallArgument &CallArg) {
         for (const SHABIWord &Word : CallArg.ABIValue.Words) {
           if (Word.isRegister())
             continue;
           const auto *Formal =
               CLI.CB && CLI.CB->arg_size() == CLI.Args.size()
                   ? dyn_cast<Argument>(CLI.CB->getArgOperand(CallArg.ArgIndex))
                   : nullptr;
           std::optional<int64_t> IncomingOffset =
               Formal && Formal->getParent() == &MF.getFunction()
                   ? FuncInfo.getIncomingStackArgumentOffset(Formal->getArgNo())
                   : std::nullopt;
           if (CallArg.Arg->IsByVal || CallArg.IsSRet ||
               CallArg.ABIValue.Size != 4 ||
               CallArg.ABIValue.isFloatingPoint() ||
               CallArg.Arg->OrigTy->isAggregateType() ||
               CallArg.PartIndices.size() != 1 || !IncomingOffset ||
               *IncomingOffset != Word.StackOffset)
             return false;
         }
         return true;
       }));
  const Function &Caller = MF.getFunction();
  // SH C and fast calls use identical argument, result, and preserved-register
  // rules. Fast only further restricts linkage and variadic calls.
  bool HasCompatibleCallingConvention =
      (Caller.getCallingConv() == CallingConv::C ||
       Caller.getCallingConv() == CallingConv::Fast) &&
      (CLI.CallConv == CallingConv::C || CLI.CallConv == CallingConv::Fast);
  CLI.IsTailCall =
      CLI.IsTailCall && CLI.CB && CanReuseStackArguments &&
      HasCompatibleCallingConvention && !CLI.IsVarArg && !Caller.isVarArg() &&
      ReturnTy == Caller.getReturnType() && !FuncInfo.isHandler() &&
      isa<GlobalAddressSDNode>(CLI.Callee) &&
      !getTargetMachine().isPositionIndependent() && !HasNestArgument &&
      llvm::none_of(CallArguments, [](const SHCallArgument &Arg) {
        return Arg.Arg->IsByVal || Arg.IsSRet;
      });
  if (!CLI.IsTailCall && StackBytes > 256)
    FuncInfo.setNeedsCallFrameScratch();
  SDValue Chain = CLI.IsTailCall
                      ? CLI.Chain
                      : DAG.getCALLSEQ_START(CLI.Chain, StackBytes, 0, CLI.DL);
  bool NeedsNarrowStackAddress =
      llvm::any_of(CallArguments, [](const SHCallArgument &CallArg) {
        return llvm::any_of(CallArg.ABIValue.Words, [](const SHABIWord &Word) {
          return !Word.isRegister() && Word.ValidBytes != 4;
        });
      });
  SDValue StackPtr;
  if (NeedsNarrowStackAddress || StackBytes > 60 ||
      (Subtarget.useHardFloatABI() && StackBytes != 0)) {
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
    SmallVector<SDValue, 4> Values;
    for (unsigned I : CallArg.PartIndices)
      Values.push_back(CLI.OutVals[I]);
    if (CallArg.ABIValue.isFloatingPoint()) {
      if (Values.size() != CallArg.ABIValue.Words.size() ||
          llvm::any_of(Values, [&](SDValue Value) {
            return Value.getValueType() != CallArg.ABIValue.ValueVT;
          }))
        report_fatal_error(
            "SH hard-float call argument parts are inconsistent");
      ArgumentWords.push_back(std::move(Values));
      continue;
    }
    SmallVector<SHValuePart, 4> Parts =
        getSHValueParts(*this, DataLayout, CallArg.Arg->OrigTy);
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
      if (CLI.IsTailCall)
        continue;
      unsigned StackOffset = Word.StackOffset;
      if (CallArg.ABIValue.isFloatingPoint()) {
        SDValue Address =
            getSHMemoryAddress(DAG, CLI.DL, StackPtr, StackOffset);
        StoreChains.push_back(DAG.getStore(
            Chain, CLI.DL, Words[WordIndex], Address,
            MachinePointerInfo::getStack(MF, StackOffset), Align(4)));
        continue;
      }
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
  SDValue Callee = CLI.Callee;
  bool IsDirectPCRelCall = false;
  bool IsIndirectFDPICCall = false;
  SDValue DirectCallBase;
  if (const auto *Global = dyn_cast<GlobalAddressSDNode>(Callee)) {
    const GlobalValue *CalleeGlobal = Global->getGlobal();
    const auto *CalleeFunction = dyn_cast<Function>(CalleeGlobal);
    const auto *CalleeAlias = dyn_cast<GlobalAlias>(CalleeGlobal);
    if (!CalleeFunction &&
        (!CalleeAlias ||
         !isa_and_nonnull<Function>(CalleeAlias->getAliaseeObject())))
      report_fatal_error("SH direct call target is not a function");
    bool IsNonPreemptible =
        Subtarget.isFDPIC()
            ? CalleeGlobal->hasLocalLinkage() ||
                  CalleeGlobal->hasHiddenVisibility() ||
                  CalleeGlobal->hasProtectedVisibility()
            : CalleeGlobal->isDSOLocal() && !CalleeGlobal->isInterposable();
    if (Subtarget.isFDPIC()) {
      if (Global->getOffset() != 0)
        report_fatal_error("SH FDPIC direct call target cannot have an addend");
      MCSymbol *PCRelativeBase =
          MF.getContext().createTempSymbol("sh_fdpic_call", true);
      SHConstantPoolValue *CPV = SHConstantPoolValue::create(
          CalleeGlobal, 0,
          IsNonPreemptible ? SHConstantPoolValue::Modifier::PCREL
                           : SHConstantPoolValue::Modifier::PLT);
      CPV->setPCRelativeBase(PCRelativeBase);
      Callee = lowerSHSymbolAddress(CPV, CLI.DL, DAG);
      DirectCallBase = DAG.getMCSymbol(PCRelativeBase, MVT::i32);
      IsDirectPCRelCall = true;
    } else if (getTargetMachine().isPositionIndependent() &&
               !IsNonPreemptible) {
      if (Global->getOffset() != 0)
        report_fatal_error("SH direct call target cannot have an addend");
      SHConstantPoolValue *CPV = SHConstantPoolValue::create(
          CalleeGlobal, 0, SHConstantPoolValue::Modifier::PLT);
      Callee = lowerSHPICPairedAddress(CPV, CLI.DL, DAG);
    } else
      Callee = LowerGlobalAddress(Callee, DAG);
  } else if (isa<ExternalSymbolSDNode>(Callee)) {
    const auto *External = cast<ExternalSymbolSDNode>(Callee);
    MCSymbol *PCRelativeBase =
        Subtarget.isFDPIC()
            ? MF.getContext().createTempSymbol("sh_fdpic_call", true)
            : nullptr;
    SHConstantPoolValue *CPV =
        SHConstantPoolValue::create(*DAG.getContext(), External->getSymbol(), 0,
                                    getTargetMachine().isPositionIndependent()
                                        ? SHConstantPoolValue::Modifier::PLT
                                        : SHConstantPoolValue::Modifier::None);
    if (PCRelativeBase)
      CPV->setPCRelativeBase(PCRelativeBase);
    Callee = Subtarget.isFDPIC() ? lowerSHSymbolAddress(CPV, CLI.DL, DAG)
             : getTargetMachine().isPositionIndependent()
                 ? lowerSHPICPairedAddress(CPV, CLI.DL, DAG)
                 : lowerSHSymbolAddress(CPV, CLI.DL, DAG);
    if (Subtarget.isFDPIC()) {
      DirectCallBase = DAG.getMCSymbol(PCRelativeBase, MVT::i32);
      IsDirectPCRelCall = true;
    }
  } else if (Callee.getValueType() != MVT::i32) {
    report_fatal_error("SH indirect call target must be a 32-bit GPR value");
  } else if (Subtarget.isFDPIC()) {
    SDValue Entry =
        DAG.getLoad(MVT::i32, CLI.DL, Chain, Callee, MachinePointerInfo(),
                    Align(4), MachineMemOperand::MOVolatile);
    SDValue ContextAddress = DAG.getNode(ISD::ADD, CLI.DL, MVT::i32, Callee,
                                         DAG.getConstant(4, CLI.DL, MVT::i32));
    SDValue Context = DAG.getLoad(MVT::i32, CLI.DL, Entry.getValue(1),
                                  ContextAddress, MachinePointerInfo(),
                                  Align(4), MachineMemOperand::MOVolatile);
    Chain = Context.getValue(1);
    Chain = DAG.getCopyToReg(Chain, CLI.DL, SH::R1, Entry, Glue);
    Glue = Chain.getValue(1);
    Chain = DAG.getCopyToReg(Chain, CLI.DL, SH::R12, Context, Glue);
    Glue = Chain.getValue(1);
    Callee = DAG.getRegister(SH::R1, MVT::i32);
    IsIndirectFDPICCall = true;
  }
  if (Subtarget.isFDPIC() && IsDirectPCRelCall) {
    Chain = DAG.getCopyToReg(Chain, CLI.DL, SH::R12,
                             getSHFDPICBase(CLI.DL, DAG), Glue);
    Glue = Chain.getValue(1);
  }
  for (const auto &[Reg, Value] : RegsToPass) {
    Chain = DAG.getCopyToReg(Chain, CLI.DL, Reg, Value, Glue);
    Glue = Chain.getValue(1);
  }

  SmallVector<SDValue, 8> CallOps;
  CallOps.push_back(Chain);
  CallOps.push_back(Callee);
  const SHRegisterInfo *RegisterInfo =
      MF.getSubtarget<SHSubtarget>().getRegisterInfo();
  const uint32_t *Mask =
      IsVariableShiftLibcall
          ? RegisterInfo->getShiftLibcallPreservedMask(IsArithmeticShiftLibcall)
          : RegisterInfo->getCallPreservedMask(MF, CLI.CallConv);
  if (!Mask)
    report_fatal_error("SH C calling convention has no call-preserved mask");
  CallOps.push_back(DAG.getRegisterMask(Mask));
  if (DirectCallBase)
    CallOps.push_back(DirectCallBase);
  for (const auto &[Reg, Value] : RegsToPass)
    CallOps.push_back(DAG.getRegister(Reg, Value.getValueType()));
  if (Subtarget.isFDPIC())
    CallOps.push_back(DAG.getRegister(SH::R12, MVT::i32));
  if (IsIndirectFDPICCall)
    CallOps.push_back(DAG.getRegister(SH::R1, MVT::i32));
  if (Glue)
    CallOps.push_back(Glue);

  if (CLI.IsTailCall) {
    MF.getFrameInfo().setHasTailCall();
    return DAG.getNode(SHISD::TAIL_CALL, CLI.DL, MVT::Other, CallOps);
  }

  SDVTList CallVTs = DAG.getVTList(MVT::Other, MVT::Glue);
  Chain = DAG.getNode(IsDirectPCRelCall ? SHISD::CALL_PCREL : SHISD::CALL,
                      CLI.DL, CallVTs, CallOps);
  DAG.addNoMergeSiteInfo(Chain.getNode(), CLI.NoMerge);
  Glue = Chain.getValue(1);
  Chain = DAG.getCALLSEQ_END(Chain, StackBytes, 0, Glue, CLI.DL);
  Glue = Chain.getValue(1);

  if (!CLI.Ins.empty()) {
    if (!HardFloatReturnVTs.empty()) {
      for (unsigned I = 0; I != HardFloatReturnVTs.size(); ++I) {
        MVT VT = HardFloatReturnVTs[I];
        MCRegister Reg = VT == MVT::f32 ? (I == 0 ? SH::FR0 : SH::FR1)
                                        : (I == 0 ? SH::DR0 : SH::DR2);
        SDValue Result = DAG.getCopyFromReg(Chain, CLI.DL, Reg, VT, Glue);
        InVals.push_back(Result);
        Chain = Result.getValue(1);
        Glue = Result.getValue(2);
      }
      return Chain;
    }
    SHABIState ReturnState;
    SHABIValue ABIValue =
        ReturnState.allocate(IsAtomicBooleanResult || IsNarrowAtomicResult
                                 ? 4
                                 : getSHABITypeSize(DataLayout, ReturnTy));
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
  if (Intrinsic == Intrinsic::sh_ftrv || Intrinsic == Intrinsic::sh_ftrv4 ||
      Intrinsic == Intrinsic::sh_ftrv_array ||
      Intrinsic == Intrinsic::sh_ftrv_compose) {
    bool Single = Intrinsic == Intrinsic::sh_ftrv;
    bool Counted = Intrinsic == Intrinsic::sh_ftrv_array ||
                   Intrinsic == Intrinsic::sh_ftrv_compose;
    unsigned Args = Single                                  ? 1
                    : Intrinsic == Intrinsic::sh_ftrv4      ? 3
                    : Intrinsic == Intrinsic::sh_ftrv_array ? 4
                                                            : 5;
    for (unsigned Arg = 0; Arg != Args; ++Arg) {
      if (Counted && Arg == 2)
        continue;
      IntrinsicInfo Info;
      Info.opc = Single ? ISD::INTRINSIC_W_CHAIN : ISD::INTRINSIC_VOID;
      Info.memVT = MVT::v16f32;
      Info.ptrVal = I.getArgOperand(Arg);
      Info.size = 64;
      if (Counted && Arg < 2) {
        Info.size = MemoryLocation::UnknownSize;
        if (const auto *N = dyn_cast<ConstantInt>(I.getArgOperand(2));
            N && !N->isZero())
          Info.size = 16 * N->getZExtValue();
      }
      Info.align = std::max(Align(4), I.getParamAlign(Arg).valueOrOne());
      Info.flags = !Single && Arg == 0 ? MachineMemOperand::MOStore
                                       : MachineMemOperand::MOLoad;
      Infos.push_back(Info);
    }
    return;
  }
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
  if (MF.getSubtarget<SHSubtarget>().useHardFloatABI()) {
    if (!FuncInfo.hasFPUVarArgsAreas())
      report_fatal_error("SH hard-float va_start is missing its save areas");
    SDValue GPRBase =
        DAG.getFrameIndex(FuncInfo.getVarArgsGPRFrameIndex(), MVT::i32);
    SDValue FPRBase =
        DAG.getFrameIndex(FuncInfo.getVarArgsFPRFrameIndex(), MVT::i32);
    SDValue Stack =
        DAG.getFrameIndex(FuncInfo.getVarArgsStackFrameIndex(), MVT::i32);
    SDValue Fields[] = {getSHMemoryAddress(DAG, DL, GPRBase,
                                           FuncInfo.getVarArgsGPRCursor() * 4),
                        getSHMemoryAddress(DAG, DL, GPRBase, 16),
                        getSHMemoryAddress(DAG, DL, FPRBase,
                                           FuncInfo.getVarArgsFPRCursor() * 4),
                        getSHMemoryAddress(DAG, DL, FPRBase, 32), Stack};
    const Value *Storage = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
    SmallVector<SDValue, 5> Stores;
    for (unsigned I = 0; I != std::size(Fields); ++I) {
      SDValue Address = getSHMemoryAddress(DAG, DL, Op.getOperand(1), I * 4);
      Stores.push_back(DAG.getStore(Op.getOperand(0), DL, Fields[I], Address,
                                    MachinePointerInfo(Storage, I * 4),
                                    Align(4)));
    }
    return DAG.getNode(ISD::TokenFactor, DL, MVT::Other, Stores);
  }
  SDValue Cursor = DAG.getFrameIndex(FuncInfo.getVarArgsFrameIndex(), MVT::i32);
  const Value *Storage = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  return DAG.getStore(Op.getOperand(0), DL, Cursor, Op.getOperand(1),
                      MachinePointerInfo(Storage), Align(4));
}

SDValue SHTargetLowering::LowerVAARG(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  EVT VT = Op.getValueType();
  SDValue Chain = Op.getOperand(0), List = Op.getOperand(1);
  const Value *Storage = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  unsigned Size = VT.getStoreSize();
  assert((Size == 4 || Size == 8) && "unsupported SH va_arg type");
  auto Address = [&](unsigned Offset) {
    return getSHMemoryAddress(DAG, DL, List, Offset);
  };
  auto LoadField = [&](unsigned Offset) {
    SDValue Load = DAG.getLoad(MVT::i32, DL, Chain, Address(Offset),
                               MachinePointerInfo(Storage, Offset), Align(4));
    Chain = Load.getValue(1);
    return Load;
  };
  auto StoreField = [&](unsigned Offset, SDValue Value) {
    Chain = DAG.getStore(Chain, DL, Value, Address(Offset),
                         MachinePointerInfo(Storage, Offset), Align(4));
  };
  auto AddSize = [&](SDValue Cursor) {
    return getSHMemoryAddress(DAG, DL, Cursor, Size);
  };

  SDValue ValueAddress;
  if (!Subtarget.useHardFloatABI()) {
    ValueAddress = LoadField(0);
    StoreField(0, AddSize(ValueAddress));
  } else {
    // GNU hard-float va_list: GPR cursor/limit, FPR cursor/limit, overflow.
    // E-float passes doubles in GPRs; SH4 aligns register doubles to a pair.
    bool InFPRs = VT == MVT::f64 && Subtarget.usesSH4HardFloatABI();
    unsigned CursorOffset = InFPRs ? 8 : 0;
    SDValue Cursor = LoadField(CursorOffset);
    SDValue Limit = LoadField(CursorOffset + 4);
    SDValue Stack = LoadField(16);
    SDValue Candidate = Cursor;
    if (InFPRs) {
      Candidate = getSHMemoryAddress(DAG, DL, Candidate, 7);
      Candidate = DAG.getNode(ISD::AND, DL, MVT::i32, Candidate,
                              DAG.getSignedConstant(-8, DL, MVT::i32));
    }
    SDValue Next = AddSize(Candidate);
    SDValue Fits = DAG.getSetCC(DL, MVT::i32, Next, Limit, ISD::SETULE);
    auto Select = [&](SDValue Yes, SDValue No) {
      return DAG.getNode(ISD::SELECT, DL, MVT::i32, Fits, Yes, No);
    };
    // Match GNU's E-float exhaustion rule. SH4 may still use the remaining
    // single GPR after a two-word argument has gone entirely to the stack.
    SDValue Exhausted = !InFPRs && Subtarget.usesEFloatABI() ? Limit : Cursor;
    StoreField(CursorOffset, Select(Next, Exhausted));
    StoreField(16, Select(Stack, AddSize(Stack)));
    ValueAddress = Select(Candidate, Stack);
  }
  return DAG.getLoad(VT, DL, Chain, ValueAddress, MachinePointerInfo(),
                     Align(4));
}

SDValue SHTargetLowering::LowerVACOPY(SDValue Op, SelectionDAG &DAG) const {
  assert(Subtarget.useHardFloatABI() &&
         "SH soft-float va_copy should use the generic expansion");
  SDLoc DL(Op);
  const Value *DestSV = cast<SrcValueSDNode>(Op.getOperand(3))->getValue();
  const Value *SrcSV = cast<SrcValueSDNode>(Op.getOperand(4))->getValue();
  return DAG.getMemcpy(Op.getOperand(0), DL, Op.getOperand(1), Op.getOperand(2),
                       DAG.getConstant(20, DL, MVT::i32), Align(4), Align(4),
                       /*isVolatile=*/false, /*AlwaysInline=*/false,
                       /*CI=*/nullptr, std::nullopt, MachinePointerInfo(DestSV),
                       MachinePointerInfo(SrcSV));
}

SDValue SHTargetLowering::LowerATOMIC_FENCE(SDValue Op,
                                            SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SyncScope::ID Scope = static_cast<SyncScope::ID>(Op.getConstantOperandVal(2));
  if (Scope != SyncScope::SingleThread && Scope != SyncScope::System)
    report_fatal_error("SH synchronization scope is not supported");
  if (Scope == SyncScope::System && Subtarget.hasSH4ACommon())
    return DAG.getNode(SHISD::SYNCO, DL, MVT::Other, Op.getOperand(0));
  // Classic SH processors are strongly ordered and have no hardware memory
  // barrier instruction. GCC therefore lowers __sync_synchronize to a
  // compiler barrier for these CPUs as well.
  return DAG.getNode(ISD::MEMBARRIER, DL, MVT::Other, Op.getOperand(0));
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

static void requireSHPICBase(SelectionDAG &DAG) {
  MachineFunction &MF = DAG.getMachineFunction();
  SHMachineFunctionInfo &FuncInfo = *MF.getInfo<SHMachineFunctionInfo>();
  if (FuncInfo.usesPICBase())
    return;
  SHConstantPoolValue *GOTPC =
      SHConstantPoolValue::create(*DAG.getContext(), "_GLOBAL_OFFSET_TABLE_", 0,
                                  SHConstantPoolValue::Modifier::GOTPC);
  unsigned CPI = MF.getConstantPool()->getConstantPoolIndex(GOTPC, Align(4));
  FuncInfo.setPICBaseCPI(CPI);
  MF.setAlignment(std::max(MF.getAlignment(), Align(4)));
}

static SDValue lowerSHPICLiteralAddress(SHConstantPoolValue *CPV,
                                        unsigned Opcode, const SDLoc &DL,
                                        SelectionDAG &DAG) {
  MachineFunction &MF = DAG.getMachineFunction();
  if (!MF.getSubtarget<SHSubtarget>().isFDPIC())
    requireSHPICBase(DAG);
  MF.setAlignment(std::max(MF.getAlignment(), Align(4)));
  SDValue CPAddr = DAG.getTargetConstantPool(CPV, MVT::i32, Align(4));
  SDValue Ops[] = {DAG.getEntryNode(), CPAddr};
  MachineMemOperand::Flags Flags = MachineMemOperand::MOLoad |
                                   MachineMemOperand::MODereferenceable |
                                   MachineMemOperand::MOInvariant;
  return DAG.getMemIntrinsicNode(
      Opcode, DL, DAG.getVTList(MVT::i32, MVT::Other), Ops, MVT::i32,
      MachinePointerInfo::getConstantPool(MF), Align(4), Flags,
      LocationSize::precise(4));
}

static SDValue lowerSHPICOffsetAddress(SHConstantPoolValue *CPV,
                                       const SDLoc &DL, SelectionDAG &DAG) {
  return lowerSHPICLiteralAddress(CPV, SHISD::PIC_GOTOFF, DL, DAG);
}

static SDValue lowerSHPICGOTAddress(const GlobalValue *GV, int32_t Addend,
                                    const SDLoc &DL, SelectionDAG &DAG) {
  SHConstantPoolValue *CPV =
      SHConstantPoolValue::create(GV, 0, SHConstantPoolValue::Modifier::GOT);
  SDValue Slot = lowerSHPICOffsetAddress(CPV, DL, DAG);
  MachineFunction &MF = DAG.getMachineFunction();
  SDValue Address = DAG.getLoad(MVT::i32, DL, DAG.getEntryNode(), Slot,
                                MachinePointerInfo::getGOT(MF), Align(4),
                                MachineMemOperand::MOLoad |
                                    MachineMemOperand::MODereferenceable |
                                    MachineMemOperand::MOInvariant);
  if (Addend != 0)
    Address = DAG.getNode(ISD::ADD, DL, MVT::i32, Address,
                          DAG.getSignedConstant(Addend, DL, MVT::i32));
  return Address;
}

static SDValue lowerSHFDPICOffsetAddress(SHConstantPoolValue *CPV,
                                         const SDLoc &DL, SelectionDAG &DAG) {
  SDValue Offset = lowerSHSymbolAddress(CPV, DL, DAG);
  return DAG.getNode(ISD::ADD, DL, MVT::i32, getSHFDPICBase(DL, DAG), Offset);
}

static SDValue lowerSHFDPICGOTAddress(const GlobalValue *GV, int32_t Addend,
                                      SHConstantPoolValue::Modifier Modifier,
                                      const SDLoc &DL, SelectionDAG &DAG) {
  SHConstantPoolValue *CPV = SHConstantPoolValue::create(GV, 0, Modifier);
  SDValue Slot = lowerSHFDPICOffsetAddress(CPV, DL, DAG);
  MachineFunction &MF = DAG.getMachineFunction();
  SDValue Address = DAG.getLoad(MVT::i32, DL, DAG.getEntryNode(), Slot,
                                MachinePointerInfo::getGOT(MF), Align(4),
                                MachineMemOperand::MOLoad |
                                    MachineMemOperand::MODereferenceable |
                                    MachineMemOperand::MOInvariant);
  if (Addend)
    Address = DAG.getNode(ISD::ADD, DL, MVT::i32, Address,
                          DAG.getSignedConstant(Addend, DL, MVT::i32));
  return Address;
}

static bool isSHFunctionGlobal(const GlobalValue *GV) {
  if (isa<Function>(GV))
    return true;
  const auto *GA = dyn_cast<GlobalAlias>(GV);
  return GA && isa_and_nonnull<Function>(GA->getAliaseeObject());
}

static SDValue lowerSHPICPairedAddress(SHConstantPoolValue *CPV,
                                       const SDLoc &DL, SelectionDAG &DAG) {
  return lowerSHPICLiteralAddress(CPV, SHISD::PIC_ADDRESS, DL, DAG);
}

SDValue SHTargetLowering::LowerGlobalAddress(SDValue Op,
                                             SelectionDAG &DAG) const {
  const auto *Global = cast<GlobalAddressSDNode>(Op);
  int64_t Addend = Global->getOffset();
  if (!isInt<32>(Addend))
    report_fatal_error("SH global address addend must fit signed 32 bits");
  if (getTargetMachine().isPositionIndependent()) {
    const GlobalValue *GV = Global->getGlobal();
    if (Subtarget.isFDPIC()) {
      bool IsNonPreemptible =
          GV->hasLocalLinkage() || GV->hasHiddenVisibility();
      if (isSHFunctionGlobal(GV)) {
        if (IsNonPreemptible && !GV->hasProtectedVisibility()) {
          SHConstantPoolValue *CPV = SHConstantPoolValue::create(
              GV, static_cast<int32_t>(Addend),
              SHConstantPoolValue::Modifier::GOTOFFFUNCDESC);
          return lowerSHFDPICOffsetAddress(CPV, SDLoc(Op), DAG);
        }
        return lowerSHFDPICGOTAddress(
            GV, static_cast<int32_t>(Addend),
            SHConstantPoolValue::Modifier::GOTFUNCDESC, SDLoc(Op), DAG);
      }
      if (!IsNonPreemptible)
        return lowerSHFDPICGOTAddress(GV, static_cast<int32_t>(Addend),
                                      SHConstantPoolValue::Modifier::GOT,
                                      SDLoc(Op), DAG);
      if (const auto *Variable = dyn_cast<GlobalVariable>(GV);
          Variable && Variable->isConstant()) {
        if (Variable->isDeclarationForLinker() ||
            TargetLoweringObjectFile::getKindForGlobal(Variable,
                                                       getTargetMachine())
                .isReadOnlyWithRel())
          return lowerSHFDPICGOTAddress(GV, static_cast<int32_t>(Addend),
                                        SHConstantPoolValue::Modifier::GOT,
                                        SDLoc(Op), DAG);
        SHConstantPoolValue *CPV =
            SHConstantPoolValue::create(GV, static_cast<int32_t>(Addend),
                                        SHConstantPoolValue::Modifier::PCREL);
        return lowerSHPICPairedAddress(CPV, SDLoc(Op), DAG);
      }
      SHConstantPoolValue *CPV =
          SHConstantPoolValue::create(GV, static_cast<int32_t>(Addend),
                                      SHConstantPoolValue::Modifier::GOTOFF);
      return lowerSHFDPICOffsetAddress(CPV, SDLoc(Op), DAG);
    }
    if (!GV->isDSOLocal() || GV->isInterposable())
      return lowerSHPICGOTAddress(GV, static_cast<int32_t>(Addend), SDLoc(Op),
                                  DAG);
    SHConstantPoolValue *CPV =
        SHConstantPoolValue::create(GV, static_cast<int32_t>(Addend),
                                    SHConstantPoolValue::Modifier::GOTOFF);
    return lowerSHPICOffsetAddress(CPV, SDLoc(Op), DAG);
  }
  SHConstantPoolValue *CPV = SHConstantPoolValue::create(
      Global->getGlobal(), static_cast<int32_t>(Addend));
  return lowerSHSymbolAddress(CPV, SDLoc(Op), DAG);
}

static SDValue addSHTLSAddressOffset(SDValue Address, int64_t Offset,
                                     const SDLoc &DL, SelectionDAG &DAG) {
  if (Offset == 0)
    return Address;
  if (!isInt<32>(Offset))
    report_fatal_error("SH TLS address addend must fit signed 32 bits");
  return DAG.getNode(ISD::ADD, DL, MVT::i32, Address,
                     DAG.getSignedConstant(Offset, DL, MVT::i32));
}

static SDValue lowerSHTLSResolverCall(const GlobalValue *GV,
                                      SHConstantPoolValue::Modifier Modifier,
                                      const SDLoc &DL, SelectionDAG &DAG) {
  MachineFunction &MF = DAG.getMachineFunction();
  bool IsFDPIC = MF.getSubtarget<SHSubtarget>().isFDPIC();
  if (!IsFDPIC)
    requireSHPICBase(DAG);
  MF.setAlignment(std::max(MF.getAlignment(), Align(4)));
  SHConstantPoolValue *Descriptor =
      SHConstantPoolValue::create(GV, 0, Modifier);
  SHConstantPoolValue *Resolver =
      SHConstantPoolValue::create(*DAG.getContext(), "__tls_get_addr", 0,
                                  SHConstantPoolValue::Modifier::PLT);
  SDValue DescriptorCP =
      DAG.getTargetConstantPool(Descriptor, MVT::i32, Align(4));
  SDValue ResolverCP = DAG.getTargetConstantPool(Resolver, MVT::i32, Align(4));

  SDValue Chain = DAG.getCALLSEQ_START(DAG.getEntryNode(), 0, 0, DL);
  if (IsFDPIC)
    Chain = DAG.getCopyToReg(Chain, DL, SH::R12, getSHFDPICBase(DL, DAG));
  const uint32_t *Mask =
      MF.getSubtarget<SHSubtarget>().getRegisterInfo()->getCallPreservedMask(
          MF, CallingConv::C);
  if (!Mask)
    report_fatal_error("SH TLS resolver has no call-preserved mask");
  SDValue Ops[] = {Chain, DescriptorCP, ResolverCP, DAG.getRegisterMask(Mask)};
  MachineMemOperand::Flags Flags = MachineMemOperand::MOLoad |
                                   MachineMemOperand::MODereferenceable |
                                   MachineMemOperand::MOInvariant;
  Chain = DAG.getMemIntrinsicNode(
      SHISD::TLS_CALL, DL, DAG.getVTList(MVT::Other, MVT::Glue), Ops, MVT::i32,
      MachinePointerInfo::getConstantPool(MF), Align(4), Flags,
      LocationSize::precise(4));
  SDValue Glue = Chain.getValue(1);
  Chain = DAG.getCALLSEQ_END(Chain, 0, 0, Glue, DL);
  Glue = Chain.getValue(1);
  return DAG.getCopyFromReg(Chain, DL, SH::R0, MVT::i32, Glue);
}

SDValue SHTargetLowering::LowerGlobalTLSAddress(SDValue Op,
                                                SelectionDAG &DAG) const {
  const auto *Global = cast<GlobalAddressSDNode>(Op);
  const GlobalValue *GV = Global->getGlobal();
  const SDLoc DL(Op);
  if (GV->getAddressSpace() != 0)
    report_fatal_error("SH TLS only supports address space zero");
  if (getTargetMachine().useEmulatedTLS())
    report_fatal_error("SH emulated TLS is not supported");
  if (getTargetMachine().useTLSDESC())
    report_fatal_error("SH TLSDESC is not supported");

  int64_t Addend = Global->getOffset();
  if (!isInt<32>(Addend))
    report_fatal_error("SH TLS address addend must fit signed 32 bits");
  switch (getTargetMachine().getTLSModel(GV)) {
  case TLSModel::GeneralDynamic: {
    SDValue Address = lowerSHTLSResolverCall(
        GV, SHConstantPoolValue::Modifier::TLSGD, DL, DAG);
    return addSHTLSAddressOffset(Address, Addend, DL, DAG);
  }
  case TLSModel::LocalDynamic: {
    SDValue Base = lowerSHTLSResolverCall(
        GV, SHConstantPoolValue::Modifier::TLSLDM, DL, DAG);
    SHConstantPoolValue *Offset =
        SHConstantPoolValue::create(GV, static_cast<int32_t>(Addend),
                                    SHConstantPoolValue::Modifier::DTPOFF);
    return DAG.getNode(ISD::ADD, DL, MVT::i32, Base,
                       lowerSHSymbolAddress(Offset, DL, DAG));
  }
  case TLSModel::InitialExec: {
    if (Subtarget.isFDPIC()) {
      SHConstantPoolValue *Offset = SHConstantPoolValue::create(
          GV, 0, SHConstantPoolValue::Modifier::GOTTPOFF);
      SDValue GOTOffset = lowerSHSymbolAddress(Offset, DL, DAG);
      SDValue Slot = DAG.getNode(ISD::ADD, DL, MVT::i32,
                                 getSHFDPICBase(DL, DAG), GOTOffset);
      MachineFunction &MF = DAG.getMachineFunction();
      SDValue TLSOffset = DAG.getLoad(MVT::i32, DL, DAG.getEntryNode(), Slot,
                                      MachinePointerInfo::getGOT(MF), Align(4),
                                      MachineMemOperand::MOLoad |
                                          MachineMemOperand::MODereferenceable |
                                          MachineMemOperand::MOInvariant);
      SDValue ThreadPointer = DAG.getNode(SHISD::THREAD_POINTER, DL, MVT::i32);
      return addSHTLSAddressOffset(
          DAG.getNode(ISD::ADD, DL, MVT::i32, ThreadPointer, TLSOffset), Addend,
          DL, DAG);
    }
    requireSHPICBase(DAG);
    MachineFunction &MF = DAG.getMachineFunction();
    SHConstantPoolValue *Offset = SHConstantPoolValue::create(
        GV, 0, SHConstantPoolValue::Modifier::GOTTPOFF);
    SDValue CPAddr = DAG.getTargetConstantPool(Offset, MVT::i32, Align(4));
    SDValue Ops[] = {DAG.getEntryNode(), CPAddr};
    MachineMemOperand::Flags Flags = MachineMemOperand::MOLoad |
                                     MachineMemOperand::MODereferenceable |
                                     MachineMemOperand::MOInvariant;
    SDValue Address = DAG.getMemIntrinsicNode(
        SHISD::TLS_IE, DL, DAG.getVTList(MVT::i32, MVT::Other), Ops, MVT::i32,
        MachinePointerInfo::getGOT(MF), Align(4), Flags,
        LocationSize::precise(4));
    return addSHTLSAddressOffset(Address, Addend, DL, DAG);
  }
  case TLSModel::LocalExec: {
    SDValue ThreadPointer = DAG.getNode(SHISD::THREAD_POINTER, DL, MVT::i32);
    SHConstantPoolValue *Offset = SHConstantPoolValue::create(
        GV, static_cast<int32_t>(Addend), SHConstantPoolValue::Modifier::TPOFF);
    return DAG.getNode(ISD::ADD, DL, MVT::i32, ThreadPointer,
                       lowerSHSymbolAddress(Offset, DL, DAG));
  }
  }
  llvm_unreachable("unexpected SH TLS model");
}

SDValue SHTargetLowering::LowerBlockAddress(SDValue Op,
                                            SelectionDAG &DAG) const {
  const auto *Block = cast<BlockAddressSDNode>(Op);
  int64_t Addend = Block->getOffset();
  if (!isInt<32>(Addend))
    report_fatal_error("SH block address addend must fit signed 32 bits");
  SHConstantPoolValue *CPV = SHConstantPoolValue::create(
      Block->getBlockAddress(), static_cast<int32_t>(Addend),
      getTargetMachine().isPositionIndependent()
          ? (Subtarget.isFDPIC() ? SHConstantPoolValue::Modifier::PCREL
                                 : SHConstantPoolValue::Modifier::GOTOFF)
          : SHConstantPoolValue::Modifier::None);
  return getTargetMachine().isPositionIndependent()
             ? (Subtarget.isFDPIC()
                    ? lowerSHPICPairedAddress(CPV, SDLoc(Op), DAG)
                    : lowerSHPICOffsetAddress(CPV, SDLoc(Op), DAG))
             : lowerSHSymbolAddress(CPV, SDLoc(Op), DAG);
}

SDValue SHTargetLowering::LowerJumpTable(SDValue Op, SelectionDAG &DAG) const {
  const auto *Table = cast<JumpTableSDNode>(Op);
  if (Table->getIndex() < 0)
    report_fatal_error("SH jump-table index must be nonnegative");
  SHConstantPoolValue *CPV = SHConstantPoolValue::create(
      *DAG.getContext(), static_cast<unsigned>(Table->getIndex()), 0,
      getTargetMachine().isPositionIndependent()
          ? (Subtarget.isFDPIC() ? SHConstantPoolValue::Modifier::PCREL
                                 : SHConstantPoolValue::Modifier::GOTOFF)
          : SHConstantPoolValue::Modifier::None);
  return getTargetMachine().isPositionIndependent()
             ? (Subtarget.isFDPIC()
                    ? lowerSHPICPairedAddress(CPV, SDLoc(Op), DAG)
                    : lowerSHPICOffsetAddress(CPV, SDLoc(Op), DAG))
             : lowerSHSymbolAddress(CPV, SDLoc(Op), DAG);
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
  MachineJumpTableInfo::JTEntryKind ExpectedKind =
      static_cast<MachineJumpTableInfo::JTEntryKind>(getJumpTableEncoding());
  if (MJTI->getEntryKind() != ExpectedKind) {
    if (ExpectedKind == MachineJumpTableInfo::EK_Inline)
      reportFatalUsageError(
          "SH2 jump-table branch requires inline relative entries");
    reportFatalUsageError(
        getTargetMachine().isPositionIndependent()
            ? "SH PIC requires relative jump-table entries"
            : "SH static code requires absolute block-address jump tables");
  }
  if (ExpectedKind != MachineJumpTableInfo::EK_Inline &&
      (MJTI->getEntrySize(DAG.getDataLayout()) != 4 ||
       MJTI->getEntryAlignment(DAG.getDataLayout()) != 4))
    report_fatal_error("SH jump-table entries must be four-byte words");

  SDValue TargetJT = DAG.getTargetJumpTable(JT->getIndex(), MVT::i32);
  SDValue DispatchBase =
      ExpectedKind == MachineJumpTableInfo::EK_Inline ? Index : Table;
  SDValue Ops[] = {Chain, Index, DispatchBase, TargetJT};
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

// Any legal pointer value can be materialized in a GPR for register-indirect
// addressing. Recognizing a foldable address shape is instruction selection's
// responsibility, not a restriction on which producers memory operations
// accept.
static bool isLegalSHAddress(SDValue Addr) {
  return Addr.getValueType() == MVT::i32;
}

SDValue SHTargetLowering::LowerDIVREM(SDValue Op, SelectionDAG &DAG) const {
  bool IsSigned = Op.getOpcode() == ISD::SDIVREM;
  bool QuotientUsed = Op->hasAnyUseOfValue(0);
  bool RemainderUsed = Op->hasAnyUseOfValue(1);
  SDLoc DL(Op);
  SDValue Dividend = Op.getOperand(0);
  SDValue Divisor = Op.getOperand(1);
  const SDNodeFlags Flags = Op->getFlags();
  const Function &F = DAG.getMachineFunction().getFunction();
  if (Subtarget.hasSH2A()) {
    SDValue Quotient = DAG.getNode(IsSigned ? ISD::SDIV : ISD::UDIV, DL,
                                   MVT::i32, Dividend, Divisor, Flags);
    SDValue Remainder =
        DAG.getNode(ISD::SUB, DL, MVT::i32, Dividend,
                    DAG.getNode(ISD::MUL, DL, MVT::i32, Quotient, Divisor));
    if (!QuotientUsed)
      Quotient = DAG.getUNDEF(MVT::i32);
    if (!RemainderUsed)
      Remainder = DAG.getUNDEF(MVT::i32);
    return DAG.getMergeValues({Quotient, Remainder}, DL);
  }
  bool UseLibcall = !Subtarget.hasSH2() ||
                    F.hasFnAttribute(Attribute::OptimizeForSize) ||
                    F.hasFnAttribute(Attribute::MinSize);

  if (!UseLibcall && Subtarget.getSchedModel().IssueWidth == 2 &&
      !F.hasOptNone() &&
      DAG.getMachineFunction().getTarget().getOptLevel() !=
          CodeGenOptLevel::None &&
      !isa<ConstantSDNode>(Divisor) && isKnownPositiveWord(Divisor, DAG)) {
    SDValue Magnitude = Dividend;
    SDValue Sign;
    if (IsSigned) {
      Sign = DAG.getNode(ISD::SRA, DL, MVT::i32, Dividend,
                         DAG.getConstant(31, DL, MVT::i32));
      Magnitude = DAG.getNode(
          ISD::SUB, DL, MVT::i32,
          DAG.getNode(ISD::XOR, DL, MVT::i32, Dividend, Sign), Sign);
    }
    SDValue Quotient =
        DAG.getNode(SHISD::UDIV_NARROW, DL, MVT::i32, Magnitude, Divisor);
    if (IsSigned)
      Quotient = DAG.getNode(
          ISD::SUB, DL, MVT::i32,
          DAG.getNode(ISD::XOR, DL, MVT::i32, Quotient, Sign), Sign);
    SDValue Remainder = DAG.getUNDEF(MVT::i32);
    if (RemainderUsed)
      Remainder =
          DAG.getNode(ISD::SUB, DL, MVT::i32, Dividend,
                      DAG.getNode(ISD::MUL, DL, MVT::i32, Quotient, Divisor));
    if (!QuotientUsed)
      Quotient = DAG.getUNDEF(MVT::i32);
    return DAG.getMergeValues({Quotient, Remainder}, DL);
  }

  if (!UseLibcall && !IsSigned && Subtarget.getSchedModel().IssueWidth == 1 &&
      !F.hasOptNone() &&
      DAG.getMachineFunction().getTarget().getOptLevel() !=
          CodeGenOptLevel::None &&
      !isa<ConstantSDNode>(Divisor) && isKnownUnsignedWord(Divisor, DAG)) {
    // The unsigned helper groups DIV1 work for word-sized divisors. On
    // single-issue cores this saves the inline sequence's interleaved
    // rotates without giving up dual-issue opportunities.
    MakeLibCallOptions Options;
    Options.setIsSigned(false).setIsPostTypeLegalization();
    SDValue Quotient = makeLibCall(DAG, RTLIB::UDIV_I32, MVT::i32,
                                   {Dividend, Divisor}, Options, DL)
                           .first;
    SDValue Remainder = DAG.getUNDEF(MVT::i32);
    if (RemainderUsed)
      Remainder =
          DAG.getNode(ISD::SUB, DL, MVT::i32, Dividend,
                      DAG.getNode(ISD::MUL, DL, MVT::i32, Quotient, Divisor));
    if (!QuotientUsed)
      Quotient = DAG.getUNDEF(MVT::i32);
    return DAG.getMergeValues({Quotient, Remainder}, DL);
  }

  if (!Subtarget.hasSH2()) {
    MakeLibCallOptions Options;
    Options.setIsSigned(IsSigned).setIsPostTypeLegalization();
    RTLIB::Libcall Libcall = IsSigned ? RTLIB::SDIV_I32 : RTLIB::UDIV_I32;
    SDValue Quotient =
        makeLibCall(DAG, Libcall, MVT::i32, {Dividend, Divisor}, Options, DL)
            .first;
    SDValue Undef = DAG.getUNDEF(MVT::i32);
    if (!RemainderUsed)
      return DAG.getMergeValues({Quotient, Undef}, DL);
    SDValue Product = DAG.getNode(ISD::MUL, DL, MVT::i32, Quotient, Divisor);
    SDValue Remainder = DAG.getNode(ISD::SUB, DL, MVT::i32, Dividend, Product);
    if (!QuotientUsed)
      Quotient = Undef;
    return DAG.getMergeValues({Quotient, Remainder}, DL);
  }

  if (UseLibcall) {
    if (QuotientUsed && RemainderUsed) {
      RTLIB::Libcall Libcall =
          IsSigned ? RTLIB::SDIVREM_I32 : RTLIB::UDIVREM_I32;
      RTLIB::LibcallImpl LibcallImpl = getLibcallImpl(Libcall);
      assert(LibcallImpl != RTLIB::Unsupported &&
             "SH divrem libcall must be available");
      Type *RetTy = Type::getInt32Ty(*DAG.getContext());
      ArgListTy Args;
      for (SDValue Value : {Dividend, Divisor}) {
        ArgListEntry Entry(Value, RetTy);
        Entry.IsSExt = IsSigned;
        Entry.IsZExt = !IsSigned;
        Args.push_back(Entry);
      }
      SDValue RemainderAddress = DAG.CreateStackTemporary(MVT::i32);
      Args.emplace_back(RemainderAddress,
                        PointerType::getUnqual(*DAG.getContext()));
      SDValue Callee =
          DAG.getExternalSymbol(LibcallImpl, getPointerTy(DAG.getDataLayout()));
      CallLoweringInfo CLI(DAG);
      CLI.setDebugLoc(DL)
          .setChain(DAG.getEntryNode())
          .setLibCallee(getLibcallImplCallingConv(LibcallImpl), RetTy, Callee,
                        std::move(Args))
          .setSExtResult(IsSigned)
          .setZExtResult(!IsSigned);
      auto [Quotient, CallChain] = LowerCallTo(CLI);
      int FrameIndex = cast<FrameIndexSDNode>(RemainderAddress)->getIndex();
      MachinePointerInfo PtrInfo = MachinePointerInfo::getFixedStack(
          DAG.getMachineFunction(), FrameIndex);
      SDValue Remainder =
          DAG.getLoad(MVT::i32, DL, CallChain, RemainderAddress, PtrInfo);
      return DAG.getMergeValues({Quotient, Remainder}, DL);
    }

    MakeLibCallOptions Options;
    Options.setIsSigned(IsSigned).setIsPostTypeLegalization();
    SDValue Undef = DAG.getUNDEF(MVT::i32);
    SDValue Quotient = Undef;
    SDValue Remainder = Undef;
    SDValue Chain;
    if (QuotientUsed) {
      RTLIB::Libcall Libcall = IsSigned ? RTLIB::SDIV_I32 : RTLIB::UDIV_I32;
      std::tie(Quotient, Chain) =
          makeLibCall(DAG, Libcall, MVT::i32, {Dividend, Divisor}, Options, DL);
    }
    if (RemainderUsed) {
      RTLIB::Libcall Libcall = IsSigned ? RTLIB::SREM_I32 : RTLIB::UREM_I32;
      std::tie(Remainder, Chain) = makeLibCall(
          DAG, Libcall, MVT::i32, {Dividend, Divisor}, Options, DL, Chain);
    }
    return DAG.getMergeValues({Quotient, Remainder}, DL);
  }

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

static SDValue lowerSHVolatileUnalignedIntegerLoad(SDValue Op,
                                                   SelectionDAG &DAG) {
  const auto *Load = cast<LoadSDNode>(Op);
  unsigned Width = Load->getMemoryVT().getStoreSize();
  if ((Width != 2 && Width != 4) || Load->getAlign() >= Align(Width))
    report_fatal_error("SH invalid unaligned integer load");
  if (Load->getBasePtr().getValueType() != MVT::i32)
    report_fatal_error("SH unaligned integer load address is not supported");

  SDLoc DL(Op);
  SDValue Result = DAG.getConstant(0, DL, MVT::i32);
  SDValue LoadChain = Load->getChain();
  constexpr unsigned PieceBytes = 1;
  for (unsigned Byte = 0; Byte != Width; Byte += PieceBytes) {
    SDValue Address = DAG.getObjectPtrOffset(DL, Load->getBasePtr(),
                                             TypeSize::getFixed(Byte));
    unsigned Shift =
        8 * (DAG.getDataLayout().isLittleEndian() ? Byte
                                                  : Width - PieceBytes - Byte);
    SDValue Loaded = DAG.getExtLoad(
        ISD::ZEXTLOAD, DL, MVT::i32, LoadChain, Address,
        Load->getPointerInfo().getWithOffset(Byte), MVT::i8, Align(1),
        Load->getMemOperand()->getFlags(), Load->getAAInfo());
    SDValue Piece = Loaded;
    if (Shift != 0)
      Piece = DAG.getNode(ISD::SHL, DL, MVT::i32, Piece,
                          DAG.getConstant(Shift, DL, MVT::i32));
    Result = DAG.getNode(ISD::OR, DL, MVT::i32, Result, Piece);
    LoadChain = Loaded.getValue(1);
  }
  if (Load->getExtensionType() == ISD::SEXTLOAD)
    Result = DAG.getNode(ISD::SIGN_EXTEND_INREG, DL, MVT::i32, Result,
                         DAG.getValueType(Width == 2 ? MVT::i16 : MVT::i32));
  return DAG.getMergeValues({Result, LoadChain}, DL);
}

static SDValue lowerSHVolatileUnalignedIntegerStore(SDValue Op,
                                                    SelectionDAG &DAG) {
  const auto *Store = cast<StoreSDNode>(Op);
  unsigned Width = Store->getMemoryVT().getStoreSize();
  if ((Width != 2 && Width != 4) || Store->getAlign() >= Align(Width))
    report_fatal_error("SH invalid unaligned integer store");
  if (Store->getBasePtr().getValueType() != MVT::i32)
    report_fatal_error("SH unaligned integer store address is not supported");

  SDLoc DL(Op);
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
    StoreChain = ByteStore;
  }
  return StoreChain;
}

static SDValue lowerSHVolatileI64Load(SDValue Op, SelectionDAG &DAG) {
  const auto *Load = cast<LoadSDNode>(Op);
  assert(Load->isVolatile() && "ordinary loads use native legalization");
  if (!isLegalSHAddress(Load->getBasePtr()))
    report_fatal_error("SH i64 load address must be a 32-bit pointer");

  SDLoc DL(Op);
  SDValue Chain = Load->getChain();
  SDValue Base = Load->getBasePtr();
  SDValue Other = DAG.getObjectPtrOffset(DL, Base, TypeSize::getFixed(4));
  SDValue First = DAG.getLoad(
      MVT::i32, DL, Chain, Base, Load->getPointerInfo(), Load->getBaseAlign(),
      Load->getMemOperand()->getFlags(), Load->getAAInfo());
  SDValue Second = DAG.getLoad(
      MVT::i32, DL, First.getValue(1), Other, Load->getPointerInfo().getWithOffset(4),
      commonAlignment(Load->getBaseAlign(), 4),
      Load->getMemOperand()->getFlags(), Load->getAAInfo());


  SDValue Lo = DAG.getDataLayout().isLittleEndian() ? First : Second;
  SDValue Hi = DAG.getDataLayout().isLittleEndian() ? Second : First;
  SDValue Value = DAG.getNode(ISD::BUILD_PAIR, DL, MVT::i64, Lo, Hi);
  return DAG.getMergeValues({Value, Second.getValue(1)}, DL);
}

static SDValue lowerSHVolatileI64Store(SDValue Op, SelectionDAG &DAG) {
  const auto *Store = cast<StoreSDNode>(Op);
  assert(Store->isVolatile() && "ordinary stores use native legalization");
  if (!isLegalSHAddress(Store->getBasePtr()))
    report_fatal_error("SH i64 store address must be a 32-bit pointer");

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
  SDValue Second =
      DAG.getStore(First, DL, SecondValue, Other,
                   Store->getPointerInfo().getWithOffset(4),
                   commonAlignment(Store->getBaseAlign(), 4),
                   Store->getMemOperand()->getFlags(), Store->getAAInfo());
  return Second;
}

static SDValue lowerSHF64Load(SDValue Op, SelectionDAG &DAG) {
  const auto *Load = cast<LoadSDNode>(Op);
  if (Load->getBasePtr().getOpcode() == ISD::ConstantPool ||
      Load->getBasePtr().getOpcode() == ISD::TargetConstantPool)
    return Op;
  if (!isLegalSHAddress(Load->getBasePtr()))
    report_fatal_error("SH f64 load address must be a 32-bit pointer");
  if (Load->getAlign() >= Align(4))
    return Op;

  SDLoc DL(Op);
  SDValue Chain = Load->getChain();
  SDValue Base = Load->getBasePtr();
  SDValue Other = DAG.getObjectPtrOffset(DL, Base, TypeSize::getFixed(4));
  SDValue First = DAG.getLoad(
      MVT::i32, DL, Chain, Base, Load->getPointerInfo(), Load->getBaseAlign(),
      Load->getMemOperand()->getFlags(), Load->getAAInfo());
  bool IsOrdered =
      Load->getAlign() < Align(4) || Load->getMemOperand()->isVolatile();
  SDValue SecondChain = IsOrdered ? First.getValue(1) : Chain;
  SDValue Second = DAG.getLoad(
      MVT::i32, DL, SecondChain, Other, Load->getPointerInfo().getWithOffset(4),
      commonAlignment(Load->getBaseAlign(), 4),
      Load->getMemOperand()->getFlags(), Load->getAAInfo());
  SDValue ResultChain =
      IsOrdered ? Second.getValue(1)
                : DAG.getNode(ISD::TokenFactor, DL, MVT::Other,
                              First.getValue(1), Second.getValue(1));
  SDValue MSW = DAG.getDataLayout().isBigEndian() ? First : Second;
  SDValue LSW = DAG.getDataLayout().isBigEndian() ? Second : First;
  SDValue Value = DAG.getNode(SHISD::GPR_PAIR_TO_DR, DL, MVT::f64, MSW, LSW);
  return DAG.getMergeValues({Value, ResultChain}, DL);
}

static SDValue lowerSHF64Store(SDValue Op, SelectionDAG &DAG) {
  const auto *Store = cast<StoreSDNode>(Op);
  if (!isLegalSHAddress(Store->getBasePtr()))
    report_fatal_error("SH f64 store address must be a 32-bit pointer");
  if (Store->getAlign() >= Align(4))
    return Op;

  SDLoc DL(Op);
  SDValue Pair =
      DAG.getNode(SHISD::DR_TO_GPR_PAIR, DL, DAG.getVTList(MVT::i32, MVT::i32),
                  Store->getValue());
  SDValue MSW = Pair.getValue(0);
  SDValue LSW = Pair.getValue(1);
  SDValue FirstValue = DAG.getDataLayout().isBigEndian() ? MSW : LSW;
  SDValue SecondValue = DAG.getDataLayout().isBigEndian() ? LSW : MSW;
  SDValue Chain = Store->getChain();
  SDValue Base = Store->getBasePtr();
  SDValue Other = DAG.getObjectPtrOffset(DL, Base, TypeSize::getFixed(4));
  SDValue First =
      DAG.getStore(Chain, DL, FirstValue, Base, Store->getPointerInfo(),
                   Store->getBaseAlign(), Store->getMemOperand()->getFlags(),
                   Store->getAAInfo());
  bool IsOrdered =
      Store->getAlign() < Align(4) || Store->getMemOperand()->isVolatile();
  SDValue SecondChain = IsOrdered ? First : Chain;
  SDValue Second =
      DAG.getStore(SecondChain, DL, SecondValue, Other,
                   Store->getPointerInfo().getWithOffset(4),
                   commonAlignment(Store->getBaseAlign(), 4),
                   Store->getMemOperand()->getFlags(), Store->getAAInfo());
  if (IsOrdered)
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

static SDValue lowerSHVariableShift(const SHTargetLowering &TLI, SDValue Op,
                                    SelectionDAG &DAG);

static SDValue lowerSHRotate(const SHTargetLowering &TLI, SDValue Op,
                             SelectionDAG &DAG) {
  SDLoc DL(Op);
  SDValue Value = Op.getOperand(0);
  SDValue Count = Op.getOperand(1);
  bool IsLeft = Op.getOpcode() == ISD::ROTL;
  const auto *ConstantCount = dyn_cast<ConstantSDNode>(Count);
  const SHSubtarget &Subtarget =
      DAG.getMachineFunction().getSubtarget<SHSubtarget>();
  const auto *ConstantValue = dyn_cast<ConstantSDNode>(Value);
  if (!ConstantCount) {
    // Rotates use modulo counts, while SHLD interprets the sign of its count.
    // Normalize before forming the positive and negative shift directions.
    Count = DAG.getNode(ISD::AND, DL, MVT::i32, Count,
                        DAG.getConstant(31, DL, MVT::i32));
    if (IsLeft && ConstantValue && ConstantValue->getSExtValue() == -2) {
      unsigned ShiftOpcode = ISD::SHL;
      if (Subtarget.hasDynamicShift())
        ShiftOpcode = SHISD::SHL;
      SDValue Shift = DAG.getNode(ShiftOpcode, DL, MVT::i32,
                                  DAG.getConstant(1, DL, MVT::i32), Count);
      if (!Subtarget.hasDynamicShift())
        Shift = lowerSHVariableShift(TLI, Shift, DAG);
      return DAG.getNOT(DL, Shift, MVT::i32);
    }
    const Function &F = DAG.getMachineFunction().getFunction();
    bool OptimizeNone = DAG.getMachineFunction().getTarget().getOptLevel() ==
                        CodeGenOptLevel::None;
    APInt MaxCount = DAG.computeKnownBits(Count).getMaxValue();
    // Four bit tests execute fewer instructions than the counted one-bit loop
    // for the recurring 0..15 range.  Its larger static footprint is suitable
    // only for performance-oriented SH2 code.
    if (Subtarget.hasSH2() && !Subtarget.hasDynamicShift() && !OptimizeNone &&
        !F.hasOptNone() && !F.hasOptSize() && MaxCount == 15)
      return DAG.getNode(IsLeft ? SHISD::ROTL_SHORT : SHISD::ROTR_SHORT, DL,
                         MVT::i32, Value, Count);
    // Two shift helpers beat the linear rotate loop only when the count can
    // reach its longer half.  Keep short known ranges self-contained.
    bool MayNeedLongRotate = MaxCount.uge(16);
    if (!Subtarget.hasDynamicShift() && !OptimizeNone && !F.hasOptNone() &&
        !F.hasMinSize() && MayNeedLongRotate) {
      SDValue OppositeCount = DAG.getNode(
          ISD::SUB, DL, MVT::i32, DAG.getConstant(0, DL, MVT::i32), Count);
      OppositeCount = DAG.getNode(ISD::AND, DL, MVT::i32, OppositeCount,
                                  DAG.getConstant(31, DL, MVT::i32));
      SDValue LeftCount = IsLeft ? Count : OppositeCount;
      SDValue RightCount = IsLeft ? OppositeCount : Count;
      SDValue Left = DAG.getNode(ISD::SHL, DL, MVT::i32, Value, LeftCount);
      SDValue Right = DAG.getNode(ISD::SRL, DL, MVT::i32, Value, RightCount);
      return DAG.getNode(ISD::OR, DL, MVT::i32, Left, Right);
    }
    return DAG.getNode(IsLeft ? SHISD::ROTL : SHISD::ROTR, DL, MVT::i32, Value,
                       Count);
  }

  unsigned Amount = ConstantCount->getZExtValue() & 31;
  if (Amount == 0)
    return Value;
  if (Amount > 16) {
    Amount = 32 - Amount;
    IsLeft = !IsLeft;
  }

  unsigned DirectCost = Amount;
  unsigned SwapCost = 1 + 16 - Amount;
  unsigned LeftAmount = IsLeft ? Amount : 32 - Amount;
  unsigned RightAmount = 32 - LeftAmount;
  // Every SH CPU has a one-word swap.w rotate by 16. Compare the single-chain
  // rotate forms against the centralized constant-shift plan, preferring the
  // swap path on ties so the split form does not keep two values live.
  unsigned SplitCost = SH::planConstantShift(SH::SHLri, LeftAmount).size() +
                       SH::planConstantShift(SH::SRLri, RightAmount).size() + 1;
  const MachineFunction &MF = DAG.getMachineFunction();
  const Function &F = MF.getFunction();
  if (MF.getTarget().getOptLevel() != CodeGenOptLevel::None &&
      !F.hasOptNone() && !F.hasOptSize() && !F.hasMinSize()) {
    // Inspect the original value before creating either composition branch.
    // A tied shift needs a preservation copy when the source has other uses;
    // the untied swap.w already produces an independent temporary.
    unsigned PreserveCost = !Value.hasOneUse();
    unsigned BestCost = DirectCost < SwapCost && DirectCost < SplitCost
                            ? DirectCost + PreserveCost
                        : SwapCost <= SplitCost ? SwapCost
                                                : SplitCost + 1 + PreserveCost;
    unsigned ByteBase = 0;
    int ByteCorrection = 0;
    for (unsigned Base : {8u, 24u}) {
      int Correction = int(LeftAmount) - int(Base);
      if (Correction < -16)
        Correction += 32;
      if (Correction > 16)
        Correction -= 32;
      unsigned Steps = Correction < 0 ? -Correction : Correction;
      if (Steps > 4)
        continue;
      // swap.w, two eight-bit shifts and OR; neither a byte rotate nor
      // the copy needed to preserve a live source is free.
      unsigned Cost = 4 + Steps + PreserveCost;
      if (Cost < BestCost) {
        BestCost = Cost;
        ByteBase = Base;
        ByteCorrection = Correction;
      }
    }
    if (ByteBase) {
      // rotl8(x) = (x << 8) | (rotl16(x) >> 8). The overlapping
      // bits agree. Reverse the shifts for rotl24; keep both values
      // distinct so ordinary allocation preserves every source use.
      SDValue Swapped = DAG.getNode(SHISD::ROTL16, DL, MVT::i32, Value);
      SDValue Eight = DAG.getConstant(8, DL, MVT::i32);
      SDValue Shifted = DAG.getNode(ByteBase == 8 ? ISD::SHL : ISD::SRL, DL,
                                    MVT::i32, Value, Eight);
      Swapped = DAG.getNode(ByteBase == 8 ? ISD::SRL : ISD::SHL, DL, MVT::i32,
                            Swapped, Eight);
      Value = DAG.getNode(ISD::OR, DL, MVT::i32, Shifted, Swapped);
      unsigned Opcode = ByteCorrection < 0 ? SHISD::ROTR1 : SHISD::ROTL1;
      unsigned Steps = ByteCorrection < 0 ? -ByteCorrection : ByteCorrection;
      for (unsigned I = 0; I != Steps; ++I)
        Value = DAG.getNode(Opcode, DL, MVT::i32, Value);
      return Value;
    }
  }
  if (DirectCost < SwapCost && DirectCost < SplitCost) {
    unsigned Opcode = IsLeft ? SHISD::ROTL1 : SHISD::ROTR1;
    for (unsigned I = 0; I != Amount; ++I)
      Value = DAG.getNode(Opcode, DL, MVT::i32, Value);
    return Value;
  }
  if (SwapCost <= SplitCost) {
    Value = DAG.getNode(SHISD::ROTL16, DL, MVT::i32, Value);
    unsigned Opcode = IsLeft ? SHISD::ROTR1 : SHISD::ROTL1;
    for (unsigned I = Amount; I != 16; ++I)
      Value = DAG.getNode(Opcode, DL, MVT::i32, Value);
    return Value;
  }

  SDValue Left = DAG.getNode(ISD::SHL, DL, MVT::i32, Value,
                             DAG.getConstant(LeftAmount, DL, MVT::i32));
  SDValue Right = DAG.getNode(ISD::SRL, DL, MVT::i32, Value,
                              DAG.getConstant(RightAmount, DL, MVT::i32));
  return DAG.getNode(ISD::OR, DL, MVT::i32, Left, Right);
}

static SDValue lowerSHVariableShift(const SHTargetLowering &TLI, SDValue Op,
                                    SelectionDAG &DAG) {
  if (isa<ConstantSDNode>(Op.getOperand(1)))
    return Op;
  const Function &F = DAG.getMachineFunction().getFunction();
  // The runtime helpers are substantially faster on CPUs without dynamic
  // shifts and amortize well at -Os.  Keep the self-contained loop at -O0 and
  // -Oz, where avoiding call setup for an isolated shift is more important.
  if (F.hasOptNone() || F.hasMinSize())
    return Op;

  RTLIB::Libcall Libcall;
  switch (Op.getOpcode()) {
  default:
    llvm_unreachable("unexpected SH variable shift");
  case ISD::SHL:
    Libcall = RTLIB::SHL_I32;
    break;
  case ISD::SRL:
    Libcall = RTLIB::SRL_I32;
    break;
  case ISD::SRA:
    Libcall = RTLIB::SRA_I32;
    break;
  }

  Type *I32Ty = Type::getInt32Ty(*DAG.getContext());
  TargetLowering::ArgListTy Args;
  Args.emplace_back(Op.getOperand(0), I32Ty);
  Args.emplace_back(Op.getOperand(1), I32Ty);
  SDValue Callee = DAG.getExternalSymbol(TLI.getLibcallName(Libcall),
                                         TLI.getPointerTy(DAG.getDataLayout()));
  TargetLowering::CallLoweringInfo CLI(DAG);
  CLI.setDebugLoc(SDLoc(Op))
      .setChain(DAG.getEntryNode())
      .setLibCallee(CallingConv::C, I32Ty, Callee, std::move(Args))
      .setIsPostTypeLegalization();
  return TLI.LowerCallTo(CLI).first;
}

static SDValue lowerSHOverflow(SDValue Op, SelectionDAG &DAG) {
  SDLoc DL(Op);
  SDValue Arithmetic;
  switch (Op.getOpcode()) {
  default:
    llvm_unreachable("unexpected SH overflow node");
  case ISD::UADDO:
    Arithmetic = DAG.getNode(ISD::ADDC, DL, DAG.getVTList(MVT::i32, MVT::Glue),
                             Op.getOperand(0), Op.getOperand(1));
    break;
  case ISD::USUBO:
    Arithmetic = DAG.getNode(ISD::SUBC, DL, DAG.getVTList(MVT::i32, MVT::Glue),
                             Op.getOperand(0), Op.getOperand(1));
    break;
  case ISD::SADDO:
    Arithmetic =
        DAG.getNode(SHISD::ADDV, DL, DAG.getVTList(MVT::i32, MVT::Glue),
                    Op.getOperand(0), Op.getOperand(1));
    break;
  case ISD::SSUBO:
    Arithmetic =
        DAG.getNode(SHISD::SUBV, DL, DAG.getVTList(MVT::i32, MVT::Glue),
                    Op.getOperand(0), Op.getOperand(1));
    break;
  }
  SDValue Overflow =
      DAG.getNode(SHISD::MOVT, DL, MVT::i32, Arithmetic.getValue(1));
  return DAG.getNode(ISD::MERGE_VALUES, DL, Op->getVTList(), Arithmetic,
                     Overflow);
}

static SDValue lowerSH1Multiply(const SHTargetLowering &TLI, SDValue Op,
                                SelectionDAG &DAG) {
  SDLoc DL(Op);
  Type *I32Ty = Type::getInt32Ty(*DAG.getContext());
  TargetLowering::ArgListTy Args;
  Args.emplace_back(Op.getOperand(0), I32Ty);
  Args.emplace_back(Op.getOperand(1), I32Ty);
  SDValue Callee = DAG.getExternalSymbol(TLI.getLibcallName(RTLIB::MUL_I32),
                                         TLI.getPointerTy(DAG.getDataLayout()));
  TargetLowering::CallLoweringInfo CLI(DAG);
  CLI.setDebugLoc(DL)
      .setChain(DAG.getEntryNode())
      .setLibCallee(CallingConv::C, I32Ty, Callee, std::move(Args))
      .setIsPostTypeLegalization();
  return TLI.LowerCallTo(CLI).first;
}

static SDValue lowerSH1MultiplyHalves(const SHTargetLowering &TLI, SDValue LHS,
                                      SDValue RHS, bool IsSigned, SDLoc DL,
                                      SelectionDAG &DAG) {
  SDValue Zero = DAG.getConstant(0, DL, MVT::i32);
  SDValue Shift = DAG.getConstant(31, DL, MVT::i32);
  SDValue LHSHigh =
      IsSigned ? DAG.getNode(ISD::SRA, DL, MVT::i32, LHS, Shift) : Zero;
  SDValue RHSHigh =
      IsSigned ? DAG.getNode(ISD::SRA, DL, MVT::i32, RHS, Shift) : Zero;
  SDValue WideLHS = DAG.getNode(ISD::BUILD_PAIR, DL, MVT::i64, LHS, LHSHigh);
  SDValue WideRHS = DAG.getNode(ISD::BUILD_PAIR, DL, MVT::i64, RHS, RHSHigh);
  Type *I64Ty = Type::getInt64Ty(*DAG.getContext());
  TargetLowering::ArgListTy Args;
  Args.emplace_back(WideLHS, I64Ty);
  Args.emplace_back(WideRHS, I64Ty);
  SDValue Callee = DAG.getExternalSymbol(TLI.getLibcallName(RTLIB::MUL_I64),
                                         TLI.getPointerTy(DAG.getDataLayout()));
  TargetLowering::CallLoweringInfo CLI(DAG);
  CLI.setDebugLoc(DL)
      .setChain(DAG.getEntryNode())
      .setLibCallee(CallingConv::C, I64Ty, Callee, std::move(Args))
      .setIsPostTypeLegalization();
  return TLI.LowerCallTo(CLI).first;
}

static SDValue lowerSHMultiplyHalves(const SHTargetLowering &TLI, SDValue LHS,
                                     SDValue RHS, bool IsSigned, bool HasSH2,
                                     SDLoc DL, SelectionDAG &DAG) {
  if (!HasSH2)
    return lowerSH1MultiplyHalves(TLI, LHS, RHS, IsSigned, DL, DAG);
  unsigned Opcode = IsSigned ? SHISD::SMUL_LOHI : SHISD::UMUL_LOHI;
  return DAG.getNode(Opcode, DL, DAG.getVTList(MVT::i32, MVT::i32), LHS, RHS);
}

static SDValue lowerSHMultiplyHigh(const SHTargetLowering &TLI, SDValue LHS,
                                   SDValue RHS, bool IsSigned, bool HasSH2,
                                   SDLoc DL, SelectionDAG &DAG) {
  if (!HasSH2)
    return lowerSH1MultiplyHalves(TLI, LHS, RHS, IsSigned, DL, DAG).getValue(1);
  unsigned Opcode =
      IsSigned ? SHISD::MUL_HIGH_SIGNED : SHISD::MUL_HIGH_UNSIGNED;
  return DAG.getNode(Opcode, DL, MVT::i32, LHS, RHS);
}

static SDValue lowerSHMultiplyOverflow(const SHTargetLowering &TLI, SDValue Op,
                                       bool HasSH2, SelectionDAG &DAG) {
  SDLoc DL(Op);
  bool IsSigned = Op.getOpcode() == ISD::SMULO;
  SDValue Halves = lowerSHMultiplyHalves(
      TLI, Op.getOperand(0), Op.getOperand(1), IsSigned, HasSH2, DL, DAG);
  SDValue Low = Halves;
  SDValue High = Halves.getValue(1);
  SDValue ExpectedHigh = DAG.getConstant(0, DL, MVT::i32);
  if (IsSigned)
    ExpectedHigh = DAG.getNode(ISD::SRA, DL, MVT::i32, Low,
                               DAG.getConstant(31, DL, MVT::i32));
  SDValue Overflow = DAG.getSetCC(DL, MVT::i32, High, ExpectedHigh, ISD::SETNE);
  return DAG.getNode(ISD::MERGE_VALUES, DL, Op->getVTList(), Low, Overflow);
}

void SHTargetLowering::ReplaceNodeResults(SDNode *N,
                                          SmallVectorImpl<SDValue> &Results,
                                          SelectionDAG &DAG) const {
  if (N->getOpcode() == ISD::VAARG) {
    SDValue Lowered = LowerVAARG(SDValue(N, 0), DAG);
    Results.push_back(Lowered);
    Results.push_back(Lowered.getValue(1));
    return;
  }
  if (N->getOpcode() != ISD::LOAD ||
      cast<LoadSDNode>(N)->getMemoryVT() != MVT::i64 ||
      !cast<LoadSDNode>(N)->isVolatile())
    return;
  SDValue Lowered = lowerSHVolatileI64Load(SDValue(N, 0), DAG);
  Results.push_back(Lowered);
  Results.push_back(Lowered.getValue(1));
}

static SDValue lowerSHFrameAddress(SDValue Op, SelectionDAG &DAG) {
  if (Op.getConstantOperandVal(0) != 0)
    report_fatal_error(
        "SH frame and return address intrinsics only support depth zero");
  MachineFunction &MF = DAG.getMachineFunction();
  MF.getFrameInfo().setFrameAddressIsTaken(true);
  return DAG.getCopyFromReg(DAG.getEntryNode(), SDLoc(Op), SH::R14,
                            Op.getValueType());
}

static SDValue lowerSHReturnAddress(SDValue Op, SelectionDAG &DAG) {
  if (Op.getConstantOperandVal(0) != 0)
    report_fatal_error(
        "SH frame and return address intrinsics only support depth zero");
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MFI.setReturnAddressIsTaken(true);
  SHMachineFunctionInfo *FuncInfo = MF.getInfo<SHMachineFunctionInfo>();
  if (!FuncInfo->hasReturnAddressFrameIndex())
    FuncInfo->setReturnAddressFrameIndex(createSHPRSpillFrameIndex(MF));
  int FrameIndex = FuncInfo->getReturnAddressFrameIndex();
  SDValue Address = DAG.getFrameIndex(FrameIndex, MVT::i32);
  return DAG.getLoad(MVT::i32, SDLoc(Op), DAG.getEntryNode(), Address,
                     MachinePointerInfo::getFixedStack(MF, FrameIndex),
                     Align(4));
}

SDValue SHTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const {
  if (Op.getOpcode() == ISD::EXTRACT_VECTOR_ELT ||
      Op.getOpcode() == ISD::INSERT_VECTOR_ELT) {
    bool Insert = Op.getOpcode() == ISD::INSERT_VECTOR_ELT;
    auto *Index = dyn_cast<ConstantSDNode>(Op.getOperand(Insert ? 2 : 1));
    if (!Index)
      return SDValue(); // Standard memory expansion handles variable indices.
    unsigned Lane = Index->getZExtValue();
    if (Lane >= 4)
      return DAG.getUNDEF(Op.getValueType());
    unsigned Sub = SH::sub_fpu32_0 + Lane;
    if (!Insert)
      return SDValue(
          DAG.getMachineNode(TargetOpcode::EXTRACT_SUBREG, SDLoc(Op), MVT::f32,
                             Op.getOperand(0),
                             DAG.getTargetConstant(Sub, SDLoc(Op), MVT::i32)),
          0);
    return SDValue(
        DAG.getMachineNode(TargetOpcode::INSERT_SUBREG, SDLoc(Op), MVT::v4f32,
                           Op.getOperand(0), Op.getOperand(1),
                           DAG.getTargetConstant(Sub, SDLoc(Op), MVT::i32)),
        0);
  }
  if (Op.getOpcode() == ISD::ConstantFP) {
    SDLoc DL(Op);
    const APInt &Bits =
        cast<ConstantFPSDNode>(Op)->getValueAPF().bitcastToAPInt();
    if (Op.getValueType() == MVT::f32) {
      if (Bits == APInt(32, 0))
        return DAG.getNode(SHISD::FCONST_0, DL, MVT::f32);
      if (Bits == APInt(32, UINT64_C(0x3f800000)))
        return DAG.getNode(SHISD::FCONST_1, DL, MVT::f32);
      MachineFunction &MF = DAG.getMachineFunction();
      const ConstantFP *Constant =
          cast<ConstantFPSDNode>(Op)->getConstantFPValue();
      SDValue Address = DAG.getConstantPool(Constant, MVT::i32, Align(4));
      return DAG.getLoad(MVT::f32, DL, DAG.getEntryNode(), Address,
                         MachinePointerInfo::getConstantPool(MF), Align(4),
                         MachineMemOperand::MOInvariant |
                             MachineMemOperand::MODereferenceable);
    }
    if (Op.getValueType() == MVT::f64) {
      uint64_t MSW = Bits.extractBits(32, 32).getZExtValue();
      uint64_t LSW = Bits.extractBits(32, 0).getZExtValue();
      MachineFunction &MF = DAG.getMachineFunction();
      bool OptimizeSize =
          MF.getFunction().hasOptSize() || MF.getFunction().hasMinSize();
      if (OptimizeSize && MSW == LSW) {
        SDValue Word = DAG.getConstant(MSW, DL, MVT::i32);
        return DAG.getNode(SHISD::GPR_PAIR_TO_DR, DL, MVT::f64, Word, Word);
      }
      const ConstantFP *Constant =
          cast<ConstantFPSDNode>(Op)->getConstantFPValue();
      bool Negate = false;
      if (OptimizeSize && Constant->getValueAPF().isNegative() &&
          Constant->getValueAPF().isFiniteNonZero()) {
        APFloat Magnitude = Constant->getValueAPF();
        Magnitude.changeSign();
        const ConstantFP *Positive =
            ConstantFP::get(*DAG.getContext(), Magnitude);
        // Sharing an existing magnitude saves an eight-byte pool entry.
        // Keep the sign operation visible to combining and allocation.
        Negate =
            llvm::any_of(MF.getFunction(), [Positive](const BasicBlock &BB) {
              return llvm::any_of(BB, [Positive](const Instruction &I) {
                return llvm::any_of(I.operands(), [Positive](const Use &U) {
                  return U.get() == Positive;
                });
              });
            });
        if (Negate)
          Constant = Positive;
      }
      SDValue Address = DAG.getConstantPool(Constant, MVT::i32, Align(4));
      SDValue Value =
          DAG.getLoad(MVT::f64, DL, DAG.getEntryNode(), Address,
                      MachinePointerInfo::getConstantPool(MF), Align(4),
                      MachineMemOperand::MOInvariant |
                          MachineMemOperand::MODereferenceable);
      return Negate ? DAG.getNode(ISD::FNEG, DL, MVT::f64, Value) : Value;
    }
    report_fatal_error("SH hardware floating constant has an unsupported type");
  }
  if (Op.getOpcode() == ISD::SINT_TO_FP) {
    if (Op.getOperand(0).getValueType() != MVT::i32)
      report_fatal_error("SH hardware signed floating conversion requires i32");
    if (Op.getValueType() == MVT::f32)
      return DAG.getNode(SHISD::I32_TO_F32, SDLoc(Op), MVT::f32,
                         Op.getOperand(0));
    if (Op.getValueType() == MVT::f64)
      return DAG.getNode(SHISD::I32_TO_F64, SDLoc(Op), MVT::f64,
                         Op.getOperand(0));
    report_fatal_error(
        "SH hardware signed floating conversion has an unsupported result");
  }
  if (Op.getOpcode() == ISD::FP_TO_SINT) {
    if (Op.getValueType() != MVT::i32)
      report_fatal_error(
          "SH hardware floating truncation requires an i32 result");
    if (Op.getOperand(0).getValueType() == MVT::f32)
      return DAG.getNode(SHISD::F32_TO_I32, SDLoc(Op), MVT::i32,
                         Op.getOperand(0));
    if (Op.getOperand(0).getValueType() == MVT::f64)
      return DAG.getNode(SHISD::F64_TO_I32, SDLoc(Op), MVT::i32,
                         Op.getOperand(0));
    report_fatal_error(
        "SH hardware floating truncation has an unsupported source");
  }
  if (Op.getOpcode() == ISD::FP_TO_UINT) {
    if (Op.getValueType() != MVT::i32)
      report_fatal_error(
          "SH hardware unsigned floating truncation requires an i32 result");
    SDLoc DL(Op);
    SDValue Source = Op.getOperand(0);
    EVT SourceVT = Source.getValueType();
    if (SourceVT != MVT::f32 && SourceVT != MVT::f64)
      report_fatal_error(
          "SH hardware unsigned floating truncation has an unsupported source");
    SDValue Threshold = DAG.getConstantFP(2147483648.0, DL, SourceVT);
    const MachineFunction &MF = DAG.getMachineFunction();
    const Function &F = MF.getFunction();
    bool BranchLocal = !F.hasOptSize() && !F.hasMinSize() && !F.hasOptNone() &&
                       !F.isStrictFP() &&
                       MF.getTarget().getOptLevel() != CodeGenOptLevel::None &&
                       Subtarget.getCPU() == "sh4" &&
                       Subtarget.getTuneCPU() == "sh4" &&
                       Subtarget.getABI() == SH::ABIKind::GNUHardFloat &&
                       !Subtarget.hasFPUSingleMode() &&
                       F.getFnAttribute("no-trapping-math").getValueAsBool();
    for (StringRef Name : {"denormal-fp-math", "denormal-fp-math-f32"}) {
      Attribute A = F.getFnAttribute(Name);
      BranchLocal &= !A.isValid() || A.getValueAsString() == "ieee,ieee";
    }
    for (StringRef Name : {"sh-interrupt-handler", "sh-nosave-low-regs",
                           "sh-resbank", "sh-sp-switch", "sh-trap-exit"})
      BranchLocal &= !F.hasFnAttribute(Name);
    if (BranchLocal) {
      for (const BasicBlock &BB : F) {
        for (const Instruction &I : BB) {
          const auto *Call = dyn_cast<CallBase>(&I);
          if (!Call)
            continue;
          if (Call->isInlineAsm() || isa<ConstrainedFPIntrinsic>(Call)) {
            BranchLocal = false;
            break;
          }
          const auto *Callee = dyn_cast<Function>(
              Call->getCalledOperand()->stripPointerCastsAndAliases());
          if (!Callee) {
            BranchLocal = false;
            break;
          }
          {
            StringRef Name = Callee->getName();
            if (Name.starts_with("llvm.get.fp") ||
                Name.starts_with("llvm.set.fp") ||
                Name.starts_with("llvm.reset.fp") ||
                Name == "llvm.get.rounding" || Name == "llvm.set.rounding" ||
                Name.starts_with("llvm.read_register") ||
                Name.starts_with("llvm.write_register") ||
                Name.starts_with("llvm.sh.") || Name == "fegetenv" ||
                Name == "fesetenv" || Name == "feupdateenv" ||
                Name == "feholdexcept" || Name == "fetestexcept" ||
                Name == "feclearexcept" || Name == "feraiseexcept" ||
                Name == "fegetexceptflag" || Name == "fesetexceptflag" ||
                Name == "fegetround" || Name == "fesetround" ||
                Name == "feenableexcept" || Name == "fedisableexcept" ||
                Name == "fegetexcept")
              BranchLocal = false;
          }
        }
        if (!BranchLocal)
          break;
      }
    }
    if (BranchLocal)
      return DAG.getNode(SourceVT == MVT::f32 ? SHISD::F32_TO_U32
                                              : SHISD::F64_TO_U32,
                         DL, MVT::i32, Source, Threshold);
    SDValue High = DAG.getSetCC(DL, MVT::i32, Source, Threshold, ISD::SETUGE);
    SDValue Adjusted = DAG.getNode(ISD::FSUB, DL, SourceVT, Source, Threshold);
    SDValue Convertible = DAG.getSelect(DL, SourceVT, High, Adjusted, Source);
    unsigned ConvertOpcode =
        SourceVT == MVT::f32 ? SHISD::F32_TO_I32 : SHISD::F64_TO_I32;
    SDValue Converted = DAG.getNode(ConvertOpcode, DL, MVT::i32, Convertible);
    SDValue HighBit = DAG.getNode(ISD::ROTR, DL, MVT::i32, High,
                                  DAG.getConstant(1, DL, MVT::i32));
    return DAG.getNode(ISD::XOR, DL, MVT::i32, Converted, HighBit);
  }
  if (Op.getOpcode() == ISD::FP_EXTEND) {
    if (Op.getOperand(0).getValueType() != MVT::f32 ||
        Op.getValueType() != MVT::f64)
      report_fatal_error(
          "SH hardware floating extension supports only f32 to f64");
    return DAG.getNode(SHISD::F32_TO_F64, SDLoc(Op), MVT::f64,
                       Op.getOperand(0));
  }
  if (Op.getOpcode() == ISD::FP_ROUND) {
    if (Op.getOperand(0).getValueType() != MVT::f64 ||
        Op.getValueType() != MVT::f32)
      report_fatal_error(
          "SH hardware floating rounding supports only f64 to f32");
    return DAG.getNode(SHISD::F64_TO_F32, SDLoc(Op), MVT::f32,
                       Op.getOperand(0));
  }
  if (Op.getOpcode() == ISD::FRAMEADDR)
    return lowerSHFrameAddress(Op, DAG);
  if (Op.getOpcode() == ISD::RETURNADDR)
    return lowerSHReturnAddress(Op, DAG);
  if (Op.getOpcode() == ISD::CTPOP)
    return expandCTPOP(Op.getNode(), DAG);
  if (Op.getOpcode() == ISD::MUL)
    return lowerSH1Multiply(*this, Op, DAG);
  if (Op.getOpcode() == ISD::SMULO || Op.getOpcode() == ISD::UMULO)
    return lowerSHMultiplyOverflow(*this, Op, Subtarget.hasSH2(), DAG);
  if (Op.getOpcode() == ISD::SMUL_LOHI || Op.getOpcode() == ISD::UMUL_LOHI)
    return lowerSHMultiplyHalves(*this, Op.getOperand(0), Op.getOperand(1),
                                 Op.getOpcode() == ISD::SMUL_LOHI,
                                 Subtarget.hasSH2(), SDLoc(Op), DAG);
  if (Op.getOpcode() == ISD::MULHS || Op.getOpcode() == ISD::MULHU)
    return lowerSHMultiplyHigh(*this, Op.getOperand(0), Op.getOperand(1),
                               Op.getOpcode() == ISD::MULHS, Subtarget.hasSH2(),
                               SDLoc(Op), DAG);
  if (Op.getOpcode() == ISD::INTRINSIC_WO_CHAIN) {
    unsigned IntrinsicID = Op.getConstantOperandVal(0);
    if (IntrinsicID == Intrinsic::thread_pointer)
      return DAG.getNode(SHISD::THREAD_POINTER, SDLoc(Op),
                         getPointerTy(DAG.getDataLayout()));
  }
  if (Op.getOpcode() == ISD::EH_RETURN)
    return LowerEH_RETURN(Op, DAG);
  if (Op.getOpcode() == ISD::VASTART)
    return LowerVASTART(Op, DAG);
  if (Op.getOpcode() == ISD::VAARG)
    return LowerVAARG(Op, DAG);
  if (Op.getOpcode() == ISD::VACOPY)
    return LowerVACOPY(Op, DAG);
  if (Op.getOpcode() == ISD::ATOMIC_FENCE)
    return LowerATOMIC_FENCE(Op, DAG);
  if (Op.getOpcode() == ISD::SIGN_EXTEND_INREG) {
    EVT InRegVT = cast<VTSDNode>(Op.getOperand(1))->getVT();
    assert(InRegVT == MVT::i1 && "unexpected SH sign-extend-in-register type");
    return lowerSHBooleanSignExtend(Op.getOperand(0), SDLoc(Op), DAG);
  }
  if (Op.getOpcode() == ISD::ROTL || Op.getOpcode() == ISD::ROTR)
    return lowerSHRotate(*this, Op, DAG);
  if (Op.getOpcode() == ISD::SELECT_CC) {
    if (DAG.getMachineFunction().getFunction().hasOptNone())
      return SDValue();
    if (Op.getOperand(0).getValueType() != MVT::i32)
      return SDValue();
    SDValue Predicate = DAG.getTargetConstant(
        static_cast<unsigned>(cast<CondCodeSDNode>(Op.getOperand(4))->get()),
        SDLoc(Op), MVT::i32);
    return DAG.getNode(SHISD::SELECT_CC, SDLoc(Op), MVT::i32,
                       {Op.getOperand(0), Op.getOperand(1), Op.getOperand(2),
                        Op.getOperand(3), Predicate});
  }
  if (Op.getOpcode() == ISD::SHL || Op.getOpcode() == ISD::SRL ||
      Op.getOpcode() == ISD::SRA)
    return lowerSHVariableShift(*this, Op, DAG);
  if (Op.getOpcode() == ISD::UADDO || Op.getOpcode() == ISD::SADDO ||
      Op.getOpcode() == ISD::USUBO || Op.getOpcode() == ISD::SSUBO)
    return lowerSHOverflow(Op, DAG);
  if (Op.getOpcode() == ISD::BR_CC)
    return LowerBR_CC(Op, DAG);
  if (Op.getOpcode() == ISD::SETCC)
    return LowerSETCC(Op, DAG);
  if (Op.getOpcode() == ISD::SELECT)
    return LowerSELECT(Op, DAG);
  if (Op.getOpcode() == ISD::SHL_PARTS || Op.getOpcode() == ISD::SRL_PARTS ||
      Op.getOpcode() == ISD::SRA_PARTS)
    return lowerSHShiftParts(Op, DAG);
  if (Op.getOpcode() == ISD::SDIVREM || Op.getOpcode() == ISD::UDIVREM)
    return LowerDIVREM(Op, DAG);
  if (Op.getOpcode() == ISD::LOAD || Op.getOpcode() == ISD::STORE) {
    const auto *Mem = cast<MemSDNode>(Op);
    EVT MemoryVT = Mem->getMemoryVT();
    if (MemoryVT == MVT::f64)
      return Op.getOpcode() == ISD::LOAD ? lowerSHF64Load(Op, DAG)
                                         : lowerSHF64Store(Op, DAG);
    // Native integer legalization owns ordinary splitting, extension and
    // alignment. Retain ordered byte/word accesses for volatile memory: the
    // generic expander joins its pieces with a TokenFactor and may use wider
    // accesses when the base alignment permits them.
    if (auto *Store = dyn_cast<StoreSDNode>(Op);
        Store && Store->getValue().getValueType() == MVT::i64 &&
        !Store->isVolatile())
      return SDValue();
    if (MemoryVT == MVT::i64) {
      if (!Mem->isVolatile())
        return SDValue();
      return Op.getOpcode() == ISD::LOAD ? lowerSHVolatileI64Load(Op, DAG)
                                         : lowerSHVolatileI64Store(Op, DAG);
    }
    if (MemoryVT != MVT::i8 && MemoryVT != MVT::i16 && MemoryVT != MVT::i32)
      report_fatal_error("SH unexpected legalized integer memory type");
    if (Mem->getAlign() < Align(MemoryVT.getStoreSize())) {
      if (Mem->isVolatile())
        return Op.getOpcode() == ISD::LOAD
                   ? lowerSHVolatileUnalignedIntegerLoad(Op, DAG)
                   : lowerSHVolatileUnalignedIntegerStore(Op, DAG);
      if (auto *Load = dyn_cast<LoadSDNode>(Op)) {
        auto [Value, Chain] = expandUnalignedLoad(Load, DAG);
        return DAG.getMergeValues({Value, Chain}, SDLoc(Op));
      }
      return expandUnalignedStore(cast<StoreSDNode>(Op), DAG);
    }
    if (!isLegalSHAddress(Mem->getBasePtr()))
      report_fatal_error(
          "SH byte/word memory address must be a 32-bit pointer");
    return Op;
  }
  if (Op.getOpcode() == ISD::GlobalAddress)
    return LowerGlobalAddress(Op, DAG);
  if (Op.getOpcode() == ISD::BR_JT)
    return LowerBR_JT(Op, DAG);
  if (Op.getOpcode() == ISD::ConstantPool)
    return LowerConstantPool(Op, DAG);
  if (Op.getOpcode() == ISD::GlobalTLSAddress)
    return LowerGlobalTLSAddress(Op, DAG);
  if (Op.getOpcode() == ISD::BlockAddress)
    return LowerBlockAddress(Op, DAG);
  if (Op.getOpcode() == ISD::JumpTable)
    return LowerJumpTable(Op, DAG);
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

SDValue SHTargetLowering::LowerEH_RETURN(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SDValue Chain = Op.getOperand(0);
  SDValue Offset = Op.getOperand(1);
  SDValue Handler = Op.getOperand(2);
  Chain = DAG.getCopyToReg(Chain, DL, SH::R3, Offset, SDValue());
  // Keep the handler out of ABI r3 and the large-frame epilogue's r0 scratch.
  Chain = DAG.getCopyToReg(Chain, DL, SH::R2, Handler, Chain.getValue(1));
  return DAG.getNode(SHISD::EH_RETURN, DL, MVT::Other, Chain,
                     DAG.getRegister(SH::R2, MVT::i32), Chain.getValue(1));
}

Register SHTargetLowering::getExceptionPointerRegister(
    ExceptionHandling EH, const Constant *PersonalityFn) const {
  return SH::R4;
}

Register SHTargetLowering::getExceptionSelectorRegister(
    ExceptionHandling EH, const Constant *PersonalityFn) const {
  return SH::R5;
}

static Register createGPR(MachineRegisterInfo &MRI) {
  return MRI.createVirtualRegister(&SH::GPRRegClass);
}

static MachineBasicBlock *emitJumpTableDispatch(MachineInstr &MI,
                                                MachineBasicBlock *MBB) {
  if (MI.getNumOperands() != 3 || !MI.getOperand(0).isReg() ||
      !MI.getOperand(1).isReg() || !MI.getOperand(2).isJTI())
    reportFatalUsageError(
        "malformed SH jump-table dispatch operands; expected index, base, and "
        "jump-table index");

  MachineFunction &MF = *MBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const MachineJumpTableInfo *MJTI = MF.getJumpTableInfo();
  unsigned JTI = MI.getOperand(2).getIndex();
  if (!MJTI || JTI >= MJTI->getJumpTables().size())
    reportFatalUsageError("SH jump-table dispatch has an invalid table index");
  bool IsPIC = MF.getTarget().isPositionIndependent();
  bool IsInline = MJTI->getEntryKind() == MachineJumpTableInfo::EK_Inline;
  MachineJumpTableInfo::JTEntryKind ExpectedKind =
      MF.getSubtarget<SHSubtarget>().hasSH2() ? MachineJumpTableInfo::EK_Inline
      : IsPIC ? MachineJumpTableInfo::EK_LabelDifference32
              : MachineJumpTableInfo::EK_BlockAddress;
  if (MJTI->getEntryKind() != ExpectedKind ||
      (!IsInline && (MJTI->getEntrySize(MF.getDataLayout()) != 4 ||
                     MJTI->getEntryAlignment(MF.getDataLayout()) != 4))) {
    if (ExpectedKind == MachineJumpTableInfo::EK_Inline)
      reportFatalUsageError(
          "SH2 jump-table dispatch requires inline relative entries");
    reportFatalUsageError(
        IsPIC ? "SH PIC jump-table dispatch requires four-byte relative entries"
              : "SH jump-table dispatch requires four-byte absolute block "
                "addresses");
  }

  if (MI.memoperands().size() != 1)
    reportFatalUsageError(
        "SH jump-table dispatch requires one table-entry load memory operand");
  MachineMemOperand *MMO = *MI.memoperands_begin();
  const PseudoSourceValue *PSV = MMO->getPseudoValue();
  if (!MMO->isLoad() || MMO->isStore() ||
      MMO->getSize() != LocationSize::precise(4) ||
      MMO->getAlign() < Align(4) || !MMO->isInvariant() ||
      !MMO->isDereferenceable() || !PSV || !PSV->isJumpTable())
    reportFatalUsageError(
        "SH jump-table dispatch requires an invariant, dereferenceable, "
        "four-byte aligned jump-table load");

  const DebugLoc &DL = MI.getDebugLoc();
  Register Index = MI.getOperand(0).getReg();
  Register Base = MI.getOperand(1).getReg();
  Register IndexCopy = createGPR(MRI);
  Register Scaled = createGPR(MRI);
  Register Target = createGPR(MRI);
  unsigned CopyOpcode = SH::MOVrr;
  if (MF.getTarget().getOptLevel() != CodeGenOptLevel::None)
    CopyOpcode = TargetOpcode::COPY;

  BuildMI(*MBB, MI, DL, TII.get(CopyOpcode), IndexCopy).addReg(Index);
  if (IsInline)
    BuildMI(*MBB, MI, DL, TII.get(SH::MOVA), SH::R0).addJumpTableIndex(JTI);
  BuildMI(*MBB, MI, DL, TII.get(SH::SHLL2), Scaled).addReg(IndexCopy);
  if (IsInline)
    BuildMI(*MBB, MI, DL, TII.get(SH::MOVL_load_indexed), Target)
        .addReg(Scaled)
        .addMemOperand(MMO);
  else {
    Register BaseCopy = createGPR(MRI);
    Register EntryAddress = createGPR(MRI);
    BuildMI(*MBB, MI, DL, TII.get(CopyOpcode), BaseCopy).addReg(Base);
    BuildMI(*MBB, MI, DL, TII.get(SH::ADDrr), EntryAddress)
        .addReg(BaseCopy)
        .addReg(Scaled);
    BuildMI(*MBB, MI, DL, TII.get(SH::MOVL_load_reg), Target)
        .addReg(EntryAddress)
        .addMemOperand(MMO);
  }
  Register DispatchTarget = Target;
  if (IsPIC && !IsInline) {
    DispatchTarget = createGPR(MRI);
    BuildMI(*MBB, MI, DL, TII.get(SH::ADDrr), DispatchTarget)
        .addReg(Target)
        .addReg(Base);
  }
  BuildMI(*MBB, MI, DL, TII.get(IsInline ? SH::BRAF : SH::JMP))
      .addReg(DispatchTarget)
      .addJumpTableIndex(JTI);

  MI.eraseFromParent();
  return MBB;
}

static MachineBasicBlock *emitMultiply(MachineInstr &MI,
                                       MachineBasicBlock *MBB) {
  MachineFunction &MF = *MBB->getParent();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  unsigned Pseudo = MI.getOpcode();
  bool IsLoHi =
      Pseudo == SH::DMULS_LOHI_PSEUDO || Pseudo == SH::DMULU_LOHI_PSEUDO;
  unsigned InputIndex = IsLoHi ? 2 : 1;
  Register LHS = MI.getOperand(InputIndex).getReg();
  Register RHS = MI.getOperand(InputIndex + 1).getReg();
  unsigned MultiplyOpcode;
  switch (Pseudo) {
  case SH::MUL32_PSEUDO:
    MultiplyOpcode = SH::MUL_L;
    break;
  case SH::MULS_WORD_PSEUDO:
    MultiplyOpcode = SH::MULS_W;
    break;
  case SH::MULU_WORD_PSEUDO:
    MultiplyOpcode = SH::MULU_W;
    break;
  case SH::DMULS_LOHI_PSEUDO:
  case SH::DMULS_HIGH_PSEUDO:
    MultiplyOpcode = SH::DMULS_L;
    break;
  case SH::DMULU_LOHI_PSEUDO:
  case SH::DMULU_HIGH_PSEUDO:
    MultiplyOpcode = SH::DMULU_L;
    break;
  default:
    llvm_unreachable("unexpected SH multiply pseudo");
  }

  BuildMI(*MBB, MI, DL, TII.get(MultiplyOpcode)).addReg(LHS).addReg(RHS);
  if (IsLoHi) {
    BuildMI(*MBB, MI, DL, TII.get(SH::STS_MACL), MI.getOperand(0).getReg());
    BuildMI(*MBB, MI, DL, TII.get(SH::STS_MACH), MI.getOperand(1).getReg());
  } else if (Pseudo == SH::DMULS_HIGH_PSEUDO ||
             Pseudo == SH::DMULU_HIGH_PSEUDO) {
    BuildMI(*MBB, MI, DL, TII.get(SH::STS_MACH), MI.getOperand(0).getReg());
  } else {
    BuildMI(*MBB, MI, DL, TII.get(SH::STS_MACL), MI.getOperand(0).getReg());
  }

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

static MachineBasicBlock *emitNarrowDivision(MachineInstr &MI,
                                             MachineBasicBlock *MBB) {
  MachineRegisterInfo &MRI = MBB->getParent()->getRegInfo();
  const SHInstrInfo &TII =
      *MBB->getParent()->getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  Register Dividend = MI.getOperand(1).getReg();
  Register Divisor = MI.getOperand(2).getReg();
  auto Unary = [&](unsigned Opcode, Register Source) {
    Register Dest = createGPR(MRI);
    BuildMI(*MBB, MI, DL, TII.get(Opcode), Dest).addReg(Source);
    return Dest;
  };
  Register Partial = Unary(SH::SHLR16, Dividend);
  Register ScaledDivisor = Unary(SH::SHLL16, Divisor);
  BuildMI(*MBB, MI, DL, TII.get(SH::DIV0U));
  auto DivideGroup = [&](Register Value) {
    for (unsigned I = 0; I != 16; ++I) {
      Register Next = createGPR(MRI);
      BuildMI(*MBB, MI, DL, TII.get(SH::DIV1), Next)
          .addReg(Value)
          .addReg(ScaledDivisor);
      Value = Next;
    }
    return Value;
  };
  // With the divisor scaled by 2^16, DIV1 packs quotient bits in the low
  // halfword while Q and the high halfword carry the partial remainder.
  Partial = DivideGroup(Partial);
  Register Packed = Unary(SH::SHLL16, Partial);
  // Save the first quotient group and bring in the low dividend halfword
  // without disturbing Q/M/T. The pending quotient bit remains in T.
  Register Spliced = createGPR(MRI);
  BuildMI(*MBB, MI, DL, TII.get(SH::XTRCT), Spliced)
      .addReg(Partial)
      .addReg(Dividend);
  Partial = DivideGroup(Unary(SH::SWAP_W, Spliced));
  Register Low = Unary(SH::EXTUW, Partial);
  Register Combined = createGPR(MRI);
  BuildMI(*MBB, MI, DL, TII.get(SH::ORrr), Combined).addReg(Low).addReg(Packed);
  BuildMI(*MBB, MI, DL, TII.get(SH::ROTCL), MI.getOperand(0).getReg())
      .addReg(Combined);
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

static MachineBasicBlock *emitSH2ADivision(MachineInstr &MI,
                                           MachineBasicBlock *MBB) {
  MachineFunction &MF = *MBB->getParent();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  Register Dest = MI.getOperand(0).getReg();
  Register Dividend = MI.getOperand(1).getReg();
  Register Divisor = MI.getOperand(2).getReg();
  Register Work = MRI.createVirtualRegister(&SH::GPRNoR0RegClass);
  BuildMI(*MBB, MI, DL, TII.get(TargetOpcode::COPY), Work).addReg(Dividend);
  BuildMI(*MBB, MI, DL, TII.get(TargetOpcode::COPY), SH::R0).addReg(Divisor);
  BuildMI(*MBB, MI, DL,
          TII.get(MI.getOpcode() == SH::SH2A_SDIV_PSEUDO ? SH::DIVS : SH::DIVU),
          Dest)
      .addReg(Work);
  MI.eraseFromParent();
  return MBB;
}

static MachineBasicBlock *emitI32Constant(MachineInstr &MI,
                                          MachineBasicBlock *MBB) {
  MachineFunction &MF = *MBB->getParent();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  TII.materializeI32(*MBB, MI, MI.getDebugLoc(), MI.getOperand(0).getReg(),
                     static_cast<uint32_t>(MI.getOperand(1).getImm()));
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

SH::DynamicShiftPlan llvm::SH::planDynamicShift(unsigned PseudoOpcode) {
  switch (PseudoOpcode) {
  default:
    llvm_unreachable("unexpected SH dynamic shift pseudo");
  case SH::SHLrr:
    return {SH::SHLD, false};
  case SH::SRLrr:
    return {SH::SHLD, true};
  case SH::SRArr:
    return {SH::SHAD, true};
  }
}

static MachineBasicBlock *emitConstantShift(MachineInstr &MI,
                                            MachineBasicBlock *MBB) {
  MachineFunction &MF = *MBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHSubtarget &STI = MF.getSubtarget<SHSubtarget>();
  const SHInstrInfo &TII = *STI.getInstrInfo();
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
  } else if (!STI.hasDynamicShift() && MI.getOpcode() == SH::SRLri &&
             SignedAmount == 31) {
    Register Rotated = createGPR(MRI);
    BuildMI(*MBB, MI, DL, TII.get(SH::ROTL), Rotated).addReg(Current);
    BuildMI(*MBB, MI, DL, TII.get(SH::MOVT), Dest);
  } else if (STI.hasDynamicShift() && Opcodes.size() > 2) {
    Register Count = createGPR(MRI);
    bool IsLeft = MI.getOpcode() == SH::SHLri;
    BuildMI(*MBB, MI, DL, TII.get(SH::MOVri), Count)
        .addImm(IsLeft ? SignedAmount : -SignedAmount);
    BuildMI(*MBB, MI, DL,
            TII.get(MI.getOpcode() == SH::SRAri ? SH::SHAD : SH::SHLD), Dest)
        .addReg(Current)
        .addReg(Count);
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

static MachineBasicBlock *emitSmallCountShift(MachineInstr &MI,
                                              MachineBasicBlock *EntryMBB) {
  MachineFunction &MF = *EntryMBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc DL = MI.getDebugLoc();
  const MachineOperand Source = MI.getOperand(1);
  const MachineOperand Count = MI.getOperand(2);
  unsigned Base = MI.getOperand(3).getImm();
  unsigned Mask = MI.getOperand(4).getImm();
  if (Base >= 32 || !Mask || Mask >= 32 || (Base & Mask) ||
      llvm::popcount(Mask) > 2)
    report_fatal_error("malformed SH small-count shift domain");
  unsigned Shift =
      MI.getOpcode() == SH::SHL_SMALL_PSEUDO ? SH::SHLri : SH::SRLri;
  MachineBasicBlock *DoneMBB =
      MF.CreateMachineBasicBlock(EntryMBB->getBasicBlock());
  MF.insert(std::next(EntryMBB->getIterator()), DoneMBB);
  DoneMBB->splice(DoneMBB->begin(), EntryMBB, std::next(MI.getIterator()),
                  EntryMBB->end());
  DoneMBB->transferSuccessorsAndUpdatePHIs(EntryMBB);
  auto AddUse = [](MachineInstrBuilder Builder, const MachineOperand &Operand) {
    return Builder.addReg(Operand.getReg(), getUndefRegState(Operand.isUndef()),
                          Operand.getSubReg());
  };
  MachineOperand Current = Source;
  for (unsigned Opcode : SH::planConstantShift(Shift, Base)) {
    Register Next = createGPR(MRI);
    AddUse(BuildMI(*EntryMBB, MI, DL, TII.get(Opcode), Next), Current);
    Current = MachineOperand::CreateReg(Next, false);
  }
  MachineBasicBlock *TestMBB = EntryMBB;
  for (unsigned Bit = 16; Bit; Bit >>= 1) {
    if (!(Mask & Bit))
      continue;
    MachineBasicBlock *ShiftMBB =
        MF.CreateMachineBasicBlock(EntryMBB->getBasicBlock());
    MF.insert(DoneMBB->getIterator(), ShiftMBB);
    bool Last = !(Mask & (Bit - 1));
    MachineBasicBlock *JoinMBB = DoneMBB;
    if (!Last) {
      JoinMBB = MF.CreateMachineBasicBlock(EntryMBB->getBasicBlock());
      MF.insert(DoneMBB->getIterator(), JoinMBB);
    }
    TestMBB->addSuccessor(ShiftMBB, BranchProbability(1, 2));
    TestMBB->addSuccessor(JoinMBB, BranchProbability(1, 2));
    ShiftMBB->addSuccessor(JoinMBB);
    auto Insert = TestMBB == EntryMBB ? MI.getIterator() : TestMBB->end();
    Register BitMask = createGPR(MRI);
    BuildMI(*TestMBB, Insert, DL, TII.get(SH::MOVri), BitMask).addImm(Bit);
    AddUse(BuildMI(*TestMBB, Insert, DL, TII.get(SH::TST)), Count)
        .addReg(BitMask);
    BuildMI(*TestMBB, Insert, DL, TII.get(SH::BT)).addMBB(JoinMBB);
    MachineOperand Shifted = Current;
    for (unsigned Opcode : SH::planConstantShift(Shift, Bit)) {
      Register Next = createGPR(MRI);
      AddUse(BuildMI(*ShiftMBB, ShiftMBB->end(), DL, TII.get(Opcode), Next),
             Shifted);
      Shifted = MachineOperand::CreateReg(Next, false);
    }
    Register Selected = Last ? MI.getOperand(0).getReg() : createGPR(MRI);
    AddUse(AddUse(BuildMI(*JoinMBB, JoinMBB->begin(), DL,
                          TII.get(TargetOpcode::PHI), Selected),
                  Current)
               .addMBB(TestMBB),
           Shifted)
        .addMBB(ShiftMBB);
    Current = MachineOperand::CreateReg(Selected, false);
    TestMBB = JoinMBB;
  }
  MF.getProperties().resetNoPHIs();
  MI.eraseFromParent();
  return DoneMBB;
}

static MachineBasicBlock *emitVariableShift(MachineInstr &MI,
                                            MachineBasicBlock *EntryMBB) {
  MachineFunction &MF = *EntryMBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHSubtarget &STI = MF.getSubtarget<SHSubtarget>();
  const SHInstrInfo &TII = *STI.getInstrInfo();
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
  Register ValuePhi = createGPR(MRI);
  Register CountPhi = createGPR(MRI);
  Register NextValue = createGPR(MRI);
  Register NextCount = createGPR(MRI);

  // Ordinary shift counts outside 0..31 are poison, so no modulo mask is
  // needed. Rotate lowering normalizes its count before reaching this loop.
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::TST)).addReg(Count).addReg(Count);
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::BT)).addMBB(DoneMBB);

  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TargetOpcode::PHI), ValuePhi)
      .addReg(Source)
      .addMBB(EntryMBB)
      .addReg(NextValue)
      .addMBB(LoopMBB);
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TargetOpcode::PHI), CountPhi)
      .addReg(Count)
      .addMBB(EntryMBB)
      .addReg(NextCount)
      .addMBB(LoopMBB);

  unsigned StepOpcode;
  switch (MI.getOpcode()) {
  default:
    llvm_unreachable("unexpected SH variable shift or rotate pseudo");
  case SH::SHLrr:
    StepOpcode = SH::SHLL;
    break;
  case SH::SRLrr:
    StepOpcode = SH::SHLR;
    break;
  case SH::SRArr:
    StepOpcode = SH::SHAR;
    break;
  case SH::ROTLrr:
    StepOpcode = SH::ROTL;
    break;
  case SH::ROTRrr:
    StepOpcode = SH::ROTR;
    break;
  }
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(StepOpcode), NextValue)
      .addReg(ValuePhi);
  if (STI.hasSH2())
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::DT), NextCount)
        .addReg(CountPhi);
  else {
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::ADDri), NextCount)
        .addReg(CountPhi)
        .addImm(-1);
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::TST))
        .addReg(NextCount)
        .addReg(NextCount);
  }
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::BF)).addMBB(LoopMBB);

  BuildMI(*DoneMBB, DoneMBB->begin(), DL, TII.get(TargetOpcode::PHI), Dest)
      .addReg(Source)
      .addMBB(EntryMBB)
      .addReg(NextValue)
      .addMBB(LoopMBB);

  MI.eraseFromParent();
  return DoneMBB;
}

static MachineBasicBlock *emitShortVariableRotate(MachineInstr &MI,
                                                  MachineBasicBlock *EntryMBB) {
  MachineFunction &MF = *EntryMBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  const BasicBlock *IRBB = EntryMBB->getBasicBlock();
  MachineFunction::iterator Insert = std::next(EntryMBB->getIterator());
  SmallVector<MachineBasicBlock *, 4> RotateMBBs;
  SmallVector<MachineBasicBlock *, 4> NextMBBs;
  for (unsigned I = 0; I != 4; ++I) {
    MachineBasicBlock *RotateMBB = MF.CreateMachineBasicBlock(IRBB);
    MachineBasicBlock *NextMBB = MF.CreateMachineBasicBlock(IRBB);
    MF.insert(Insert, RotateMBB);
    MF.insert(Insert, NextMBB);
    RotateMBBs.push_back(RotateMBB);
    NextMBBs.push_back(NextMBB);
  }
  MachineBasicBlock *DoneMBB = NextMBBs.back();
  DoneMBB->splice(DoneMBB->begin(), EntryMBB,
                  std::next(MachineBasicBlock::iterator(MI)), EntryMBB->end());
  DoneMBB->transferSuccessorsAndUpdatePHIs(EntryMBB);

  Register Dest = MI.getOperand(0).getReg();
  Register CurrentValue = MI.getOperand(1).getReg();
  Register CurrentCount = MI.getOperand(2).getReg();
  MachineBasicBlock *TestMBB = EntryMBB;
  unsigned RotateOpcode =
      MI.getOpcode() == SH::ROTL_SHORT_PSEUDO ? SH::ROTL : SH::ROTR;
  for (unsigned Stage = 0; Stage != 4; ++Stage) {
    MachineBasicBlock *RotateMBB = RotateMBBs[Stage];
    MachineBasicBlock *NextMBB = NextMBBs[Stage];
    Register NextCount = createGPR(MRI);
    BuildMI(*TestMBB, TestMBB == EntryMBB ? MI.getIterator() : TestMBB->end(),
            DL, TII.get(SH::SHLR), NextCount)
        .addReg(CurrentCount);
    BuildMI(*TestMBB, TestMBB == EntryMBB ? MI.getIterator() : TestMBB->end(),
            DL, TII.get(SH::BF))
        .addMBB(NextMBB);
    TestMBB->addSuccessor(RotateMBB);
    TestMBB->addSuccessor(NextMBB);

    Register Rotated = CurrentValue;
    for (unsigned I = 0; I != 1U << Stage; ++I) {
      Register NextValue = createGPR(MRI);
      BuildMI(*RotateMBB, RotateMBB->end(), DL, TII.get(RotateOpcode),
              NextValue)
          .addReg(Rotated);
      Rotated = NextValue;
    }
    RotateMBB->addSuccessor(NextMBB);

    Register MergedValue = Stage == 3 ? Dest : createGPR(MRI);
    BuildMI(*NextMBB, NextMBB->begin(), DL, TII.get(TargetOpcode::PHI),
            MergedValue)
        .addReg(CurrentValue)
        .addMBB(TestMBB)
        .addReg(Rotated)
        .addMBB(RotateMBB);
    CurrentValue = MergedValue;
    CurrentCount = NextCount;
    TestMBB = NextMBB;
  }

  MI.eraseFromParent();
  return DoneMBB;
}

static MachineBasicBlock *emitDynamicShift(MachineInstr &MI,
                                           MachineBasicBlock *MBB) {
  MachineFunction &MF = *MBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  Register Dest = MI.getOperand(0).getReg();
  Register Source = MI.getOperand(1).getReg();
  Register Count = MI.getOperand(2).getReg();
  SH::DynamicShiftPlan Plan = SH::planDynamicShift(MI.getOpcode());

  if (Plan.NegateCount) {
    Register NegatedCount = createGPR(MRI);
    BuildMI(*MBB, MI, DL, TII.get(SH::NEG), NegatedCount).addReg(Count);
    Count = NegatedCount;
  }
  BuildMI(*MBB, MI, DL, TII.get(Plan.Opcode), Dest)
      .addReg(Source)
      .addReg(Count);
  MI.eraseFromParent();
  return MBB;
}

static MachineBasicBlock *emitDynamicRotate(MachineInstr &MI,
                                            MachineBasicBlock *MBB) {
  MachineFunction &MF = *MBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  Register Dest = MI.getOperand(0).getReg();
  Register Source = MI.getOperand(1).getReg();
  Register Count = MI.getOperand(2).getReg();
  Register LeftCount = Count;
  Register RightCount;

  if (MI.getOpcode() == SH::ROTLrr) {
    RightCount = createGPR(MRI);
    BuildMI(*MBB, MI, DL, TII.get(SH::ADDri), RightCount)
        .addReg(Count)
        .addImm(-32);
  } else {
    RightCount = createGPR(MRI);
    LeftCount = createGPR(MRI);
    BuildMI(*MBB, MI, DL, TII.get(SH::NEG), RightCount).addReg(Count);
    BuildMI(*MBB, MI, DL, TII.get(SH::ADDri), LeftCount)
        .addReg(RightCount)
        .addImm(32);
  }

  Register Left = createGPR(MRI);
  Register Right = createGPR(MRI);
  BuildMI(*MBB, MI, DL, TII.get(SH::SHLD), Left)
      .addReg(Source)
      .addReg(LeftCount);
  BuildMI(*MBB, MI, DL, TII.get(SH::SHLD), Right)
      .addReg(Source)
      .addReg(RightCount);
  BuildMI(*MBB, MI, DL, TII.get(SH::ORrr), Dest).addReg(Left).addReg(Right);
  MI.eraseFromParent();
  return MBB;
}

static MachineBasicBlock *emitFPToUInt(MachineInstr &MI,
                                       MachineBasicBlock *EntryMBB) {
  MachineFunction &MF = *EntryMBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc DL = MI.getDebugLoc();
  const bool Single = MI.getOpcode() == SH::F32_TO_U32_PSEUDO;
  const unsigned Convert =
      Single ? SH::F32_TO_I32_PSEUDO : SH::F64_TO_I32_PSEUDO;
  const MachineOperand Source = MI.getOperand(1);
  const MachineOperand Threshold = MI.getOperand(2);
  Register Dest = MI.getOperand(0).getReg();
  Register Zero = MRI.createVirtualRegister(&SH::GPRRegClass);
  Register One = MRI.createVirtualRegister(&SH::GPRRegClass);
  Register Converted = MRI.createVirtualRegister(&SH::GPRRegClass);
  Register HighBit = MRI.createVirtualRegister(&SH::GPRRegClass);
  Register Mask = MRI.createVirtualRegister(&SH::GPRRegClass);
  Register Adjusted =
      MRI.createVirtualRegister(Single ? &SH::FRRegClass : &SH::DRRegClass);
  Register Convertible =
      MRI.createVirtualRegister(Single ? &SH::FRRegClass : &SH::DRRegClass);
  MachineBasicBlock *HighMBB =
      MF.CreateMachineBasicBlock(EntryMBB->getBasicBlock());
  MachineBasicBlock *DoneMBB =
      MF.CreateMachineBasicBlock(EntryMBB->getBasicBlock());
  auto Insert = std::next(EntryMBB->getIterator());
  MF.insert(Insert, HighMBB);
  MF.insert(Insert, DoneMBB);
  DoneMBB->splice(DoneMBB->begin(), EntryMBB, std::next(MI.getIterator()),
                  EntryMBB->end());
  DoneMBB->transferSuccessorsAndUpdatePHIs(EntryMBB);
  EntryMBB->addSuccessor(DoneMBB, BranchProbability(1, 2));
  EntryMBB->addSuccessor(HighMBB, BranchProbability(1, 2));
  HighMBB->addSuccessor(DoneMBB);
  // FCMP writes T = threshold > source. Only the branch consumes it;
  // integer mask restoration does not rely on incidental later T contents.
  // Preserve undef/subregister data uses; allocation computes new kill flags.
  auto AddUse = [](MachineInstrBuilder Builder, const MachineOperand &Operand) {
    return Builder.addReg(Operand.getReg(), getUndefRegState(Operand.isUndef()),
                          Operand.getSubReg());
  };
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::MOVri), Zero).addImm(0);
  AddUse(AddUse(BuildMI(*EntryMBB, MI, DL,
                        TII.get(Single ? SH::FCMP_GT_FR : SH::FCMP_GT_DR)),
                Source),
         Threshold);
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::BT)).addMBB(DoneMBB);
  AddUse(AddUse(BuildMI(*HighMBB, HighMBB->end(), DL,
                        TII.get(Single ? SH::FSUB_FR : SH::FSUB_DR), Adjusted),
                Source),
         Threshold);
  BuildMI(*HighMBB, HighMBB->end(), DL, TII.get(SH::MOVri), One).addImm(1);
  BuildMI(*HighMBB, HighMBB->end(), DL, TII.get(SH::ROTR), HighBit).addReg(One);
  auto DoneInsert = DoneMBB->begin();
  AddUse(BuildMI(*DoneMBB, DoneInsert, DL, TII.get(TargetOpcode::PHI),
                 Convertible),
         Source)
      .addMBB(EntryMBB)
      .addReg(Adjusted)
      .addMBB(HighMBB);
  BuildMI(*DoneMBB, DoneInsert, DL, TII.get(TargetOpcode::PHI), Mask)
      .addReg(Zero)
      .addMBB(EntryMBB)
      .addReg(HighBit)
      .addMBB(HighMBB);
  BuildMI(*DoneMBB, DoneInsert, DL, TII.get(Convert), Converted)
      .addReg(Convertible);
  BuildMI(*DoneMBB, DoneInsert, DL, TII.get(SH::XORrr), Dest)
      .addReg(Converted)
      .addReg(Mask);
  MF.getProperties().resetNoPHIs();
  MI.eraseFromParent();
  return DoneMBB;
}

static MachineBasicBlock *emitSelect(MachineInstr &MI,
                                     MachineBasicBlock *EntryMBB) {
  if (MI.getNumExplicitOperands() != 4 || !MI.getOperand(0).isReg() ||
      !MI.getOperand(1).isReg() || !MI.getOperand(2).isReg() ||
      !MI.getOperand(3).isReg())
    report_fatal_error("malformed SH select operands; expected result, "
                       "condition, true, and false registers");

  MachineFunction &MF = *EntryMBB->getParent();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  const BasicBlock *IRBB = EntryMBB->getBasicBlock();
  MachineFunction::iterator Insert = std::next(EntryMBB->getIterator());
  MachineBasicBlock *FalseMBB = MF.CreateMachineBasicBlock(IRBB);
  MachineBasicBlock *DoneMBB = MF.CreateMachineBasicBlock(IRBB);
  MF.insert(Insert, FalseMBB);
  MF.insert(Insert, DoneMBB);

  DoneMBB->splice(DoneMBB->begin(), EntryMBB,
                  std::next(MachineBasicBlock::iterator(MI)), EntryMBB->end());
  DoneMBB->transferSuccessorsAndUpdatePHIs(EntryMBB);
  EntryMBB->addSuccessor(FalseMBB);
  EntryMBB->addSuccessor(DoneMBB);
  FalseMBB->addSuccessor(DoneMBB);

  Register Dest = MI.getOperand(0).getReg();
  Register Condition = MI.getOperand(1).getReg();
  Register TrueValue = MI.getOperand(2).getReg();
  Register FalseValue = MI.getOperand(3).getReg();
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::TST))
      .addReg(Condition)
      .addReg(Condition);
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::BF)).addMBB(DoneMBB);
  BuildMI(*DoneMBB, DoneMBB->begin(), DL, TII.get(TargetOpcode::PHI), Dest)
      .addReg(TrueValue)
      .addMBB(EntryMBB)
      .addReg(FalseValue)
      .addMBB(FalseMBB);

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
    reportFatalUsageError("SH integer comparison predicate is not supported");
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

static MachineBasicBlock *emitSelectCC(MachineInstr &MI,
                                       MachineBasicBlock *EntryMBB) {
  if (MI.getNumExplicitOperands() != 6)
    report_fatal_error("malformed SH compare-select operands");
  MachineFunction &MF = *EntryMBB->getParent();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  const BasicBlock *IRBB = EntryMBB->getBasicBlock();
  MachineFunction::iterator Insert = std::next(EntryMBB->getIterator());
  MachineBasicBlock *FalseMBB = MF.CreateMachineBasicBlock(IRBB);
  MachineBasicBlock *DoneMBB = MF.CreateMachineBasicBlock(IRBB);
  MF.insert(Insert, FalseMBB);
  MF.insert(Insert, DoneMBB);

  DoneMBB->splice(DoneMBB->begin(), EntryMBB,
                  std::next(MachineBasicBlock::iterator(MI)), EntryMBB->end());
  DoneMBB->transferSuccessorsAndUpdatePHIs(EntryMBB);
  EntryMBB->addSuccessor(FalseMBB);
  EntryMBB->addSuccessor(DoneMBB);
  FalseMBB->addSuccessor(DoneMBB);

  Register Dest = MI.getOperand(0).getReg();
  Register LHS = MI.getOperand(1).getReg();
  Register RHS = MI.getOperand(2).getReg();
  Register TrueValue = MI.getOperand(3).getReg();
  Register FalseValue = MI.getOperand(4).getReg();
  ISD::CondCode CC = static_cast<ISD::CondCode>(MI.getOperand(5).getImm());
  SHCompareBranch Info = getSHCompareBranch(CC);
  BuildMI(*EntryMBB, MI, DL, TII.get(Info.CompareOpcode))
      .addReg(RHS)
      .addReg(LHS);
  BuildMI(*EntryMBB, MI, DL, TII.get(Info.BranchOnSet ? SH::BT : SH::BF))
      .addMBB(DoneMBB);
  BuildMI(*DoneMBB, DoneMBB->begin(), DL, TII.get(TargetOpcode::PHI), Dest)
      .addReg(TrueValue)
      .addMBB(EntryMBB)
      .addReg(FalseValue)
      .addMBB(FalseMBB);

  MI.eraseFromParent();
  return DoneMBB;
}

static void duplicatePhiIncoming(MachineBasicBlock &MBB,
                                 MachineBasicBlock *OldPred,
                                 MachineBasicBlock *NewPred) {
  MachineFunction &MF = *MBB.getParent();
  for (MachineInstr &PHI : MBB) {
    if (!PHI.isPHI())
      break;
    for (unsigned Index = 1; Index < PHI.getNumOperands(); Index += 2) {
      if (PHI.getOperand(Index + 1).getMBB() != OldPred)
        continue;
      PHI.addOperand(MF, PHI.getOperand(Index));
      PHI.addOperand(MF, MachineOperand::CreateMBB(NewPred));
      break;
    }
  }
}

static MachineBasicBlock *emitOrderedFPBranch(MachineInstr &MI,
                                              MachineBasicBlock *EntryMBB) {
  if (MI.getNumExplicitOperands() != 4 || !MI.getOperand(2).isImm() ||
      !MI.getOperand(3).isMBB())
    report_fatal_error("malformed SH ordered floating branch operands");
  bool BranchOnTrue = MI.getOperand(2).getImm();
  if (MI.getOperand(2).getImm() != 0 && MI.getOperand(2).getImm() != 1)
    report_fatal_error("invalid SH ordered floating branch predicate");

  MachineFunction &MF = *EntryMBB->getParent();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();
  MachineBasicBlock *BranchDest = MI.getOperand(3).getMBB();
  MachineBasicBlock *OtherDest = nullptr;
  BranchProbability BranchDestProbability = BranchProbability(1, 2);
  for (auto Succ = EntryMBB->succ_begin(); Succ != EntryMBB->succ_end();
       ++Succ) {
    MachineBasicBlock *Successor = *Succ;
    if (Successor == BranchDest)
      BranchDestProbability = EntryMBB->getSuccProbability(Succ);
    else if (!OtherDest)
      OtherDest = Successor;
    else
      report_fatal_error("SH ordered floating branch has too many successors");
  }
  if (!EntryMBB->isSuccessor(BranchDest) || !OtherDest)
    report_fatal_error(
        "SH ordered floating branch requires true and false successors");

  const BasicBlock *IRBB = EntryMBB->getBasicBlock();
  MachineBasicBlock *SecondMBB = MF.CreateMachineBasicBlock(IRBB);
  MF.insert(std::next(EntryMBB->getIterator()), SecondMBB);
  SecondMBB->splice(SecondMBB->begin(), EntryMBB,
                    std::next(MachineBasicBlock::iterator(MI)),
                    EntryMBB->end());
  SecondMBB->transferSuccessorsAndUpdatePHIs(EntryMBB);

  MachineBasicBlock *BypassDest = BranchOnTrue ? BranchDest : OtherDest;
  BranchProbability OriginalBypassProbability =
      BranchOnTrue ? BranchDestProbability : BranchDestProbability.getCompl();
  BranchProbability FirstBypassProbability = OriginalBypassProbability / 2;
  BranchProbability ReachSecondProbability = FirstBypassProbability.getCompl();
  BranchProbability SecondBypassProbability =
      ReachSecondProbability.isZero()
          ? BranchProbability::getZero()
          : (OriginalBypassProbability - FirstBypassProbability) /
                ReachSecondProbability;
  EntryMBB->addSuccessor(BypassDest, FirstBypassProbability);
  EntryMBB->addSuccessor(SecondMBB, ReachSecondProbability);
  for (auto Succ = SecondMBB->succ_begin(); Succ != SecondMBB->succ_end();
       ++Succ)
    SecondMBB->setSuccProbability(
        Succ, *Succ == BypassDest ? SecondBypassProbability
                                  : SecondBypassProbability.getCompl());
  duplicatePhiIncoming(*BypassDest, SecondMBB, EntryMBB);

  Register LHS = MI.getOperand(0).getReg();
  Register RHS = MI.getOperand(1).getReg();
  bool IsF32 = MI.getOpcode() == SH::BR_FP_OR_F32_PSEUDO;
  unsigned GreaterOpcode = IsF32 ? SH::FCMP_GT_FR : SH::FCMP_GT_DR;
  unsigned EqualOpcode = IsF32 ? SH::FCMP_EQ_FR : SH::FCMP_EQ_DR;
  BuildMI(*EntryMBB, MI, DL, TII.get(GreaterOpcode)).addReg(RHS).addReg(LHS);
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::BT)).addMBB(BypassDest);
  MachineBasicBlock::iterator SecondInsert = SecondMBB->begin();
  BuildMI(*SecondMBB, SecondInsert, DL, TII.get(EqualOpcode))
      .addReg(RHS)
      .addReg(LHS);
  BuildMI(*SecondMBB, SecondInsert, DL, TII.get(BranchOnTrue ? SH::BT : SH::BF))
      .addMBB(BranchDest);

  MI.eraseFromParent();
  return SecondMBB;
}

MachineBasicBlock *
SHTargetLowering::EmitInstrWithCustomInserter(MachineInstr &MI,
                                              MachineBasicBlock *MBB) const {
  auto CallFrame = preserveSHCallFrame(MI, *MBB);
  switch (MI.getOpcode()) {
  default:
    llvm_unreachable("unexpected SH custom inserter opcode");
  case SH::FTRV4_MEM:
  case SH::FTRV_ARRAY:
  case SH::FTRV_COMPOSE:
    return emitGraphicsRegion(MI, MBB);
  case SH::ADDC_LO_PSEUDO:
  case SH::SUBC_LO_PSEUDO:
    return emitLowCarry(MI, MBB);
  case SH::MUL32_PSEUDO:
  case SH::MULS_WORD_PSEUDO:
  case SH::MULU_WORD_PSEUDO:
  case SH::DMULS_LOHI_PSEUDO:
  case SH::DMULU_LOHI_PSEUDO:
  case SH::DMULS_HIGH_PSEUDO:
  case SH::DMULU_HIGH_PSEUDO:
    return emitMultiply(MI, MBB);
  case SH::UDIV_NARROW_PSEUDO:
    return emitNarrowDivision(MI, MBB);
  case SH::UDIV32_PSEUDO:
  case SH::UREM32_PSEUDO:
  case SH::UDIVREM32_PSEUDO:
  case SH::SDIV32_PSEUDO:
  case SH::SREM32_PSEUDO:
  case SH::SDIVREM32_PSEUDO:
    return emitDivision(MI, MBB);
  case SH::SH2A_SDIV_PSEUDO:
  case SH::SH2A_UDIV_PSEUDO:
    return emitSH2ADivision(MI, MBB);
  case SH::MOVi32:
    return emitI32Constant(MI, MBB);
  case SH::SHLri:
  case SH::SRLri:
  case SH::SRAri:
    return emitConstantShift(MI, MBB);
  case SH::SHLrr:
  case SH::SRLrr:
  case SH::SRArr:
    if (MBB->getParent()->getSubtarget<SHSubtarget>().hasDynamicShift())
      return emitDynamicShift(MI, MBB);
    return emitVariableShift(MI, MBB);
  case SH::SHL_SMALL_PSEUDO:
  case SH::SRL_SMALL_PSEUDO:
    return emitSmallCountShift(MI, MBB);
  case SH::ROTLrr:
  case SH::ROTRrr:
    if (MBB->getParent()->getSubtarget<SHSubtarget>().hasDynamicShift())
      return emitDynamicRotate(MI, MBB);
    return emitVariableShift(MI, MBB);
  case SH::ROTL_SHORT_PSEUDO:
  case SH::ROTR_SHORT_PSEUDO:
    return emitShortVariableRotate(MI, MBB);
  case SH::SHL64_PSEUDO:
  case SH::SRL64_PSEUDO:
  case SH::SRA64_PSEUDO:
    return emitSHVariableI64Shift(MI, MBB);
  case SH::SELECT_PSEUDO:
  case SH::SELECT_F32_PSEUDO:
  case SH::SELECT_F64_PSEUDO:
    return emitSelect(MI, MBB);
  case SH::F32_TO_U32_PSEUDO:
  case SH::F64_TO_U32_PSEUDO:
    return emitFPToUInt(MI, MBB);
  case SH::SELECT_CC_PSEUDO:
    return emitSelectCC(MI, MBB);
  case SH::BR_FP_OR_F32_PSEUDO:
  case SH::BR_FP_OR_F64_PSEUDO:
    return emitOrderedFPBranch(MI, MBB);
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

static std::optional<std::pair<unsigned, bool>>
getSHSignedZeroCompare(ISD::CondCode CC) {
  switch (CC) {
  default:
    return std::nullopt;
  case ISD::SETGE:
    return std::pair(SHISD::CMP_PZ, true);
  case ISD::SETLT:
    return std::pair(SHISD::CMP_PZ, false);
  case ISD::SETGT:
    return std::pair(SHISD::CMP_PL, true);
  case ISD::SETLE:
    return std::pair(SHISD::CMP_PL, false);
  }
}

static void optimizeSHNarrowEqualityOperands(SDValue &LHS, SDValue &RHS,
                                             ISD::CondCode CC, SDLoc DL,
                                             SelectionDAG &DAG) {
  if (CC != ISD::SETEQ && CC != ISD::SETNE)
    return;
  SDValue *Value = &LHS;
  SDValue *ConstantValue = &RHS;
  if (!isa<ConstantSDNode>(*ConstantValue))
    std::swap(Value, ConstantValue);
  const auto *Constant = dyn_cast<ConstantSDNode>(*ConstantValue);
  if (!Constant)
    return;
  const auto *Load = dyn_cast<LoadSDNode>(*Value);
  if (!Load || !Value->hasOneUse() || Load->isAtomic() ||
      Load->getAddressingMode() != ISD::UNINDEXED ||
      Load->getExtensionType() != ISD::ZEXTLOAD ||
      (Load->getMemoryVT() != MVT::i8 && Load->getMemoryVT() != MVT::i16))
    return;
  unsigned MemoryBits = Load->getMemoryVT().getSizeInBits();
  APInt Bits = Constant->getAPIntValue();
  if (Bits.ugt(APInt::getMaxValue(MemoryBits).zext(Bits.getBitWidth())))
    return;
  SDValue SignedLoad = DAG.getExtLoad(
      ISD::SEXTLOAD, DL, Value->getValueType(), Load->getChain(),
      Load->getBasePtr(), Load->getMemoryVT(), Load->getMemOperand());
  DAG.ReplaceAllUsesOfValueWith(Value->getValue(1), SignedLoad.getValue(1));
  *Value = SignedLoad;
  *ConstantValue =
      DAG.getConstant(Bits.trunc(MemoryBits).sext(Bits.getBitWidth()), DL,
                      ConstantValue->getValueType());
}

SDValue SHTargetLowering::LowerSETCC(SDValue Op, SelectionDAG &DAG) const {
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(2))->get();
  SDLoc DL(Op);
  if (LHS.getValueType() == MVT::i64) {
    const MachineFunction &MF = DAG.getMachineFunction();
    const Function &F = MF.getFunction();
    if (F.hasOptSize() || F.hasMinSize() || F.hasOptNone() ||
        OriginalOptLevel == CodeGenOptLevel::None ||
        (CC != ISD::SETULT && CC != ISD::SETULE && CC != ISD::SETUGT &&
         CC != ISD::SETUGE))
      // The type legalizer owns all remaining wide comparisons and selects.
      return SDValue();

    if (CC == ISD::SETUGT || CC == ISD::SETUGE)
      std::swap(LHS, RHS);
    // Only the high-word borrow is needed. Compare the low words to provide
    // its input; including low equality implements the non-strict predicates.
    unsigned LowCompare = CC == ISD::SETULT || CC == ISD::SETUGT
                              ? SHISD::CMP_HI
                              : SHISD::CMP_HS;
    SDValue Borrow = DAG.getNode(LowCompare, DL, MVT::Glue,
                                 getI64Part(RHS, 0, DL, DAG),
                                 getI64Part(LHS, 0, DL, DAG));
    SDValue High = DAG.getNode(ISD::SUBE, DL,
                               DAG.getVTList(MVT::i32, MVT::Glue),
                               getI64Part(LHS, 1, DL, DAG),
                               getI64Part(RHS, 1, DL, DAG), Borrow);
    SDValue Result = DAG.getNode(SHISD::MOVT, DL, MVT::i32, High.getValue(1));
    // Preserve the range through type legalization using the standard node.
    return DAG.getNode(ISD::AssertZext, DL, MVT::i32, Result,
                       DAG.getValueType(MVT::i1));
  }
  if (LHS.getValueType() == MVT::f32 || LHS.getValueType() == MVT::f64) {
    if (RHS.getValueType() != LHS.getValueType())
      report_fatal_error(
          "SH floating comparison operands must have equal types");
    unsigned EqOpcode = LHS.getValueType() == MVT::f32 ? SHISD::FCMP_EQ_F32
                                                       : SHISD::FCMP_EQ_F64;
    unsigned GtOpcode = LHS.getValueType() == MVT::f32 ? SHISD::FCMP_GT_F32
                                                       : SHISD::FCMP_GT_F64;
    auto Compare = [&](unsigned Opcode, SDValue A, SDValue B) {
      SDValue Glue = DAG.getNode(Opcode, DL, MVT::Glue, A, B);
      return DAG.getNode(SHISD::MOVT, DL, MVT::i32, Glue);
    };
    auto Invert = [&](SDValue Value) {
      return DAG.getNode(ISD::XOR, DL, MVT::i32, Value,
                         DAG.getConstant(1, DL, MVT::i32));
    };
    auto Both = [&](SDValue A, SDValue B) {
      return DAG.getNode(ISD::AND, DL, MVT::i32, A, B);
    };
    auto Either = [&](SDValue A, SDValue B) {
      return DAG.getNode(ISD::OR, DL, MVT::i32, A, B);
    };
    auto Equal = [&]() { return Compare(EqOpcode, LHS, RHS); };
    auto Greater = [&]() { return Compare(GtOpcode, LHS, RHS); };
    auto Less = [&]() { return Compare(GtOpcode, RHS, LHS); };
    auto Ordered = [&]() {
      return Both(Compare(EqOpcode, LHS, LHS), Compare(EqOpcode, RHS, RHS));
    };
    switch (CC) {
    default:
      report_fatal_error("SH floating comparison predicate is not supported");
    case ISD::SETFALSE:
    case ISD::SETFALSE2:
      return DAG.getConstant(0, DL, MVT::i32);
    case ISD::SETTRUE:
    case ISD::SETTRUE2:
      return DAG.getConstant(1, DL, MVT::i32);
    case ISD::SETEQ:
    case ISD::SETOEQ:
      return Equal();
    case ISD::SETNE:
    case ISD::SETUNE:
      return Invert(Equal());
    case ISD::SETGT:
    case ISD::SETOGT:
      return Greater();
    case ISD::SETLT:
    case ISD::SETOLT:
      return Less();
    case ISD::SETGE:
    case ISD::SETOGE:
      return Either(Greater(), Equal());
    case ISD::SETLE:
    case ISD::SETOLE:
      return Either(Less(), Equal());
    case ISD::SETUO:
      return Invert(Ordered());
    case ISD::SETONE:
      return Both(Ordered(), Invert(Equal()));
    case ISD::SETO:
      return Ordered();
    case ISD::SETUEQ:
      return Either(Equal(), Invert(Ordered()));
    case ISD::SETUGT:
      return Invert(Either(Less(), Equal()));
    case ISD::SETUGE:
      return Invert(Less());
    case ISD::SETULT:
      return Invert(Either(Greater(), Equal()));
    case ISD::SETULE:
      return Invert(Greater());
    }
  }
  if (const auto *Zero = dyn_cast<ConstantSDNode>(LHS);
      Zero && Zero->isZero()) {
    std::swap(LHS, RHS);
    CC = ISD::getSetCCSwappedOperands(CC);
  }
  if (const auto *Zero = dyn_cast<ConstantSDNode>(RHS);
      Zero && Zero->isZero()) {
    if (auto Compare = getSHSignedZeroCompare(CC)) {
      SDValue Glue = DAG.getNode(Compare->first, DL, MVT::Glue, LHS);
      SDValue Result = DAG.getNode(SHISD::MOVT, DL, MVT::i32, Glue);
      if (Compare->second)
        return Result;
      return DAG.getNode(ISD::XOR, DL, MVT::i32, Result,
                         DAG.getConstant(1, DL, MVT::i32));
    }
  }
  unsigned CompareOpcode;
  bool SwapOperands = false;
  bool Invert = false;

  switch (CC) {
  default:
    report_fatal_error("SH comparison predicate is not supported");
  case ISD::SETEQ:
    CompareOpcode = SHISD::CMP_EQ;
    break;
  case ISD::SETNE:
    CompareOpcode = SHISD::CMP_EQ;
    Invert = true;
    break;
  case ISD::SETGE:
    CompareOpcode = SHISD::CMP_GE;
    break;
  case ISD::SETLT:
    CompareOpcode = SHISD::CMP_GT;
    SwapOperands = true;
    break;
  case ISD::SETGT:
    CompareOpcode = SHISD::CMP_GT;
    break;
  case ISD::SETLE:
    CompareOpcode = SHISD::CMP_GE;
    SwapOperands = true;
    break;
  case ISD::SETUGE:
    CompareOpcode = SHISD::CMP_HS;
    break;
  case ISD::SETULT:
    CompareOpcode = SHISD::CMP_HI;
    SwapOperands = true;
    break;
  case ISD::SETUGT:
    CompareOpcode = SHISD::CMP_HI;
    break;
  case ISD::SETULE:
    CompareOpcode = SHISD::CMP_HS;
    SwapOperands = true;
    break;
  }

  if (SwapOperands)
    std::swap(LHS, RHS);
  optimizeSHNarrowEqualityOperands(LHS, RHS, CC, DL, DAG);

  SDValue Glue;
  if ((CC == ISD::SETEQ || CC == ISD::SETNE) &&
      ((isa<ConstantSDNode>(RHS) && cast<ConstantSDNode>(RHS)->isZero()) ||
       (isa<ConstantSDNode>(LHS) && cast<ConstantSDNode>(LHS)->isZero()))) {
    SDValue Value = isa<ConstantSDNode>(RHS) ? LHS : RHS;
    Glue = DAG.getNode(SHISD::TST, DL, MVT::Glue, Value, Value);
  } else {
    Glue = DAG.getNode(CompareOpcode, DL, MVT::Glue, LHS, RHS);
  }

  SDValue Result = DAG.getNode(SHISD::MOVT, DL, MVT::i32, Glue);
  if (!Invert)
    return Result;
  return DAG.getNode(ISD::XOR, DL, MVT::i32, Result,
                     DAG.getConstant(1, DL, MVT::i32));
}

SDValue SHTargetLowering::LowerSELECT(SDValue Op, SelectionDAG &DAG) const {
  unsigned Opcode = Op.getValueType() == MVT::f32   ? SHISD::SELECT_F32
                    : Op.getValueType() == MVT::f64 ? SHISD::SELECT_F64
                                                    : SHISD::SELECT;
  return DAG.getNode(Opcode, SDLoc(Op), Op.getValueType(), Op.getOperand(0),
                     Op.getOperand(1), Op.getOperand(2));
}

SDValue SHTargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const {
  SDValue Chain = Op.getOperand(0);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue Dest = Op.getOperand(4);
  SDLoc DL(Op);

  if (const auto *Zero = dyn_cast<ConstantSDNode>(LHS);
      Zero && Zero->isZero()) {
    std::swap(LHS, RHS);
    CC = ISD::getSetCCSwappedOperands(CC);
  }
  if (const auto *Zero = dyn_cast<ConstantSDNode>(RHS);
      Zero && Zero->isZero()) {
    if (auto Compare = getSHSignedZeroCompare(CC)) {
      SDValue Glue = DAG.getNode(Compare->first, DL, MVT::Glue, LHS);
      return DAG.getNode(Compare->second ? SHISD::BT : SHISD::BF, DL,
                         MVT::Other, Chain, Dest, Glue);
    }
  }
  optimizeSHNarrowEqualityOperands(LHS, RHS, CC, DL, DAG);

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

const char *SHTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case SHISD::RET_GLUE:
    return "SHISD::RET_GLUE";
  case SHISD::RTE_GLUE:
    return "SHISD::RTE_GLUE";
  case SHISD::TRAP_EXIT:
    return "SHISD::TRAP_EXIT";
  case SHISD::EH_RETURN:
    return "SHISD::EH_RETURN";
  case SHISD::CALL:
    return "SHISD::CALL";
  case SHISD::CALL_PCREL:
    return "SHISD::CALL_PCREL";
  case SHISD::TAIL_CALL:
    return "SHISD::TAIL_CALL";
  case SHISD::SYNCO:
    return "SHISD::SYNCO";
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
  case SHISD::CMP_PL:
    return "SHISD::CMP_PL";
  case SHISD::CMP_PZ:
    return "SHISD::CMP_PZ";
  case SHISD::TST:
    return "SHISD::TST";
  case SHISD::BT:
    return "SHISD::BT";
  case SHISD::BF:
    return "SHISD::BF";
  case SHISD::UDIV:
    return "SHISD::UDIV";
  case SHISD::UDIV_NARROW:
    return "SHISD::UDIV_NARROW";
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
  case SHISD::MULS_WORD:
    return "SHISD::MULS_WORD";
  case SHISD::MULU_WORD:
    return "SHISD::MULU_WORD";
  case SHISD::SMUL_LOHI:
    return "SHISD::SMUL_LOHI";
  case SHISD::UMUL_LOHI:
    return "SHISD::UMUL_LOHI";
  case SHISD::MUL_HIGH_SIGNED:
    return "SHISD::MUL_HIGH_SIGNED";
  case SHISD::MUL_HIGH_UNSIGNED:
    return "SHISD::MUL_HIGH_UNSIGNED";
  case SHISD::SHL_PARTS:
    return "SHISD::SHL_PARTS";
  case SHISD::SRL_PARTS:
    return "SHISD::SRL_PARTS";
  case SHISD::SRA_PARTS:
    return "SHISD::SRA_PARTS";
  case SHISD::SHL:
    return "SHISD::SHL";
  case SHISD::MOVT:
    return "SHISD::MOVT";
  case SHISD::ROTL1:
    return "SHISD::ROTL1";
  case SHISD::ROTR1:
    return "SHISD::ROTR1";
  case SHISD::ROTL16:
    return "SHISD::ROTL16";
  case SHISD::ROTL:
    return "SHISD::ROTL";
  case SHISD::ROTR:
    return "SHISD::ROTR";
  case SHISD::ROTL_SHORT:
    return "SHISD::ROTL_SHORT";
  case SHISD::SHL_SMALL:
    return "SHISD::SHL_SMALL";
  case SHISD::SRL_SMALL:
    return "SHISD::SRL_SMALL";
  case SHISD::ROTR_SHORT:
    return "SHISD::ROTR_SHORT";
  case SHISD::ADDV:
    return "SHISD::ADDV";
  case SHISD::SUBV:
    return "SHISD::SUBV";
  case SHISD::BOOL_SEXT:
    return "SHISD::BOOL_SEXT";
  case SHISD::SELECT:
    return "SHISD::SELECT";
  case SHISD::SELECT_CC:
    return "SHISD::SELECT_CC";
  case SHISD::BR_JT:
    return "SHISD::BR_JT";
  case SHISD::PIC_ADDRESS:
    return "SHISD::PIC_ADDRESS";
  case SHISD::PIC_GOTOFF:
    return "SHISD::PIC_GOTOFF";
  case SHISD::THREAD_POINTER:
    return "SHISD::THREAD_POINTER";
  case SHISD::TLS_CALL:
    return "SHISD::TLS_CALL";
  case SHISD::TLS_IE:
    return "SHISD::TLS_IE";
  default:
    return nullptr;
  }
}

unsigned SHTargetLowering::getJumpTableEncoding() const {
  if (Subtarget.hasSH2())
    return MachineJumpTableInfo::EK_Inline;
  return getTargetMachine().isPositionIndependent()
             ? MachineJumpTableInfo::EK_LabelDifference32
             : MachineJumpTableInfo::EK_BlockAddress;
}

bool SHTargetLowering::isSuitableForJumpTable(const SwitchInst *SI,
                                              uint64_t NumCases, uint64_t Range,
                                              ProfileSummaryInfo *PSI,
                                              BlockFrequencyInfo *BFI) const {
  if (SI->getCondition()->getType()->getIntegerBitWidth() > 32)
    return false;
  return TargetLowering::isSuitableForJumpTable(SI, NumCases, Range, PSI, BFI);
}
