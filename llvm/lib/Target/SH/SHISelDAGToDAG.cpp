//===-- SHISelDAGToDAG.cpp - SH DAG instruction selection ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SH.h"
#include "SHTargetMachine.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
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
