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
public:
  explicit SHDAGToDAGISel(SHTargetMachine &TM) : SelectionDAGISel(TM) {}

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
