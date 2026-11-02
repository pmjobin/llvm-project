//===-- SHISelLowering.h - SH SelectionDAG lowering ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHISELLOWERING_H
#define LLVM_LIB_TARGET_SH_SHISELLOWERING_H

#include "SHSelectionDAGInfo.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {

class SHSubtarget;

namespace SH {
struct DynamicShiftPlan {
  unsigned Opcode;
  bool NegateCount;
};

DynamicShiftPlan planDynamicShift(unsigned PseudoOpcode);
SmallVector<unsigned, 16> planConstantShift(unsigned PseudoOpcode,
                                            unsigned Amount);
SmallVector<unsigned, 16> planMemoryAccesses(uint64_t Size, Align Alignment);
} // namespace SH

class SHTargetLowering : public TargetLowering {
  const SHSubtarget &Subtarget;
  const CodeGenOptLevel OriginalOptLevel;

public:
  SHTargetLowering(const TargetMachine &TM, const SHSubtarget &STI);
  const TargetRegisterClass *getRepRegClassFor(MVT VT) const override;
  uint8_t getRepRegClassCostFor(MVT VT) const override;
  MVT getRegisterTypeForCallingConv(LLVMContext &Context, CallingConv::ID CC,
                                    EVT VT) const override;
  unsigned getNumRegistersForCallingConv(LLVMContext &Context,
                                         CallingConv::ID CC,
                                         EVT VT) const override;
  bool
  splitValueIntoRegisterParts(SelectionDAG &DAG, const SDLoc &DL, SDValue Val,
                              SDValue *Parts, unsigned NumParts, MVT PartVT,
                              std::optional<CallingConv::ID> CC) const override;
  SDValue
  joinRegisterPartsIntoValue(SelectionDAG &DAG, const SDLoc &DL,
                             const SDValue *Parts, unsigned NumParts,
                             MVT PartVT, EVT ValueVT,
                             std::optional<CallingConv::ID> CC) const override;
  EVT getSetCCResultType(const DataLayout &, LLVMContext &,
                         EVT VT) const override {
    return VT.isVector() ? VT.changeVectorElementTypeToInteger() : MVT::i32;
  }

  bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
                      bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      LLVMContext &Context, const Type *RetTy) const override;
  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool IsVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &DL, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;
  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
                      SelectionDAG &DAG) const override;
  SDValue LowerCall(CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;
  void getTgtMemIntrinsic(SmallVectorImpl<IntrinsicInfo> &Infos,
                          const CallBase &I, MachineFunction &MF,
                          unsigned Intrinsic) const override;
  SDValue LowerATOMIC_FENCE(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVACOPY(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVAARG(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVASTART(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSETCC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSELECT(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerDIVREM(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;
  SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const override;
  void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue> &Results,
                          SelectionDAG &DAG) const override;
  MachineBasicBlock *
  EmitInstrWithCustomInserter(MachineInstr &MI,
                              MachineBasicBlock *MBB) const override;
  MachineBasicBlock *emitGraphicsRegion(MachineInstr &MI,
                                        MachineBasicBlock *MBB) const;
  void AdjustInstrPostInstrSelection(MachineInstr &MI,
                                     SDNode *Node) const override;
  SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBR_JT(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerConstantPool(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalTLSAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerJumpTable(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerEH_RETURN(SDValue Op, SelectionDAG &DAG) const;
  Register
  getExceptionPointerRegister(ExceptionHandling EH,
                              const Constant *PersonalityFn) const override;
  Register
  getExceptionSelectorRegister(ExceptionHandling EH,
                               const Constant *PersonalityFn) const override;
  const char *getTargetNodeName(unsigned Opcode) const override;
  unsigned getJumpTableEncoding() const override;
  bool isSuitableForJumpTable(const SwitchInst *SI, uint64_t NumCases,
                              uint64_t Range, ProfileSummaryInfo *PSI,
                              BlockFrequencyInfo *BFI) const override;
  bool isIntDivCheap(EVT VT, AttributeList Attr) const override;
  bool isLegalAddressingMode(const DataLayout &DL, const AddrMode &AM, Type *Ty,
                             unsigned AddrSpace,
                             Instruction *I = nullptr) const override;
  bool shouldConsiderGEPOffsetSplit() const override { return true; }
  bool isFMAFasterThanFMulAndFAdd(const MachineFunction &,
                                  EVT VT) const override;
  bool isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const override;
  bool isSelectSupported(SelectSupportKind) const override { return false; }
  bool reduceSelectOfFPConstantLoads(EVT CmpOpVT) const override {
    return false;
  }
  ConstraintType getConstraintType(StringRef Constraint) const override;
  std::pair<unsigned, const TargetRegisterClass *>
  getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                               StringRef Constraint, MVT VT) const override;
  InlineAsm::ConstraintCode
  getInlineAsmMemConstraint(StringRef ConstraintCode) const override;
  void LowerAsmOperandForConstraint(SDValue Op, StringRef Constraint,
                                    std::vector<SDValue> &Ops,
                                    SelectionDAG &DAG) const override;
  bool getPreIndexedAddressParts(SDNode *N, SDValue &Base, SDValue &Offset,
                                 ISD::MemIndexedMode &AM,
                                 SelectionDAG &DAG) const override;
  bool getPostIndexedAddressParts(SDNode *N, SDNode *Op, SDValue &Base,
                                  SDValue &Offset, ISD::MemIndexedMode &AM,
                                  SelectionDAG &DAG) const override;
  bool allowsMisalignedMemoryAccesses(
      EVT VT, unsigned AddrSpace = 0, Align Alignment = Align(1),
      MachineMemOperand::Flags Flags = MachineMemOperand::MONone,
      unsigned *Fast = nullptr) const override;

  bool findOptimalMemOpLowering(LLVMContext &Context, std::vector<EVT> &MemOps,
                               unsigned Limit, const MemOp &Op, unsigned DstAS,
                               unsigned SrcAS, const AttributeList &Attrs,
                               EVT *LargestVT) const override;
  EVT getOptimalMemOpType(LLVMContext &Context, const MemOp &Op,
                          const AttributeList &FuncAttributes) const override;
};

} // namespace llvm

#endif
