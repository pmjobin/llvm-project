//===-- SHISelDAGToDAG.cpp - SH DAG instruction selection ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SH.h"
#include "SHConstantPoolValue.h"
#include "SHISelLowering.h"
#include "SHInstrInfo.h"
#include "SHLiteralPool.h"
#include "SHMachineFunctionInfo.h"
#include "SHSubtarget.h"
#include "SHTargetMachine.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicsSH.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

#define DEBUG_TYPE "sh-isel"
#define PASS_NAME "SH DAG->DAG Pattern Instruction Selection"

namespace {

static bool useSHNarrowDisplacementPseudos(const Function &F,
                                           CodeGenOptLevel Level) {
  // Keep compact displacements available across calls in size-optimized code.
  // The pseudos expose their R0 clobbers before register allocation.
  return F.hasOptSize();
}

class SHDAGToDAGISel : public SelectionDAGISel {
  const SHSubtarget *Subtarget = nullptr;
  bool UseNarrowDisplacementPseudos = false;
  const CodeGenOptLevel OriginalOptLevel;

  bool SelectFPUIndexedOffset(SDNode *N) {
    if (!Subtarget->hasSingleFPU())
      return false;
    const auto *Load = dyn_cast<LoadSDNode>(N);
    const auto *Store = dyn_cast<StoreSDNode>(N);
    const auto *Memory = dyn_cast<LSBaseSDNode>(N);
    if (!Memory || Memory->isAtomic() ||
        Memory->getAddressingMode() != ISD::UNINDEXED ||
        Memory->getMemoryVT() != MVT::f32)
      return false;
    if ((!Load || Load->getValueType(0) != MVT::f32) &&
        (!Store || Store->getValue().getValueType() != MVT::f32))
      return false;
    SDValue Address = stripAddressAssertions(Memory->getBasePtr());
    if (Address.getOpcode() != ISD::ADD ||
        Address.getOperand(0).getOpcode() == ISD::FrameIndex)
      return false;
    const auto *Offset = dyn_cast<ConstantSDNode>(Address.getOperand(1));
    if (!Offset || !isInt<8>(Offset->getSExtValue()))
      return false;
    SDLoc DL(N);
    // Keep the root in an ordinary GPR and constrain only this index use.
    // Sharing complete base-plus-offset values across blocks can otherwise
    // leave many addresses live, even though each offset fits one MOVri.
    SDValue Copy = CurDAG->getCopyToReg(Memory->getChain(), DL, SH::R0,
                                        Address.getOperand(1), SDValue());
    MachineSDNode *Result;
    if (Load) {
      SDValue Operands[] = {Address.getOperand(0), Copy, Copy.getValue(1)};
      Result = CurDAG->getMachineNode(SH::FMOV_S_LOAD_INDEXED, DL, MVT::f32,
                                      MVT::Other, Operands);
    } else {
      SDValue Operands[] = {Store->getValue(), Address.getOperand(0), Copy,
                            Copy.getValue(1)};
      Result = CurDAG->getMachineNode(SH::FMOV_S_STORE_INDEXED, DL, MVT::Other,
                                      Operands);
    }
    CurDAG->setNodeMemRefs(Result, {Memory->getMemOperand()});
    ReplaceNode(N, Result);
    return true;
  }

  bool SelectPostIncLoad(SDNode *N) {
    const auto *Load = dyn_cast<LoadSDNode>(N);
    if (!Load || Load->getAddressingMode() != ISD::POST_INC ||
        Load->getValueType(0) != MVT::i32)
      return false;
    EVT MemVT = Load->getMemoryVT();
    ISD::LoadExtType Ext = Load->getExtensionType();
    int64_t Offset = cast<ConstantSDNode>(Load->getOffset())->getSExtValue();
    unsigned Opcode;
    // Sign extension is a valid refinement when the upper bits are undef.
    // The address matcher controls profitability of new anyext writeback.
    if (MemVT == MVT::i8 &&
        (Ext == ISD::SEXTLOAD || Ext == ISD::ZEXTLOAD || Ext == ISD::EXTLOAD) &&
        Offset == 1)
      Opcode = SH::MOVB_load_postinc;
    else if (MemVT == MVT::i16 &&
             (Ext == ISD::SEXTLOAD || Ext == ISD::ZEXTLOAD ||
              Ext == ISD::EXTLOAD) &&
             Offset == 2)
      Opcode = SH::MOVW_load_postinc;
    else if (MemVT == MVT::i32 && Ext == ISD::NON_EXTLOAD && Offset == 4)
      Opcode = SH::MOVL_load_postinc;
    else
      return false;
    MachineSDNode *Result =
        CurDAG->getMachineNode(Opcode, SDLoc(N), MVT::i32, MVT::i32, MVT::Other,
                               Load->getBasePtr(), Load->getChain());
    CurDAG->setNodeMemRefs(Result, {Load->getMemOperand()});
    SDValue Value(Result, 0);
    if (Ext == ISD::ZEXTLOAD) {
      unsigned ExtendOpcode = MemVT == MVT::i8 ? SH::EXTUB : SH::EXTUW;
      Value = SDValue(
          CurDAG->getMachineNode(ExtendOpcode, SDLoc(N), MVT::i32, Value), 0);
    }
    ReplaceUses(SDValue(N, 0), Value);
    ReplaceUses(SDValue(N, 1), SDValue(Result, 1));
    ReplaceUses(SDValue(N, 2), SDValue(Result, 2));
    CurDAG->RemoveDeadNode(N);
    return true;
  }

  bool SelectPreDecStore(SDNode *N) {
    const auto *Store = dyn_cast<StoreSDNode>(N);
    if (!Store || Store->getAddressingMode() != ISD::PRE_DEC)
      return false;
    EVT MemVT = Store->getMemoryVT();
    int64_t Offset = cast<ConstantSDNode>(Store->getOffset())->getSExtValue();
    unsigned Opcode;
    if (MemVT == MVT::i8 && Offset == -1)
      Opcode = SH::MOVB_store_predec;
    else if (MemVT == MVT::i16 && Offset == -2)
      Opcode = SH::MOVW_store_predec;
    else if (MemVT == MVT::i32 && Offset == -4)
      Opcode = SH::MOVL_store_predec;
    else
      return false;
    MachineSDNode *Result = CurDAG->getMachineNode(
        Opcode, SDLoc(N), MVT::i32, MVT::Other, Store->getValue(),
        Store->getBasePtr(), Store->getChain());
    CurDAG->setNodeMemRefs(Result, {Store->getMemOperand()});
    ReplaceNode(N, Result);
    return true;
  }

  bool SelectOrderedFPBranch(SDNode *N) {
    SDValue Boolean;
    bool BranchOnTrue;
    SDValue BranchTarget;
    SDValue Chain;
    if (N->getOpcode() == ISD::BRCOND) {
      Boolean = N->getOperand(1);
      BranchOnTrue = true;
      BranchTarget = N->getOperand(2);
      Chain = N->getOperand(0);
    } else if (N->getOpcode() == SHISD::BT || N->getOpcode() == SHISD::BF) {
      SDValue Compare = N->getOperand(2);
      if (!Compare.hasOneUse())
        return false;
      if (Compare.getOpcode() == SHISD::TST &&
          Compare.getOperand(0) == Compare.getOperand(1)) {
        Boolean = Compare.getOperand(0);
        BranchOnTrue = N->getOpcode() == SHISD::BF;
      } else if (Compare.getOpcode() == SHISD::CMP_EQ) {
        Boolean = Compare.getOperand(0);
        SDValue ConstantValue = Compare.getOperand(1);
        if (isa<ConstantSDNode>(Boolean))
          std::swap(Boolean, ConstantValue);
        const auto *Constant = dyn_cast<ConstantSDNode>(ConstantValue);
        if (!Constant || (!Constant->isZero() && !Constant->isOne()))
          return false;
        BranchOnTrue = (N->getOpcode() == SHISD::BT) == Constant->isOne();
      } else {
        return false;
      }
      BranchTarget = N->getOperand(1);
      Chain = N->getOperand(0);
    } else {
      return false;
    }
    if (Boolean.getOpcode() != ISD::OR || !Boolean.hasOneUse())
      return false;

    SDValue Equal;
    SDValue Greater;
    for (SDValue Materialized : Boolean->ops()) {
      if (Materialized.getOpcode() != SHISD::MOVT || !Materialized.hasOneUse())
        return false;
      SDValue FPCompare = Materialized.getOperand(0);
      if (!FPCompare.hasOneUse())
        return false;
      switch (FPCompare.getOpcode()) {
      default:
        return false;
      case SHISD::FCMP_EQ_F32:
      case SHISD::FCMP_EQ_F64:
        if (Equal)
          return false;
        Equal = FPCompare;
        break;
      case SHISD::FCMP_GT_F32:
      case SHISD::FCMP_GT_F64:
        if (Greater)
          return false;
        Greater = FPCompare;
        break;
      }
    }
    if (!Equal || !Greater ||
        Equal.getOperand(0).getValueType() !=
            Greater.getOperand(0).getValueType())
      return false;
    bool SameOrder = Equal.getOperand(0) == Greater.getOperand(0) &&
                     Equal.getOperand(1) == Greater.getOperand(1);
    bool SwappedOrder = Equal.getOperand(0) == Greater.getOperand(1) &&
                        Equal.getOperand(1) == Greater.getOperand(0);
    if (!SameOrder && !SwappedOrder)
      return false;

    unsigned Opcode = Greater.getOpcode() == SHISD::FCMP_GT_F32
                          ? SH::BR_FP_OR_F32_PSEUDO
                          : SH::BR_FP_OR_F64_PSEUDO;
    SDLoc DL(N);
    SDValue BranchPredicate =
        CurDAG->getTargetConstant(BranchOnTrue, DL, MVT::i32);
    SDValue Operands[] = {Greater.getOperand(0), Greater.getOperand(1),
                          BranchPredicate, BranchTarget, Chain};
    CurDAG->SelectNodeTo(N, Opcode, MVT::Other, Operands);
    return true;
  }

  static SDNode *getSoleValueUser(SDValue Value) {
    if (!Value.hasOneUse())
      return nullptr;
    for (SDUse &Use : Value->uses())
      if (Use.getResNo() == Value.getResNo())
        return Use.getUser();
    return nullptr;
  }

  static bool isCopyToR0(const SDNode *N) {
    if (!N || N->getOpcode() != ISD::CopyToReg)
      return false;
    const auto *Reg = dyn_cast<RegisterSDNode>(N->getOperand(1));
    return Reg && Reg->getReg() == SH::R0;
  }

  static bool isCopyFromR0(SDValue Value) {
    if (Value.getOpcode() != ISD::CopyFromReg)
      return false;
    const auto *Reg = dyn_cast<RegisterSDNode>(Value.getOperand(1));
    return Reg && Reg->getReg() == SH::R0;
  }

  bool SelectCompareImmediate(SDNode *N) {
    if (N->getOpcode() != SHISD::CMP_EQ)
      return false;
    for (unsigned ConstantIndex : {0U, 1U}) {
      SDValue ConstantValue = N->getOperand(ConstantIndex);
      const auto *Constant = dyn_cast<ConstantSDNode>(ConstantValue);
      SDValue Value = N->getOperand(ConstantIndex ^ 1U);
      if (!Constant || !isInt<8>(Constant->getSExtValue()))
        continue;
      const Function &F = CurDAG->getMachineFunction().getFunction();
      if (!(isCopyFromR0(Value) && Value.hasOneUse()) &&
          !((F.hasOptSize() || F.hasMinSize()) && ConstantValue.hasOneUse() &&
            Value.getValueType() == MVT::i32))
        continue;
      // Keep the value in a GPR and expose the local R0 clobber. Constraining
      // an earlier call result to R0 can make its spill reload overlap a newer
      // call's still-live physical R0 result after scheduling.
      SDValue Operands[] = {
          Value, CurDAG->getSignedTargetConstant(Constant->getSExtValue(),
                                                 SDLoc(N), MVT::i32)};
      CurDAG->SelectNodeTo(N, SH::CMP_EQri_VALUE_PSEUDO, MVT::Glue, Operands);
      return true;
    }
    return false;
  }

  bool SelectMaskedZeroTest(SDNode *N) {
    if (N->getOpcode() != SHISD::TST || N->getNumOperands() != 2 ||
        N->getOperand(0) != N->getOperand(1))
      return false;
    SDValue Condition = N->getOperand(0);
    if (Condition.getOpcode() != ISD::AND ||
        !Condition->hasNUsesOfValue(2, Condition.getResNo()) ||
        Condition.getValueType() != MVT::i32)
      return false;
    if (!isa<ConstantSDNode>(Condition.getOperand(0)) &&
        !isa<ConstantSDNode>(Condition.getOperand(1))) {
      SDValue TestOperands[] = {Condition.getOperand(0),
                                Condition.getOperand(1)};
      CurDAG->SelectNodeTo(N, SH::TST, MVT::Glue, TestOperands);
      return true;
    }
    for (unsigned ConstantIndex : {0U, 1U}) {
      const auto *Mask =
          dyn_cast<ConstantSDNode>(Condition.getOperand(ConstantIndex));
      if (!Mask || Mask->isZero() || !isUInt<8>(Mask->getZExtValue()))
        continue;
      SDLoc DL(N);
      const Function &F = CurDAG->getMachineFunction().getFunction();
      if (TM.getOptLevel() != CodeGenOptLevel::None && !F.hasOptNone() &&
          !F.hasOptSize() && !F.hasMinSize()) {
        // Expose the architectural input before allocation. Gluing the
        // copy to the test preserves R0 while keeping the original T use.
        SDValue Copy = CurDAG->getCopyToReg(
            CurDAG->getEntryNode(), DL, SH::R0,
            Condition.getOperand(ConstantIndex ^ 1U), SDValue());
        SDValue Operands[] = {
            CurDAG->getTargetConstant(Mask->getZExtValue(), DL, MVT::i32), Copy,
            Copy.getValue(1)};
        CurDAG->SelectNodeTo(N, SH::TSTri, MVT::Glue, Operands);
        return true;
      }
      SDValue TestOperands[] = {
          Condition.getOperand(ConstantIndex ^ 1U),
          CurDAG->getTargetConstant(Mask->getZExtValue(), DL, MVT::i32)};
      CurDAG->SelectNodeTo(N, SH::TSTri_PSEUDO, MVT::Glue, TestOperands);
      return true;
    }
    return false;
  }

  static SDValue stripAddressAssertions(SDValue Address) {
    while (Address.getOpcode() == ISD::AssertSext ||
           Address.getOpcode() == ISD::AssertZext)
      Address = Address.getOperand(0);
    return Address;
  }

  bool getR0IndexedBase(SDValue Address, SDValue &Base, SDValue &IndexValue) {
    if (Address.getOpcode() != ISD::ADD)
      return false;
    for (unsigned Index : {0U, 1U}) {
      SDValue Candidate =
          stripAddressAssertions(Address.getOperand(Index ^ 1U));
      if (isCopyFromR0(Address.getOperand(Index)) &&
          Candidate.getOpcode() != ISD::FrameIndex &&
          isFoldableBase(Candidate) && !isCopyFromR0(Candidate)) {
        Base = Candidate;
        IndexValue = Address.getOperand(Index);
        return true;
      }
    }
    return false;
  }

  bool getSingleUseIndexedBase(SDValue Address, SDValue &Base,
                               SDValue &IndexValue) {
    if (Address.getOpcode() != ISD::ADD)
      return false;
    const Function &F = CurDAG->getMachineFunction().getFunction();
    bool AllowFrameBase = TM.getOptLevel() != CodeGenOptLevel::None &&
                          !F.hasOptNone() && !F.hasOptSize() && !F.hasMinSize();
    for (unsigned Index : {0U, 1U}) {
      SDValue CandidateIndex =
          stripAddressAssertions(Address.getOperand(Index));
      SDValue CandidateBase =
          stripAddressAssertions(Address.getOperand(Index ^ 1U));
      if (CandidateIndex.getValueType() != MVT::i32 ||
          !CandidateIndex.hasOneUse() ||
          CandidateIndex.getOpcode() == ISD::Constant ||
          CandidateIndex.getOpcode() == ISD::FrameIndex ||
          (CandidateBase.getOpcode() == ISD::FrameIndex && !AllowFrameBase) ||
          !isFoldableBase(CandidateBase))
        continue;
      // A frame base is materialized normally by LEA_FI. Constrain only
      // the single-use index to R0, so its producer can coalesce there.
      Base = CandidateBase;
      IndexValue = CandidateIndex;
      return true;
    }
    return false;
  }

  bool SelectR0Indexed(SDNode *N) {
    const auto *Load = dyn_cast<LoadSDNode>(N);
    if (Load && Load->getAddressingMode() == ISD::UNINDEXED &&
        !Load->isAtomic() && Load->getValueType(0) == MVT::i32) {
      EVT MemVT = Load->getMemoryVT();
      ISD::LoadExtType Ext = Load->getExtensionType();
      SDValue Base;
      SDValue IndexValue;
      if (((MemVT == MVT::i32 && Ext == ISD::NON_EXTLOAD) ||
           ((MemVT == MVT::i8 || MemVT == MVT::i16) &&
            (Ext == ISD::SEXTLOAD || Ext == ISD::ZEXTLOAD))) &&
          getR0IndexedBase(Load->getBasePtr(), Base, IndexValue)) {
        unsigned Opcode = MemVT == MVT::i8    ? SH::MOVB_load_indexed
                          : MemVT == MVT::i16 ? SH::MOVW_load_indexed
                                              : SH::MOVL_load_indexed;
        SDLoc DL(N);
        // The selected instruction has an implicit R0 use. Preserve the
        // data and chain dependency that produced the existing R0 value.
        SDValue IndexCopy = CurDAG->getCopyToReg(Load->getChain(), DL, SH::R0,
                                                 IndexValue, SDValue());
        SDValue Operands[] = {Base, IndexCopy, IndexCopy.getValue(1)};
        MachineSDNode *Result =
            CurDAG->getMachineNode(Opcode, DL, MVT::i32, MVT::Other, Operands);
        CurDAG->setNodeMemRefs(Result, {Load->getMemOperand()});
        SDValue Value(Result, 0);
        if (Ext == ISD::ZEXTLOAD) {
          unsigned ExtendOpcode = MemVT == MVT::i8 ? SH::EXTUB : SH::EXTUW;
          Value = SDValue(
              CurDAG->getMachineNode(ExtendOpcode, SDLoc(N), MVT::i32, Value),
              0);
        }
        ReplaceUses(SDValue(N, 0), Value);
        ReplaceUses(SDValue(N, 1), SDValue(Result, 1));
        CurDAG->RemoveDeadNode(N);
        return true;
      }
      if (((MemVT == MVT::i32 && Ext == ISD::NON_EXTLOAD) ||
           ((MemVT == MVT::i8 || MemVT == MVT::i16) &&
            (Ext == ISD::SEXTLOAD || Ext == ISD::ZEXTLOAD))) &&
          getSingleUseIndexedBase(Load->getBasePtr(), Base, IndexValue)) {
        unsigned Opcode = MemVT == MVT::i8    ? SH::MOVB_load_indexed
                          : MemVT == MVT::i16 ? SH::MOVW_load_indexed
                                              : SH::MOVL_load_indexed;
        SDLoc DL(N);
        SDValue IndexCopy = CurDAG->getCopyToReg(Load->getChain(), DL, SH::R0,
                                                 IndexValue, SDValue());
        SDValue Operands[] = {Base, IndexCopy, IndexCopy.getValue(1)};
        MachineSDNode *Result =
            CurDAG->getMachineNode(Opcode, DL, MVT::i32, MVT::Other, Operands);
        CurDAG->setNodeMemRefs(Result, {Load->getMemOperand()});
        SDValue Value(Result, 0);
        if (Ext == ISD::ZEXTLOAD) {
          unsigned ExtendOpcode = MemVT == MVT::i8 ? SH::EXTUB : SH::EXTUW;
          Value = SDValue(
              CurDAG->getMachineNode(ExtendOpcode, DL, MVT::i32, Value), 0);
        }
        ReplaceUses(SDValue(N, 0), Value);
        ReplaceUses(SDValue(N, 1), SDValue(Result, 1));
        CurDAG->RemoveDeadNode(N);
        return true;
      }
    }

    const auto *Store = dyn_cast<StoreSDNode>(N);
    if (!Store || Store->getAddressingMode() != ISD::UNINDEXED ||
        Store->isAtomic())
      return false;
    EVT MemVT = Store->getMemoryVT();
    SDValue Base;
    SDValue IndexValue;
    if ((MemVT != MVT::i8 && MemVT != MVT::i16 && MemVT != MVT::i32) ||
        (!getR0IndexedBase(Store->getBasePtr(), Base, IndexValue) &&
         !getSingleUseIndexedBase(Store->getBasePtr(), Base, IndexValue)))
      return false;
    unsigned Opcode = MemVT == MVT::i8    ? SH::MOVB_store_indexed
                      : MemVT == MVT::i16 ? SH::MOVW_store_indexed
                                          : SH::MOVL_store_indexed;
    SDLoc DL(N);
    // Copying a short-lived index to r0 still replaces the separate
    // address copy and add required by a register-indirect memory access.
    SDValue IndexCopy = CurDAG->getCopyToReg(Store->getChain(), DL, SH::R0,
                                             IndexValue, SDValue());
    SDValue Operands[] = {Store->getValue(), Base, IndexCopy,
                          IndexCopy.getValue(1)};
    MachineSDNode *Result =
        CurDAG->getMachineNode(Opcode, DL, MVT::Other, Operands);
    CurDAG->setNodeMemRefs(Result, {Store->getMemOperand()});
    ReplaceNode(N, Result);
    return true;
  }

  bool getNarrowDisplacement(SDValue Address, EVT MemVT, SDValue &Base,
                             SDValue &Disp) {
    if (MemVT != MVT::i8 && MemVT != MVT::i16)
      return false;
    if (Address.getOpcode() != ISD::ADD ||
        Address.getOperand(0).getOpcode() == ISD::FrameIndex)
      return false;
    const auto *Offset = dyn_cast<ConstantSDNode>(Address.getOperand(1));
    if (!Offset || !isFoldableBase(Address.getOperand(0)))
      return false;
    int64_t ByteOffset = Offset->getSExtValue();
    if ((MemVT == MVT::i8 && (ByteOffset < 1 || ByteOffset > 15)) ||
        (MemVT == MVT::i16 &&
         (ByteOffset < 2 || ByteOffset > 30 || ByteOffset % 2 != 0)))
      return false;
    Base = stripAddressAssertions(Address.getOperand(0));
    Disp = CurDAG->getTargetConstant(ByteOffset, SDLoc(Address), MVT::i32);
    return true;
  }

  bool SelectNarrowDisplacement(SDNode *N) {
    const auto *Load = dyn_cast<LoadSDNode>(N);
    bool IsR0Load = Load && isCopyToR0(getSoleValueUser(SDValue(N, 0)));
    if (Load && Load->getAddressingMode() == ISD::UNINDEXED &&
        !Load->isAtomic() && Load->getValueType(0) == MVT::i32 &&
        (IsR0Load || UseNarrowDisplacementPseudos)) {
      EVT MemVT = Load->getMemoryVT();
      ISD::LoadExtType Ext = Load->getExtensionType();
      SDValue Base;
      SDValue Disp;
      if ((Ext == ISD::EXTLOAD || Ext == ISD::SEXTLOAD ||
           Ext == ISD::ZEXTLOAD) &&
          getNarrowDisplacement(Load->getBasePtr(), MemVT, Base, Disp)) {
        // Keep the r0 clobber explicit before allocation. A virtual
        // R0Reg cannot be spilled inside another physical r0 range.
        // A direct return has no intervening r0 use and stays compact.
        SDNode *Copy = getSoleValueUser(SDValue(N, 0));
        bool IsReturn =
            IsR0Load && llvm::all_of(Copy->users(), [](SDNode *User) {
              return User->getOpcode() == SHISD::RET_GLUE ||
                     (User->isMachineOpcode() &&
                      User->getMachineOpcode() == SH::RTS);
            });
        bool UsePseudo = !IsReturn;
        unsigned LoadOpcode =
            MemVT == MVT::i8
                ? (UsePseudo ? SH::MOVB_load_frame : SH::MOVB_load_disp)
                : (UsePseudo ? SH::MOVW_load_frame : SH::MOVW_load_disp);
        MachineSDNode *Result =
            CurDAG->getMachineNode(LoadOpcode, SDLoc(N), MVT::i32, MVT::Other,
                                   Base, Disp, Load->getChain());
        CurDAG->setNodeMemRefs(Result, {Load->getMemOperand()});
        SDValue Value(Result, 0);
        if (Ext == ISD::ZEXTLOAD) {
          unsigned ExtendOpcode = MemVT == MVT::i8 ? SH::EXTUB : SH::EXTUW;
          Value = SDValue(
              CurDAG->getMachineNode(ExtendOpcode, SDLoc(N), MVT::i32, Value),
              0);
        }
        ReplaceUses(SDValue(N, 0), Value);
        ReplaceUses(SDValue(N, 1), SDValue(Result, 1));
        CurDAG->RemoveDeadNode(N);
        return true;
      }
    }

    const auto *Store = dyn_cast<StoreSDNode>(N);
    bool IsR0Store = Store && isCopyFromR0(Store->getValue());
    if (!Store || Store->getAddressingMode() != ISD::UNINDEXED ||
        Store->isAtomic() || (!IsR0Store && !UseNarrowDisplacementPseudos))
      return false;
    EVT MemVT = Store->getMemoryVT();
    SDValue Base;
    SDValue Disp;
    if ((MemVT != MVT::i8 && MemVT != MVT::i16) ||
        !getNarrowDisplacement(Store->getBasePtr(), MemVT, Base, Disp))
      return false;
    bool UsePseudo = !IsR0Store;
    unsigned Opcode =
        MemVT == MVT::i8
            ? (UsePseudo ? SH::MOVB_store_frame : SH::MOVB_store_disp)
            : (UsePseudo ? SH::MOVW_store_frame : SH::MOVW_store_disp);
    if (IsR0Store) {
      // Keep the fixed-register constraint local to this store. Giving
      // the value R0Reg class can constrain other uses, including a PHI
      // input later used by an indexed store with a different R0 index.
      SDLoc DL(N);
      SDValue Copy = CurDAG->getCopyToReg(Store->getChain(), DL, SH::R0,
                                          Store->getValue(), SDValue());
      SDValue Operands[] = {CurDAG->getRegister(SH::R0, MVT::i32), Base, Disp,
                            Copy, Copy.getValue(1)};
      MachineSDNode *Result =
          CurDAG->getMachineNode(Opcode, DL, MVT::Other, Operands);
      CurDAG->setNodeMemRefs(Result, {Store->getMemOperand()});
      ReplaceNode(N, Result);
      return true;
    }
    SDValue Operands[] = {Store->getValue(), Base, Disp, Store->getChain()};
    MachineSDNode *Result =
        CurDAG->getMachineNode(Opcode, SDLoc(N), MVT::Other, Operands);
    CurDAG->setNodeMemRefs(Result, {Store->getMemOperand()});
    ReplaceNode(N, Result);
    return true;
  }

  bool isFoldableBase(SDValue Base) const {
    Base = stripAddressAssertions(Base);
    switch (Base.getOpcode()) {
    case ISD::FrameIndex:
    case ISD::CopyFromReg:
    case ISD::LOAD:
    case ISD::STORE:
    case ISD::Register:
    case ISD::ADD:
    case ISD::SUB:
    case ISD::AND:
    case ISD::OR:
    case SHISD::PIC_ADDRESS:
    case SHISD::PIC_GOTOFF:
    case SHISD::TLS_IE:
    case SHISD::SELECT:
      return Base.getValueType() == MVT::i32;
    default:
      return false;
    }
  }

  bool SelectNarrowFrameAddr(SDValue Addr, SDValue &Base, SDValue &Disp) {
    int64_t ByteDisp = 0;
    SDValue CandidateBase = Addr;
    if (Addr.getOpcode() == ISD::ADD) {
      const auto *Offset = dyn_cast<ConstantSDNode>(Addr.getOperand(1));
      if (!Offset)
        return false;
      ByteDisp = Offset->getSExtValue();
      CandidateBase = Addr.getOperand(0);
    }
    if (CandidateBase.getOpcode() != ISD::FrameIndex)
      return false;

    Base = CurDAG->getTargetFrameIndex(
        cast<FrameIndexSDNode>(CandidateBase)->getIndex(), MVT::i32);
    Disp = CurDAG->getSignedTargetConstant(ByteDisp, SDLoc(Addr), MVT::i32);
    return true;
  }

public:
  explicit SHDAGToDAGISel(SHTargetMachine &TM)
      : SelectionDAGISel(TM), OriginalOptLevel(TM.getOptLevel()) {}

  bool SelectInlineAsmMemoryOperand(const SDValue &Op,
                                    InlineAsm::ConstraintCode ConstraintCode,
                                    std::vector<SDValue> &OutOps) override {
    if (ConstraintCode != InlineAsm::ConstraintCode::m &&
        ConstraintCode != InlineAsm::ConstraintCode::o &&
        ConstraintCode != InlineAsm::ConstraintCode::v &&
        ConstraintCode != InlineAsm::ConstraintCode::p)
      return true;
    OutOps.push_back(Op);
    return false;
  }

  bool getSH2AExtendedDisplacement(SDValue Address, EVT MemVT, SDValue &Base,
                                   SDValue &Disp) {
    if (!Subtarget->hasSH2A() || Address.getOpcode() != ISD::ADD ||
        Address.getOperand(0).getOpcode() == ISD::FrameIndex)
      return false;
    const auto *Offset = dyn_cast<ConstantSDNode>(Address.getOperand(1));
    if (!Offset || !isFoldableBase(Address.getOperand(0)))
      return false;
    int64_t ByteOffset = Offset->getSExtValue();
    unsigned Scale = MemVT == MVT::i16                        ? 2
                     : MemVT == MVT::i32 || MemVT == MVT::f32 ? 4
                     : MemVT == MVT::f64                      ? 8
                                                              : 1;
    if ((MemVT != MVT::i8 && MemVT != MVT::i16 && MemVT != MVT::i32 &&
         MemVT != MVT::f32 && MemVT != MVT::f64) ||
        ByteOffset <= 0 || ByteOffset > 4095 * Scale ||
        ByteOffset % Scale != 0 || (MemVT == MVT::i32 && ByteOffset <= 60))
      return false;
    Base = stripAddressAssertions(Address.getOperand(0));
    Disp = CurDAG->getTargetConstant(ByteOffset, SDLoc(Address), MVT::i32);
    return true;
  }

  bool SelectSH2AExtendedDisplacement(SDNode *N) {
    if (const auto *Load = dyn_cast<LoadSDNode>(N)) {
      if (Load->getAddressingMode() != ISD::UNINDEXED || Load->isAtomic())
        return false;
      EVT MemVT = Load->getMemoryVT();
      EVT ValueVT = Load->getValueType(0);
      ISD::LoadExtType Ext = Load->getExtensionType();
      SDValue Base;
      SDValue Disp;
      if (!getSH2AExtendedDisplacement(Load->getBasePtr(), MemVT, Base, Disp))
        return false;
      unsigned Opcode;
      if (MemVT == MVT::i8 && Ext == ISD::SEXTLOAD)
        Opcode = SH::MOVB_LOAD_DISP12;
      else if (MemVT == MVT::i8 && Ext == ISD::ZEXTLOAD)
        Opcode = SH::MOVUB_LOAD_DISP12;
      else if (MemVT == MVT::i16 && Ext == ISD::SEXTLOAD)
        Opcode = SH::MOVW_LOAD_DISP12;
      else if (MemVT == MVT::i16 && Ext == ISD::ZEXTLOAD)
        Opcode = SH::MOVUW_LOAD_DISP12;
      else if (MemVT == MVT::i32 && Ext == ISD::NON_EXTLOAD)
        Opcode = SH::MOVL_LOAD_DISP12;
      else if (ValueVT == MVT::f32 && MemVT == MVT::f32 &&
               Ext == ISD::NON_EXTLOAD && Subtarget->hasSingleFPU())
        Opcode = SH::FMOV_S_LOAD_DISP12;
      else if (ValueVT == MVT::f64 && MemVT == MVT::f64 &&
               Ext == ISD::NON_EXTLOAD && Subtarget->hasDoubleFPU() &&
               Subtarget->hasFPUSizeMode())
        Opcode = SH::FMOV_D_LOAD_DISP12;
      else
        return false;
      MachineSDNode *Result = CurDAG->getMachineNode(
          Opcode, SDLoc(N), ValueVT, MVT::Other, Base, Disp, Load->getChain());
      CurDAG->setNodeMemRefs(Result, {Load->getMemOperand()});
      ReplaceNode(N, Result);
      return true;
    }

    const auto *Store = dyn_cast<StoreSDNode>(N);
    if (!Store || Store->getAddressingMode() != ISD::UNINDEXED ||
        Store->isAtomic())
      return false;
    EVT MemVT = Store->getMemoryVT();
    SDValue Base;
    SDValue Disp;
    if (!getSH2AExtendedDisplacement(Store->getBasePtr(), MemVT, Base, Disp))
      return false;
    unsigned Opcode;
    if (MemVT == MVT::i8)
      Opcode = SH::MOVB_STORE_DISP12;
    else if (MemVT == MVT::i16)
      Opcode = SH::MOVW_STORE_DISP12;
    else if (MemVT == MVT::i32)
      Opcode = SH::MOVL_STORE_DISP12;
    else if (MemVT == MVT::f32 && Subtarget->hasSingleFPU())
      Opcode = SH::FMOV_S_STORE_DISP12;
    else if (MemVT == MVT::f64 && Subtarget->hasDoubleFPU() &&
             Subtarget->hasFPUSizeMode())
      Opcode = SH::FMOV_D_STORE_DISP12;
    else
      return false;
    SDValue Operands[] = {Store->getValue(), Base, Disp, Store->getChain()};
    MachineSDNode *Result =
        CurDAG->getMachineNode(Opcode, SDLoc(N), MVT::Other, Operands);
    CurDAG->setNodeMemRefs(Result, {Store->getMemOperand()});
    ReplaceNode(N, Result);
    return true;
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    Subtarget = &MF.getSubtarget<SHSubtarget>();
    UseNarrowDisplacementPseudos =
        useSHNarrowDisplacementPseudos(MF.getFunction(), OriginalOptLevel);
    bool Changed = SelectionDAGISel::runOnMachineFunction(MF);
    SHMachineFunctionInfo &FuncInfo = *MF.getInfo<SHMachineFunctionInfo>();
    if (!FuncInfo.usesPICBase())
      return Changed;
    int CPI = FuncInfo.getPICBaseCPI();
    if (CPI < 0 || static_cast<unsigned>(CPI) >=
                       MF.getConstantPool()->getConstants().size())
      report_fatal_error("SH PIC base has an invalid constant-pool entry");
    const MachineConstantPoolEntry &PICBaseEntry =
        MF.getConstantPool()->getConstants()[CPI];
    if (!PICBaseEntry.isMachineConstantPoolEntry())
      report_fatal_error("SH PIC base requires a target GOTPC constant");
    const auto *PICBase =
        static_cast<const SHConstantPoolValue *>(PICBaseEntry.Val.MachineCPVal);
    if (!PICBase->isExternalSymbol() ||
        PICBase->getExternalSymbol() != "_GLOBAL_OFFSET_TABLE_" ||
        PICBase->getModifier() != SHConstantPoolValue::Modifier::GOTPC ||
        PICBase->getAddend() != 0)
      report_fatal_error("SH PIC base requires a target GOTPC constant");
    MachineBasicBlock &Entry = MF.front();
    const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
    MachineMemOperand *MMO = MF.getMachineMemOperand(
        MachinePointerInfo::getConstantPool(MF),
        MachineMemOperand::MOLoad | MachineMemOperand::MODereferenceable |
            MachineMemOperand::MOInvariant,
        4, Align(4));
    BuildMI(Entry, Entry.begin(), DebugLoc(), TII.get(SH::SH_PIC_SETUP))
        .addConstantPoolIndex(CPI)
        .addImm(SHUnassignedLiteralIsland)
        .addMemOperand(MMO);
    return true;
  }

  bool SelectAddrReg(SDValue Addr, SDValue &Base) {
    Addr = stripAddressAssertions(Addr);
    if (Addr.getOpcode() == ISD::Constant) {
      Base = Addr;
      return true;
    }
    if (Addr.getOpcode() == ISD::ADD) {
      const auto *Offset = dyn_cast<ConstantSDNode>(Addr.getOperand(1));
      if (Offset && Offset->isZero() && isFoldableBase(Addr.getOperand(0))) {
        Base = Addr.getOperand(0);
        return true;
      }
      if (Offset && isFoldableBase(Addr.getOperand(0))) {
        int64_t ByteDisp = Offset->getSExtValue();
        if (ByteDisp < 0 || ByteDisp > 60 || ByteDisp % 4 != 0) {
          Base = Addr;
          return true;
        }
      }
    }
    // Frame indices and literal-pool operands have dedicated patterns. Every
    // other legal pointer producer can supply an ordinary GPR address.
    if (Addr.getValueType() != MVT::i32 ||
        Addr.getOpcode() == ISD::FrameIndex ||
        Addr.getOpcode() == ISD::TargetConstantPool)
      return false;
    Base = Addr;
    return true;
  }

  bool SelectAddrDisp(SDValue Addr, SDValue &Base, SDValue &Disp) {
    Addr = stripAddressAssertions(Addr);
    int64_t ByteDisp = 0;
    SDValue CandidateBase = Addr;

    if (Addr.getOpcode() == ISD::ADD) {
      const auto *Offset = dyn_cast<ConstantSDNode>(Addr.getOperand(1));
      if (!Offset)
        return false;
      ByteDisp = Offset->getSExtValue();
      CandidateBase = stripAddressAssertions(Addr.getOperand(0));
    }

    bool IsFrameIndex = CandidateBase.getOpcode() == ISD::FrameIndex;
    if (!isFoldableBase(CandidateBase) || (!IsFrameIndex && ByteDisp == 0) ||
        ByteDisp < 0 || ByteDisp > 60 || ByteDisp % 4 != 0)
      return false;

    if (IsFrameIndex)
      Base = CurDAG->getTargetFrameIndex(
          cast<FrameIndexSDNode>(CandidateBase)->getIndex(), MVT::i32);
    else
      Base = CandidateBase;
    Disp = CurDAG->getTargetConstant(ByteDisp, SDLoc(Addr), MVT::i32);
    return true;
  }

  bool SelectFPUAddrReg(SDValue Addr, SDValue &Base) {
    Addr = stripAddressAssertions(Addr);
    if (Addr.getValueType() != MVT::i32 ||
        Addr.getOpcode() == ISD::FrameIndex ||
        Addr.getOpcode() == ISD::TargetConstantPool)
      return false;
    Base = Addr;
    return true;
  }

  bool SelectFPUFrameAddr(SDValue Addr, SDValue &Base, SDValue &Disp) {
    int64_t ByteDisp = 0;
    SDValue CandidateBase = Addr;
    if (Addr.getOpcode() == ISD::ADD) {
      const auto *Offset = dyn_cast<ConstantSDNode>(Addr.getOperand(1));
      if (!Offset)
        return false;
      ByteDisp = Offset->getSExtValue();
      CandidateBase = Addr.getOperand(0);
    }
    if (CandidateBase.getOpcode() != ISD::FrameIndex)
      return false;
    Base = CurDAG->getTargetFrameIndex(
        cast<FrameIndexSDNode>(CandidateBase)->getIndex(), MVT::i32);
    Disp = CurDAG->getSignedTargetConstant(ByteDisp, SDLoc(Addr), MVT::i32);
    return true;
  }

  bool SelectNarrowAddrReg(SDValue Addr, SDValue &Base) {
    Addr = stripAddressAssertions(Addr);
    if (Addr.getValueType() != MVT::i32 ||
        Addr.getOpcode() == ISD::FrameIndex ||
        Addr.getOpcode() == ISD::TargetConstantPool)
      return false;
    if (Addr.getOpcode() == ISD::ADD &&
        Addr.getOperand(0).getOpcode() == ISD::FrameIndex &&
        isa<ConstantSDNode>(Addr.getOperand(1)))
      return false;
    Base = Addr;
    return true;
  }

  bool SelectByteFrameAddr(SDValue Addr, SDValue &Base, SDValue &Disp) {
    return SelectNarrowFrameAddr(Addr, Base, Disp);
  }

  bool SelectWordFrameAddr(SDValue Addr, SDValue &Base, SDValue &Disp) {
    return SelectNarrowFrameAddr(Addr, Base, Disp);
  }

  void Select(SDNode *N) override {
    if (N->isMachineOpcode()) {
      N->setNodeId(-1);
      return;
    }
    if (SelectFPUIndexedOffset(N) || SelectPostIncLoad(N) ||
        SelectPreDecStore(N) || SelectR0Indexed(N) ||
        SelectNarrowDisplacement(N) || SelectSH2AExtendedDisplacement(N) ||
        SelectCompareImmediate(N) || SelectMaskedZeroTest(N) ||
        SelectOrderedFPBranch(N))
      return;
    if (N->getOpcode() == ISD::BRCOND) {
      SDValue Condition = N->getOperand(1);
      SDLoc DL(N);
      SDValue TestOperands[] = {Condition, Condition};
      MachineSDNode *Test =
          CurDAG->getMachineNode(SH::TST, DL, MVT::Glue, TestOperands);
      SDValue BranchOperands[] = {N->getOperand(2), N->getOperand(0),
                                 SDValue(Test, 0)};
      CurDAG->SelectNodeTo(N, SH::BF, MVT::Other, BranchOperands);
      return;
    }
    if (N->getOpcode() == SHISD::SELECT_CC) {
      SDValue Operands[] = {N->getOperand(0), N->getOperand(1),
                            N->getOperand(2), N->getOperand(3),
                            N->getOperand(4)};
      CurDAG->SelectNodeTo(N, SH::SELECT_CC_PSEUDO, MVT::i32, Operands);
      return;
    }
    if (N->getOpcode() == SHISD::TLS_CALL) {
      if (N->getNumOperands() != 4 ||
          N->getOperand(1).getOpcode() != ISD::TargetConstantPool ||
          N->getOperand(2).getOpcode() != ISD::TargetConstantPool ||
          N->getOperand(3).getOpcode() != ISD::RegisterMask)
        report_fatal_error("SH TLS resolver call has malformed operands");
      SDLoc DL(N);
      SDValue Operands[] = {N->getOperand(1),
                            CurDAG->getSignedTargetConstant(
                                SHUnassignedLiteralIsland, DL, MVT::i32),
                            N->getOperand(2),
                            CurDAG->getSignedTargetConstant(
                                SHUnassignedLiteralIsland, DL, MVT::i32),
                            N->getOperand(3),
                            N->getOperand(0)};
      MachineMemOperand *MMO = cast<MemIntrinsicSDNode>(N)->getMemOperand();
      MachineSDNode *Call = cast<MachineSDNode>(CurDAG->SelectNodeTo(
          N, SH::SH_TLS_CALL, MVT::Other, MVT::Glue, Operands));
      MachineMemOperand *MMOs[] = {MMO, MMO};
      CurDAG->setNodeMemRefs(Call, MMOs);
      return;
    }
    if (N->getOpcode() == SHISD::TLS_IE) {
      if (N->getNumOperands() != 2 ||
          N->getOperand(1).getOpcode() != ISD::TargetConstantPool)
        report_fatal_error(
            "SH initial-exec TLS address has malformed operands");
      SDLoc DL(N);
      SDValue Operands[] = {N->getOperand(1),
                            CurDAG->getSignedTargetConstant(
                                SHUnassignedLiteralIsland, DL, MVT::i32),
                            N->getOperand(0)};
      MachineMemOperand *GOTMMO = cast<MemIntrinsicSDNode>(N)->getMemOperand();
      MachineSDNode *IE = cast<MachineSDNode>(CurDAG->SelectNodeTo(
          N, SH::SH_TLS_IE, MVT::i32, MVT::Other, Operands));
      MachineFunction &MF = CurDAG->getMachineFunction();
      MachineMemOperand *LiteralMMO = MF.getMachineMemOperand(
          MachinePointerInfo::getConstantPool(MF),
          MachineMemOperand::MOLoad | MachineMemOperand::MODereferenceable |
              MachineMemOperand::MOInvariant,
          4, Align(4));
      MachineMemOperand *MMOs[] = {LiteralMMO, GOTMMO};
      CurDAG->setNodeMemRefs(IE, MMOs);
      return;
    }
    if (N->getOpcode() == ISD::BUILD_VECTOR &&
        N->getValueType(0) == MVT::v4f32) {
      SmallVector<SDValue, 9> Ops;
      Ops.push_back(
          CurDAG->getTargetConstant(SH::FVRegClassID, SDLoc(N), MVT::i32));
      for (unsigned I = 0; I != 4; ++I) {
        Ops.push_back(N->getOperand(I));
        Ops.push_back(
            CurDAG->getTargetConstant(SH::sub_fpu32_0 + I, SDLoc(N), MVT::i32));
      }
      CurDAG->SelectNodeTo(N, TargetOpcode::REG_SEQUENCE, MVT::v4f32, Ops);
      return;
    }
    if (auto *Mem = dyn_cast<LSBaseSDNode>(N)) {
      if (Mem->getMemoryVT() == MVT::v4f32) {
        auto *Load = dyn_cast<LoadSDNode>(N);
        bool Pair = Mem->getAlign() >= Align(8);
        unsigned Opcode =
            Load ? (Pair ? SH::FMOV_V_LOAD_PAIR : SH::FMOV_V_LOAD)
                 : (Pair ? SH::FMOV_V_STORE_PAIR : SH::FMOV_V_STORE);
        SmallVector<SDValue, 3> Ops;
        if (!Load)
          Ops.push_back(cast<StoreSDNode>(N)->getValue());
        Ops.push_back(Mem->getBasePtr());
        Ops.push_back(Mem->getChain());
        auto VTs = Load ? CurDAG->getVTList(MVT::v4f32, MVT::i32, MVT::Other)
                        : CurDAG->getVTList(MVT::i32, MVT::Other);
        auto *V = CurDAG->getMachineNode(Opcode, SDLoc(N), VTs, Ops);
        CurDAG->setNodeMemRefs(V, {Mem->getMemOperand()});
        if (Load)
          ReplaceUses(SDValue(N, 0), SDValue(V, 0));
        ReplaceUses(SDValue(N, Load ? 1 : 0), SDValue(V, Load ? 2 : 1));
        CurDAG->RemoveDeadNode(N);
        return;
      }
    }
    if (N->getOpcode() == ISD::INTRINSIC_W_CHAIN ||
        N->getOpcode() == ISD::INTRINSIC_VOID) {
      unsigned ID = N->getConstantOperandVal(1);
      if (ID == Intrinsic::sh_get_thread_pointer ||
          ID == Intrinsic::sh_set_thread_pointer ||
          ID == Intrinsic::sh_get_fpscr || ID == Intrinsic::sh_set_fpscr) {
        bool FPSCR =
            ID == Intrinsic::sh_get_fpscr || ID == Intrinsic::sh_set_fpscr;
        if (FPSCR && !Subtarget->hasSingleFPU())
          report_fatal_error(
              "SH FPSCR control intrinsic requires an enabled FPU");
        if (FPSCR && CurDAG->getMachineFunction()
                         .getInfo<SHMachineFunctionInfo>()
                         ->isHandler())
          report_fatal_error(
              "SH FPSCR control intrinsic is unsupported in handlers");
        SDLoc DL(N);
        if (ID == Intrinsic::sh_get_thread_pointer ||
            ID == Intrinsic::sh_get_fpscr) {
          CurDAG->SelectNodeTo(N, FPSCR ? SH::STS_FPSCR : SH::STC_GBR,
                               CurDAG->getVTList(MVT::i32, MVT::Other),
                               {N->getOperand(0)});
        } else if (!FPSCR) {
          CurDAG->SelectNodeTo(N, SH::LDC_GBR, MVT::Other,
                               {N->getOperand(2), N->getOperand(0)});
        } else {
          auto *Set = CurDAG->getMachineNode(
              SH::SET_FPSCR_FIELDS, DL,
              CurDAG->getVTList(MVT::i32, MVT::i32, MVT::Other),
              {N->getOperand(2), N->getOperand(0)});
          ReplaceUses(SDValue(N, 0), SDValue(Set, 2));
          CurDAG->RemoveDeadNode(N);
        }
        return;
      }
      if (ID == Intrinsic::sh_fsca || ID == Intrinsic::sh_fsrra ||
          ID == Intrinsic::sh_fipr || ID == Intrinsic::sh_ftrv ||
          ID == Intrinsic::sh_ftrv4 || ID == Intrinsic::sh_ftrv_array ||
          ID == Intrinsic::sh_ftrv_compose) {
        bool Approx = ID == Intrinsic::sh_fsca || ID == Intrinsic::sh_fsrra;
        if ((Approx && !(Subtarget->hasSH4A() && Subtarget->hasSingleFPU())) ||
            (!Approx && !Subtarget->hasFPUVector()))
          report_fatal_error(
              "SH graphics intrinsic requires an unsupported target feature");
        MachineFunction &MF = CurDAG->getMachineFunction();
        if (MF.getInfo<SHMachineFunctionInfo>()->isHandler() ||
            MF.getFunction().hasFnAttribute(Attribute::StrictFP))
          report_fatal_error("SH graphics intrinsic is unsupported in handlers "
                             "or strict FP functions");
        SDLoc DL(N);
        auto Extract = [&](SDValue V, unsigned Sub) {
          return SDValue(CurDAG->getMachineNode(
                             TargetOpcode::EXTRACT_SUBREG, DL, MVT::f32, V,
                             CurDAG->getTargetConstant(Sub, DL, MVT::i32)),
                         0);
        };
        if (ID == Intrinsic::sh_fsca) {
          auto *V = CurDAG->getMachineNode(
              SH::FSCA_VALUE, DL, CurDAG->getVTList(MVT::Untyped, MVT::Other),
              {N->getOperand(2), N->getOperand(0)});
          ReplaceUses(SDValue(N, 0), Extract(SDValue(V, 0), SH::sub_fpu32_0));
          ReplaceUses(SDValue(N, 1), Extract(SDValue(V, 0), SH::sub_fpu32_1));
          ReplaceUses(SDValue(N, 2), SDValue(V, 1));
        } else if (ID == Intrinsic::sh_fsrra || ID == Intrinsic::sh_fipr) {
          bool Dot = ID == Intrinsic::sh_fipr;
          // A native vector construction can keep several whole destructive
          // FV results alive merely to use one lane from each. Expose scalars
          // there, while retaining the cheaper coalescable extraction for an
          // immediate scalar consumer (store, reduction or function return).
          bool PackedDot = Dot && llvm::any_of(N->uses(), [](const SDUse &Use) {
                             if (Use.getResNo() != 0)
                               return false;
                             const SDNode *User = Use.getUser();
                             return User->getValueType(0) == MVT::v4f32 &&
                                    (User->getOpcode() == ISD::BUILD_VECTOR ||
                                     (User->isMachineOpcode() &&
                                      User->getMachineOpcode() ==
                                          TargetOpcode::REG_SEQUENCE));
                           });
          SmallVector<SDValue, 3> Ops{N->getOperand(2)};
          if (Dot)
            Ops.push_back(N->getOperand(3));
          Ops.push_back(N->getOperand(0));
          auto *V = CurDAG->getMachineNode(
              PackedDot ? SH::FIPR_VALUE
              : Dot     ? SH::FIPR
                        : SH::FSRRA,
              DL,
              PackedDot
                  ? CurDAG->getVTList(MVT::f32, MVT::v4f32, MVT::Other)
                  : CurDAG->getVTList(Dot ? MVT::v4f32 : MVT::f32, MVT::Other),
              Ops);
          ReplaceUses(SDValue(N, 0),
                      Dot && !PackedDot
                          ? Extract(SDValue(V, 0), SH::sub_fpu32_3)
                          : SDValue(V, 0));
          ReplaceUses(SDValue(N, 1), SDValue(V, PackedDot ? 2 : 1));
        } else {
          bool Batch = ID != Intrinsic::sh_ftrv;
          bool Counted = ID == Intrinsic::sh_ftrv_array ||
                         ID == Intrinsic::sh_ftrv_compose;
          if (Counted)
            if (auto *Count = dyn_cast<ConstantSDNode>(N->getOperand(4));
                Count && Count->isZero()) {
              ReplaceUses(SDValue(N, 0), N->getOperand(0));
              CurDAG->RemoveDeadNode(N);
              return;
            }
          // Eight-byte alignment is required by paired FMOV. The private slot
          // holds the preserved tail and, for an align4 matrix, a safe copy.
          auto MMOs = cast<MemIntrinsicSDNode>(N)->memoperands();
          unsigned PrivateBytes =
              MMOs[Batch ? 2 : 0]->getAlign() >= Align(8) ? 16 : 80;
          int FI = MF.getFrameInfo().CreateStackObject(PrivateBytes, Align(8),
                                                       false);
          SDValue Save =
              SDValue(CurDAG->getMachineNode(
                          SH::LEA_FI, DL, MVT::i32,
                          CurDAG->getTargetFrameIndex(FI, MVT::i32),
                          CurDAG->getTargetConstant(0, DL, MVT::i32)),
                      0);
          SmallVector<SDValue, 5> Ops;
          if (Counted) {
            Ops.append({N->getOperand(2), N->getOperand(3), N->getOperand(4),
                        N->getOperand(5)});
            if (ID == Intrinsic::sh_ftrv_compose)
              Ops.push_back(N->getOperand(6));
          } else if (Batch)
            Ops.append({N->getOperand(2), N->getOperand(3), N->getOperand(4)});
          else
            Ops.append({N->getOperand(3), N->getOperand(2)});
          Ops.push_back(Save);
          Ops.push_back(N->getOperand(0));
          unsigned Opcode = ID == Intrinsic::sh_ftrv_compose ? SH::FTRV_COMPOSE
                            : Counted                        ? SH::FTRV_ARRAY
                            : Batch                          ? SH::FTRV4_MEM
                                                             : SH::FTRV_MEM;
          auto *V = CurDAG->getMachineNode(
              Opcode, DL,
              Batch ? CurDAG->getVTList(MVT::Other)
                    : CurDAG->getVTList({MVT::v4f32, MVT::i32, MVT::i32,
                                         MVT::i32, MVT::Other}),
              Ops);
          SmallVector<MachineMemOperand *, 5> Refs;
          for (auto *MMO : cast<MemIntrinsicSDNode>(N)->memoperands())
            Refs.push_back(MMO);
          Refs.push_back(MF.getMachineMemOperand(
              MachinePointerInfo::getFixedStack(MF, FI),
              MachineMemOperand::MOLoad | MachineMemOperand::MOStore,
              PrivateBytes, Align(8)));
          CurDAG->setNodeMemRefs(V, Refs);
          if (!Batch)
            ReplaceUses(SDValue(N, 0), SDValue(V, 0));
          ReplaceUses(SDValue(N, Batch ? 0 : 1), SDValue(V, Batch ? 0 : 4));
        }
        CurDAG->RemoveDeadNode(N);
        return;
      }
    }
    if (N->getOpcode() == ISD::INTRINSIC_W_CHAIN &&
        N->getConstantOperandVal(1) == Intrinsic::sh_tas_b) {
      SDLoc DL(N);
      SDValue TASOperands[] = {N->getOperand(2), N->getOperand(0)};
      MachineSDNode *TAS = CurDAG->getMachineNode(
          SH::TAS_B, DL, CurDAG->getVTList(MVT::Other, MVT::Glue), TASOperands);
      CurDAG->setNodeMemRefs(TAS, cast<MemIntrinsicSDNode>(N)->memoperands());
      MachineSDNode *MOVT =
          CurDAG->getMachineNode(SH::MOVT, DL, MVT::i32, SDValue(TAS, 1));
      ReplaceUses(SDValue(N, 0), SDValue(MOVT, 0));
      ReplaceUses(SDValue(N, 1), SDValue(TAS, 0));
      CurDAG->RemoveDeadNode(N);
      return;
    }
    SDValue FrameIndex;
    int64_t Offset = 0;
    if (N->getOpcode() == ISD::FrameIndex)
      FrameIndex = SDValue(N, 0);
    else if (N->getOpcode() == ISD::ADD &&
             N->getOperand(0).getOpcode() == ISD::FrameIndex)
      if (const auto *Constant = dyn_cast<ConstantSDNode>(N->getOperand(1))) {
        FrameIndex = N->getOperand(0);
        Offset = Constant->getSExtValue();
      }
    if (FrameIndex) {
      SDValue Operands[] = {
          CurDAG->getTargetFrameIndex(
              cast<FrameIndexSDNode>(FrameIndex)->getIndex(), MVT::i32),
          CurDAG->getSignedTargetConstant(Offset, SDLoc(N), MVT::i32)};
      CurDAG->SelectNodeTo(N, SH::LEA_FI, MVT::i32, Operands);
      return;
    }
    SelectCode(N);
  }

#include "SHGenDAGISel.inc"
};

class SHDAGToDAGISelLegacy : public SelectionDAGISelLegacy {
public:
  static char ID;
  explicit SHDAGToDAGISelLegacy(SHTargetMachine &TM)
      : SelectionDAGISelLegacy(ID, std::make_unique<SHDAGToDAGISel>(TM)) {}
};

char SHDAGToDAGISelLegacy::ID = 0;

} // namespace

INITIALIZE_PASS(SHDAGToDAGISelLegacy, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createSHISelDagLegacyPass(SHTargetMachine &TM) {
  return new SHDAGToDAGISelLegacy(TM);
}

SHDAGToDAGISelPass::SHDAGToDAGISelPass(SHTargetMachine &TM)
    : SelectionDAGISelPass(std::make_unique<SHDAGToDAGISel>(TM)) {}
