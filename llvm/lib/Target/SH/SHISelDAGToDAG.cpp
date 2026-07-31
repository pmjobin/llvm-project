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
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/IR/IntrinsicsSH.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

#define DEBUG_TYPE "sh-isel"
#define PASS_NAME "SH DAG->DAG Pattern Instruction Selection"

namespace {

class SHDAGToDAGISel : public SelectionDAGISel {
  bool isSupportedBase(SDValue Base) const {
    switch (Base.getOpcode()) {
    case ISD::FrameIndex:
    case ISD::CopyFromReg:
    case ISD::LOAD:
    case ISD::Register:
    case ISD::ADD:
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
    Disp = CurDAG->getTargetConstant(ByteDisp, SDLoc(Addr), MVT::i32);
    return true;
  }

public:
  explicit SHDAGToDAGISel(SHTargetMachine &TM) : SelectionDAGISel(TM) {}

  bool runOnMachineFunction(MachineFunction &MF) override {
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
    if (Addr.getOpcode() == ISD::ADD) {
      const auto *Offset = dyn_cast<ConstantSDNode>(Addr.getOperand(1));
      if (Offset && Offset->isZero() && isSupportedBase(Addr.getOperand(0))) {
        Base = Addr.getOperand(0);
        return true;
      }
      if (Offset && isSupportedBase(Addr.getOperand(0))) {
        int64_t ByteDisp = Offset->getSExtValue();
        if (ByteDisp < 0 || ByteDisp > 60 || ByteDisp % 4 != 0) {
          Base = Addr;
          return true;
        }
      }
    }
    if (Addr.getOpcode() == ISD::FrameIndex || !isSupportedBase(Addr))
      return false;
    Base = Addr;
    return true;
  }

  bool SelectAddrDisp(SDValue Addr, SDValue &Base, SDValue &Disp) {
    int64_t ByteDisp = 0;
    SDValue CandidateBase = Addr;

    if (Addr.getOpcode() == ISD::ADD) {
      const auto *Offset = dyn_cast<ConstantSDNode>(Addr.getOperand(1));
      if (!Offset)
        return false;
      ByteDisp = Offset->getSExtValue();
      CandidateBase = Addr.getOperand(0);
    }

    bool IsFrameIndex = CandidateBase.getOpcode() == ISD::FrameIndex;
    if (!isSupportedBase(CandidateBase) || (!IsFrameIndex && ByteDisp == 0) ||
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

  bool SelectNarrowAddrReg(SDValue Addr, SDValue &Base) {
    if (Addr.getValueType() != MVT::i32 || Addr.getOpcode() == ISD::FrameIndex)
      return false;
    if (Addr.getOpcode() == ISD::ADD &&
        Addr.getOperand(0).getOpcode() == ISD::FrameIndex)
      return false;
    switch (Addr.getOpcode()) {
    case ISD::CopyFromReg:
    case ISD::LOAD:
    case ISD::Register:
    case ISD::ADD:
      Base = Addr;
      return true;
    default:
      return false;
    }
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
    if (N->getOpcode() == ISD::BRCOND &&
        N->getOperand(1).getOpcode() == SHISD::SETCC64) {
      SDValue Condition = N->getOperand(1);
      SDValue Operands[] = {Condition.getOperand(0), Condition.getOperand(1),
                            Condition.getOperand(2), Condition.getOperand(3),
                            Condition.getOperand(4), N->getOperand(2),
                            N->getOperand(0)};
      CurDAG->SelectNodeTo(N, SH::BR_CC64_PSEUDO, MVT::Other, Operands);
      return;
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
          CurDAG->getTargetConstant(Offset, SDLoc(N), MVT::i32)};
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
