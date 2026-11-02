//===-- SHPostRAPeephole.cpp - Fold post-RA SH sequences -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SH.h"
#include "SHISelLowering.h"
#include "SHInstrInfo.h"
#include "SHSubtarget.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachinePassManager.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetMachine.h"
#include <algorithm>

using namespace llvm;

#define DEBUG_TYPE "sh-postra-peephole"

STATISTIC(
    FoldedImmediateComparisons,
    "Number of SH immediate comparisons folded after register allocation");
STATISTIC(FoldedSignedUnaryComparisons,
          "Number of SH signed comparisons folded to unary comparisons after "
          "register allocation");
STATISTIC(FoldedImmediateLogic, "Number of SH immediate logical operations "
                                "folded after register allocation");
STATISTIC(FoldedMaskedZeroTests,
          "Number of SH masked zero tests folded after register allocation");
STATISTIC(
    FoldedRegisterMaskedZeroTests,
    "Number of SH register masked zero tests folded after register allocation");
STATISTIC(RemovedDeadMaskedZeroTestExtensions,
          "Number of dead SH extensions removed from folded masked zero tests");
STATISTIC(FoldedAdjacentAdds, "Number of SH adjacent immediate additions "
                              "folded after register allocation");
STATISTIC(FoldedPredecrementStores,
          "Number of SH predecrement stores folded after register allocation");
STATISTIC(
    FoldedNarrowDisplacementLoads,
    "Number of SH narrow displacement loads folded after register allocation");
STATISTIC(
    FoldedNarrowDisplacementLoadCopies,
    "Number of SH narrow displacement loads folded through R0 after register "
    "allocation");
STATISTIC(
    FoldedNarrowDisplacementStores,
    "Number of SH narrow displacement stores folded after register allocation");
STATISTIC(
    FoldedNarrowDisplacementStoreCopies,
    "Number of SH narrow displacement stores folded through R0 after register "
    "allocation");
STATISTIC(
    FoldedLongDisplacementAccesses,
    "Number of SH long displacement accesses folded after register allocation");
STATISTIC(
    FoldedImmediateIndexedLoads,
    "Number of SH immediate indexed loads folded after register allocation");
STATISTIC(
    FoldedImmediateIndexedStores,
    "Number of SH immediate indexed stores folded after register allocation");
STATISTIC(FoldedR0IndexedLoads,
          "Number of SH R0 indexed loads folded after register allocation");
STATISTIC(FoldedR0IndexedStores,
          "Number of SH R0 indexed stores folded after register allocation");
STATISTIC(
    FoldedR0IndexedBaseCopies,
    "Number of SH R0 indexed base copies folded after register allocation");
STATISTIC(RemovedExtendedR0CopyRoundTrips,
          "Number of SH extended R0 copy round trips removed");
STATISTIC(
    RematerializedImmediateCopyRegisters,
    "Number of SH immediate-only copy registers rematerialized after register "
    "allocation");

namespace {

static bool rematerializeImmediateCopyRegisters(MachineFunction &MF,
                                                const SHInstrInfo &TII) {
  if (!MF.getFunction().hasOptSize() && !MF.getFunction().hasMinSize())
    return false;
  const TargetRegisterInfo &TRI = TII.getRegisterInfo();
  SmallVector<MachineInstr *> Materializations;
  for (MachineBasicBlock &MBB : MF)
    for (MachineInstr &MI : MBB)
      if (MI.getOpcode() == SH::MOVri && MI.getNumOperands() == 2 &&
          MI.getOperand(0).isReg() && MI.getOperand(0).isDef() &&
          !MI.getOperand(0).isImplicit() && !MI.getOperand(0).getSubReg() &&
          MI.getOperand(1).isImm() && MI.getOperand(0).getReg().isPhysical() &&
          MI.getOperand(0).getReg() != SH::R15 &&
          SH::GPRRegClass.contains(MI.getOperand(0).getReg()))
        Materializations.push_back(&MI);

  bool Modified = false;
  for (MachineInstr *Materialize : Materializations) {
    // N copies of a materialized simm8 cost one instruction more than N
    // direct materializations. Restrict the rewrite to a local live range
    // whose end is proven by a kill, clobber, or redefinition.
    Register ConstantReg = Materialize->getOperand(0).getReg();
    SmallVector<MachineInstr *> Copies;
    bool Eligible = !Materialize->isBundledWithPred() &&
                    !Materialize->isBundledWithSucc() &&
                    Materialize->getFlags() == MachineInstr::NoFlags;
    bool RangeEnded = false;
    for (auto I = std::next(Materialize->getIterator()),
              E = Materialize->getParent()->instr_end();
         Eligible && I != E; ++I) {
      MachineInstr &MI = *I;
      if (MI.readsRegister(ConstantReg, &TRI)) {
        if ((MI.getOpcode() != TargetOpcode::COPY &&
             MI.getOpcode() != SH::MOVrr) ||
            MI.getNumOperands() != 2 || MI.isBundledWithPred() ||
            MI.isBundledWithSucc() || MI.getFlags() != MachineInstr::NoFlags ||
            !MI.getOperand(0).isReg() || !MI.getOperand(0).isDef() ||
            MI.getOperand(0).isImplicit() || MI.getOperand(0).getSubReg() ||
            !MI.getOperand(0).getReg().isPhysical() ||
            !SH::GPRRegClass.contains(MI.getOperand(0).getReg()) ||
            MI.getOperand(0).getReg() == SH::R15 || !MI.getOperand(1).isReg() ||
            MI.getOperand(1).getReg() != ConstantReg ||
            !MI.getOperand(1).isUse() || MI.getOperand(1).isUndef() ||
            MI.getOperand(1).isImplicit() || MI.getOperand(1).getSubReg()) {
          Eligible = false;
          break;
        }
        Copies.push_back(&MI);
        if (MI.getOperand(1).isKill()) {
          RangeEnded = true;
          break;
        }
      }
      if (MI.modifiesRegister(ConstantReg, &TRI)) {
        RangeEnded = true;
        break;
      }
    }
    if (!Eligible || !RangeEnded || Copies.empty())
      continue;
    int64_t Immediate = Materialize->getOperand(1).getImm();
    for (MachineInstr *Copy : Copies) {
      Copy->setDesc(TII.get(SH::MOVri));
      Copy->getOperand(1).ChangeToImmediate(Immediate);
    }
    Materialize->eraseFromParent();
    ++RematerializedImmediateCopyRegisters;
    Modified = true;
  }
  return Modified;
}

static bool foldImmediateComparison(MachineInstr &Compare,
                                    const SHInstrInfo &TII) {
  if (Compare.getOpcode() != SH::CMP_EQ ||
      Compare.getNumExplicitOperands() != 2)
    return false;
  MachineInstr *Materialize = Compare.getPrevNode();
  if (!Materialize || Materialize->getOpcode() != SH::MOVri ||
      Materialize->getNumExplicitOperands() != 2)
    return false;
  Register ConstantReg = Materialize->getOperand(0).getReg();
  if (ConstantReg == SH::R0)
    return false;
  MachineOperand *ConstantUse = nullptr;
  MachineOperand *ValueUse = nullptr;
  for (MachineOperand &MO : Compare.explicit_operands()) {
    if (!MO.isReg())
      return false;
    if (MO.getReg() == ConstantReg)
      ConstantUse = &MO;
    else if (MO.getReg() == SH::R0)
      ValueUse = &MO;
  }
  if (!ConstantUse || !ValueUse || !ConstantUse->isKill())
    return false;
  MachineInstrBuilder Folded =
      BuildMI(*Compare.getParent(), Compare, Compare.getDebugLoc(),
              TII.get(SH::CMP_EQri))
          .addImm(Materialize->getOperand(1).getImm())
          .setMIFlags(Compare.getFlags());
  for (const MachineOperand &MO : Compare.implicit_operands())
    if (MO.isReg() && MO.isDef() && MO.isDead())
      for (MachineOperand &FoldedMO : Folded->implicit_operands())
        if (FoldedMO.isReg() && FoldedMO.isDef() &&
            FoldedMO.getReg() == MO.getReg())
          FoldedMO.setIsDead();
  Compare.eraseFromParent();
  Materialize->eraseFromParent();
  ++FoldedImmediateComparisons;
  return true;
}

static unsigned getSignedUnaryComparisonOpcode(unsigned Opcode,
                                               int64_t Immediate) {
  if (Opcode == SH::CMP_GE && Immediate == 1)
    return SH::CMP_PL;
  if (Opcode == SH::CMP_GT && Immediate == -1)
    return SH::CMP_PZ;
  return 0;
}

static bool foldSignedUnaryComparison(MachineInstr &Compare,
                                      const SHInstrInfo &TII) {
  if ((Compare.getOpcode() != SH::CMP_GE &&
       Compare.getOpcode() != SH::CMP_GT) ||
      Compare.getNumExplicitOperands() != 2 || Compare.isBundledWithPred() ||
      Compare.isBundledWithSucc() ||
      Compare.getFlag(MachineInstr::FrameSetup) ||
      Compare.getFlag(MachineInstr::FrameDestroy) ||
      !Compare.getOperand(0).isReg() || !Compare.getOperand(1).isReg())
    return false;
  MachineInstr *Materialize = Compare.getPrevNode();
  if (!Materialize || Materialize->getOpcode() != SH::MOVri ||
      Materialize->getNumExplicitOperands() != 2 ||
      Materialize->isBundledWithPred() || Materialize->isBundledWithSucc() ||
      Materialize->getFlags() != MachineInstr::NoFlags ||
      !Materialize->getOperand(0).isReg() ||
      !Materialize->getOperand(1).isImm())
    return false;
  Register ConstantReg = Materialize->getOperand(0).getReg();
  MachineOperand &ConstantUse = Compare.getOperand(0);
  MachineOperand &ValueUse = Compare.getOperand(1);
  if (ConstantReg == SH::R15 || ConstantUse.getReg() != ConstantReg ||
      !ConstantUse.isKill() || ValueUse.getReg() == ConstantReg)
    return false;
  unsigned FoldedOpcode = getSignedUnaryComparisonOpcode(
      Compare.getOpcode(), Materialize->getOperand(1).getImm());
  if (!FoldedOpcode)
    return false;
  MachineInstrBuilder Folded =
      BuildMI(*Compare.getParent(), Compare, Compare.getDebugLoc(),
              TII.get(FoldedOpcode))
          .add(ValueUse)
          .setMIFlags(Compare.getFlags());
  for (const MachineOperand &MO : Compare.implicit_operands())
    if (MO.isReg() && MO.isDef() && MO.isDead())
      for (MachineOperand &FoldedMO : Folded->implicit_operands())
        if (FoldedMO.isReg() && FoldedMO.isDef() &&
            FoldedMO.getReg() == MO.getReg())
          FoldedMO.setIsDead();
  Compare.eraseFromParent();
  Materialize->eraseFromParent();
  ++FoldedSignedUnaryComparisons;
  return true;
}

static unsigned getImmediateLogicOpcode(unsigned Opcode) {
  switch (Opcode) {
  case SH::ANDrr:
    return SH::ANDri;
  case SH::ORrr:
    return SH::ORri;
  case SH::XORrr:
    return SH::XORri;
  default:
    return 0;
  }
}

static bool foldImmediateLogic(MachineInstr &Logic, const SHInstrInfo &TII) {
  unsigned ImmediateOpcode = getImmediateLogicOpcode(Logic.getOpcode());
  if (!ImmediateOpcode || Logic.getNumExplicitOperands() != 3 ||
      Logic.getOperand(0).getReg() != SH::R0 ||
      Logic.getOperand(1).getReg() != SH::R0)
    return false;
  MachineInstr *Materialize = Logic.getPrevNode();
  if (!Materialize || Materialize->getOpcode() != SH::MOVri ||
      Materialize->getNumExplicitOperands() != 2)
    return false;
  int64_t Immediate = Materialize->getOperand(1).getImm();
  if (!isUInt<8>(Immediate) || Materialize->getOperand(0).getReg() == SH::R0 ||
      Materialize->getOperand(0).getReg() != Logic.getOperand(2).getReg() ||
      !Logic.getOperand(2).isKill())
    return false;
  MachineInstrBuilder Folded =
      BuildMI(*Logic.getParent(), Logic, Logic.getDebugLoc(),
              TII.get(ImmediateOpcode))
          .addImm(Immediate)
          .setMIFlags(Logic.getFlags());
  for (MachineOperand &MO : Folded->implicit_operands()) {
    if (!MO.isReg() || MO.getReg() != SH::R0)
      continue;
    if (MO.isDef() && Logic.getOperand(0).isDead())
      MO.setIsDead();
    if (MO.isUse() && Logic.getOperand(1).isKill())
      MO.setIsKill();
  }
  Logic.eraseFromParent();
  Materialize->eraseFromParent();
  ++FoldedImmediateLogic;
  return true;
}

static bool foldMaskedZeroTest(MachineInstr &Test, const SHInstrInfo &TII) {
  if (Test.getOpcode() != SH::TST || Test.getNumExplicitOperands() != 2 ||
      !Test.getOperand(0).isReg() || !Test.getOperand(1).isReg() ||
      Test.getOperand(0).getReg() != Test.getOperand(1).getReg() ||
      (!Test.getOperand(0).isKill() && !Test.getOperand(1).isKill()))
    return false;
  Register ResultReg = Test.getOperand(0).getReg();
  MachineInstr *And = Test.getPrevNode();
  if (!And || And->getOpcode() != SH::ANDrr ||
      And->getNumExplicitOperands() != 3 ||
      And->getOperand(0).getReg() != ResultReg ||
      And->getOperand(1).getReg() != ResultReg)
    return false;
  MachineInstr *Materialize = And->getPrevNode();
  if (!Materialize || Materialize->getOpcode() != SH::MOVri ||
      Materialize->getNumExplicitOperands() != 2 ||
      Materialize->getOperand(0).getReg() != ResultReg)
    return false;
  int64_t Immediate = Materialize->getOperand(1).getImm();
  if (!isUInt<8>(Immediate))
    return false;
  Register ValueReg = And->getOperand(2).getReg();
  MachineInstr *DeadExtension = nullptr;
  if (ValueReg == SH::R0 && And->getOperand(2).isKill()) {
    MachineInstr *Extension = Materialize->getPrevNode();
    if (Extension && Extension->getOpcode() == SH::EXTUB &&
        Extension->getNumExplicitOperands() == 2 &&
        Extension->getOperand(0).isReg() &&
        Extension->getOperand(0).getReg() == SH::R0 &&
        Extension->getOperand(1).isReg() &&
        Extension->getOperand(1).getReg() == SH::R0 &&
        Extension->getOperand(1).isKill())
      DeadExtension = Extension;
  }
  if (ResultReg == SH::R0) {
    if (ValueReg == SH::R0)
      return false;
    BuildMI(*Test.getParent(), Test, Test.getDebugLoc(), TII.get(SH::MOVrr),
            SH::R0)
        .addReg(ValueReg, getKillRegState(And->getOperand(2).isKill()))
        .setMIFlags(And->getFlags());
  } else if (ValueReg != SH::R0) {
    return false;
  }
  bool TBitDead = false;
  for (const MachineOperand &MO : Test.implicit_operands())
    if (MO.isReg() && MO.isDef() && MO.getReg() == SH::TBit)
      TBitDead = MO.isDead();
  MachineInstrBuilder Folded =
      BuildMI(*Test.getParent(), Test, Test.getDebugLoc(), TII.get(SH::TSTri))
          .addImm(Immediate)
          .setMIFlags(Test.getFlags());
  for (MachineOperand &MO : Folded->implicit_operands()) {
    if (MO.isReg() && MO.isDef() && MO.getReg() == SH::TBit && TBitDead)
      MO.setIsDead();
    if (MO.isReg() && MO.isUse() && MO.getReg() == SH::R0 &&
        ValueReg == SH::R0 && And->getOperand(2).isKill())
      MO.setIsKill();
  }
  Test.eraseFromParent();
  And->eraseFromParent();
  Materialize->eraseFromParent();
  if (DeadExtension) {
    DeadExtension->eraseFromParent();
    ++RemovedDeadMaskedZeroTestExtensions;
  }
  ++FoldedMaskedZeroTests;
  return true;
}

static bool foldRegisterMaskedZeroTest(MachineInstr &Test,
                                       const SHInstrInfo &TII) {
  if (Test.getOpcode() != SH::TST || Test.getNumExplicitOperands() != 2 ||
      Test.isBundledWithPred() || Test.isBundledWithSucc() ||
      !Test.getOperand(0).isReg() || !Test.getOperand(1).isReg() ||
      Test.getOperand(0).getReg() != Test.getOperand(1).getReg() ||
      (!Test.getOperand(0).isKill() && !Test.getOperand(1).isKill()))
    return false;
  Register ResultReg = Test.getOperand(0).getReg();
  MachineInstr *And = Test.getPrevNode();
  if (!And || And->getOpcode() != SH::ANDrr ||
      And->getNumExplicitOperands() != 3 || And->isBundledWithPred() ||
      And->isBundledWithSucc() || !And->getOperand(0).isReg() ||
      !And->getOperand(1).isReg() || !And->getOperand(2).isReg() ||
      And->getOperand(0).getReg() != ResultReg ||
      And->getOperand(1).getReg() != ResultReg)
    return false;
  Register MaskReg = And->getOperand(2).getReg();
  RegState MaskState =
      getUndefRegState(And->getOperand(2).isUndef()) |
      getKillRegState(MaskReg != ResultReg && And->getOperand(2).isKill());
  RegState ResultState =
      RegState::Kill | getUndefRegState(And->getOperand(1).isUndef());
  bool TBitDead = false;
  for (const MachineOperand &MO : Test.implicit_operands())
    if (MO.isReg() && MO.isDef() && MO.getReg() == SH::TBit)
      TBitDead = MO.isDead();
  MachineInstrBuilder Folded =
      BuildMI(*Test.getParent(), Test, Test.getDebugLoc(), TII.get(SH::TST))
          .addReg(MaskReg, MaskState)
          .addReg(ResultReg, ResultState)
          .setMIFlags(Test.getFlags());
  for (MachineOperand &MO : Folded->implicit_operands())
    if (MO.isReg() && MO.isDef() && MO.getReg() == SH::TBit && TBitDead)
      MO.setIsDead();
  Test.eraseFromParent();
  And->eraseFromParent();
  ++FoldedRegisterMaskedZeroTests;
  return true;
}

static bool foldAdjacentRegisterAdds(MachineInstr &Add) {
  if (Add.getOpcode() != SH::ADDri || Add.getNumExplicitOperands() != 3 ||
      !Add.getOperand(0).isReg() || !Add.getOperand(1).isReg() ||
      !Add.getOperand(2).isImm() || Add.isBundledWithPred() ||
      Add.isBundledWithSucc() ||
      !SH::GPRRegClass.contains(Add.getOperand(0).getReg()) ||
      Add.getOperand(1).getReg() != Add.getOperand(0).getReg())
    return false;
  MachineInstr *Previous = Add.getPrevNode();
  if (!Previous || Previous->getOpcode() != SH::ADDri ||
      Previous->getNumExplicitOperands() != 3 ||
      Previous->isBundledWithPred() || Previous->isBundledWithSucc() ||
      !Previous->getOperand(0).isReg() || !Previous->getOperand(1).isReg() ||
      !Previous->getOperand(2).isImm() ||
      Previous->getOperand(0).getReg() != Add.getOperand(0).getReg() ||
      Previous->getOperand(0).getReg() != Previous->getOperand(1).getReg() ||
      Previous->getFlags() != Add.getFlags())
    return false;
  int64_t Immediate =
      Previous->getOperand(2).getImm() + Add.getOperand(2).getImm();
  if (!isInt<8>(Immediate))
    return false;
  if (Immediate == 0)
    Previous->eraseFromParent();
  else {
    Previous->getOperand(0).setIsDead(Add.getOperand(0).isDead());
    Previous->getOperand(2).setImm(Immediate);
  }
  Add.eraseFromParent();
  ++FoldedAdjacentAdds;
  return true;
}

static bool foldPredecrementStore(MachineInstr &Store, const SHInstrInfo &TII) {
  unsigned FoldedOpcode;
  int64_t Decrement;
  switch (Store.getOpcode()) {
  case SH::MOVB_store_reg:
    FoldedOpcode = SH::MOVB_store_predec;
    Decrement = -1;
    break;
  case SH::MOVW_store_reg:
    FoldedOpcode = SH::MOVW_store_predec;
    Decrement = -2;
    break;
  case SH::MOVL_store_reg:
    FoldedOpcode = SH::MOVL_store_predec;
    Decrement = -4;
    break;
  default:
    return false;
  }
  if (Store.getNumExplicitOperands() != 2 || Store.isBundledWithPred() ||
      Store.isBundledWithSucc() || Store.getFlag(MachineInstr::FrameSetup) ||
      Store.getFlag(MachineInstr::FrameDestroy) ||
      !Store.getOperand(0).isReg() || !Store.getOperand(1).isReg())
    return false;
  MachineInstr *Add = Store.getPrevNode();
  if (!Add || Add->getOpcode() != SH::ADDri ||
      Add->getNumExplicitOperands() != 3 || Add->isBundledWithPred() ||
      Add->isBundledWithSucc() || Add->getFlags() != MachineInstr::NoFlags ||
      !Add->getOperand(0).isReg() || !Add->getOperand(1).isReg() ||
      !Add->getOperand(2).isImm() || Add->getOperand(2).getImm() != Decrement)
    return false;
  Register Address = Add->getOperand(0).getReg();
  Register Source = Store.getOperand(0).getReg();
  if (Address == SH::R15 || Source == Address ||
      Add->getOperand(1).getReg() != Address ||
      Store.getOperand(1).getReg() != Address)
    return false;
  MachineInstrBuilder Folded =
      BuildMI(*Store.getParent(), Store, Store.getDebugLoc(),
              TII.get(FoldedOpcode), Address)
          .add(Store.getOperand(0))
          .addReg(Address)
          .setMIFlags(Store.getFlags())
          .cloneMemRefs(Store);
  if (Store.getOperand(1).isKill())
    Folded->getOperand(0).setIsDead();
  Store.eraseFromParent();
  Add->eraseFromParent();
  ++FoldedPredecrementStores;
  return true;
}

static bool foldNarrowDisplacementLoad(MachineInstr &Load,
                                       const SHInstrInfo &TII) {
  unsigned FoldedOpcode;
  switch (Load.getOpcode()) {
  case SH::MOVB_load_reg:
    FoldedOpcode = SH::MOVB_load_disp;
    break;
  case SH::MOVW_load_reg:
    FoldedOpcode = SH::MOVW_load_disp;
    break;
  default:
    return false;
  }
  if (Load.getNumExplicitOperands() != 2 || Load.isBundledWithPred() ||
      Load.isBundledWithSucc() || Load.getFlag(MachineInstr::FrameSetup) ||
      Load.getFlag(MachineInstr::FrameDestroy) || !Load.getOperand(0).isReg() ||
      !Load.getOperand(1).isReg() || Load.getOperand(0).getReg() == SH::R15 ||
      Load.getOperand(1).getReg() != SH::R0 ||
      (Load.getOperand(0).getReg() != SH::R0 && !Load.getOperand(1).isKill()))
    return false;
  MachineInstr *Add = Load.getPrevNode();
  if (!Add || Add->getOpcode() != SH::ADDri ||
      Add->getNumExplicitOperands() != 3 || Add->isBundledWithPred() ||
      Add->isBundledWithSucc() || Add->getFlag(MachineInstr::FrameSetup) ||
      Add->getFlag(MachineInstr::FrameDestroy) ||
      Add->getFlag(MachineInstr::NoMerge) || !Add->getOperand(0).isReg() ||
      !Add->getOperand(1).isReg() || !Add->getOperand(2).isImm() ||
      Add->getOperand(0).getReg() != SH::R0 ||
      Add->getOperand(1).getReg() != SH::R0)
    return false;
  int64_t Displacement = Add->getOperand(2).getImm();
  if ((Load.getOpcode() == SH::MOVB_load_reg &&
       (Displacement < 1 || Displacement > 15)) ||
      (Load.getOpcode() == SH::MOVW_load_reg &&
       (Displacement < 2 || Displacement > 30 || Displacement % 2 != 0)))
    return false;
  MachineInstr *Move = Add->getPrevNode();
  MachineOperand *Base = &Add->getOperand(1);
  if (!Move || (Move->getOpcode() != SH::MOVrr && !Move->isFullCopy()) ||
      Move->getNumExplicitOperands() != 2 || Move->isBundledWithPred() ||
      Move->isBundledWithSucc() || Move->getFlag(MachineInstr::FrameSetup) ||
      Move->getFlag(MachineInstr::FrameDestroy) ||
      Move->getFlag(MachineInstr::NoMerge) || !Move->getOperand(0).isReg() ||
      !Move->getOperand(1).isReg() || Move->getOperand(0).getReg() != SH::R0 ||
      Move->getOperand(1).getReg() == SH::R0 ||
      !SH::GPRRegClass.contains(Move->getOperand(1).getReg()))
    Move = nullptr;
  if (Move)
    Base = &Move->getOperand(1);
  else if (Load.getOperand(0).getReg() != SH::R0)
    return false;
  Register Destination = Load.getOperand(0).getReg();
  MachineInstrBuilder Folded =
      BuildMI(*Load.getParent(), Load, Load.getDebugLoc(),
              TII.get(FoldedOpcode), SH::R0)
          .add(*Base)
          .addImm(Displacement)
          .setMIFlags(Load.getFlags())
          .cloneMemRefs(Load);
  if (Destination == SH::R0 && Load.getOperand(0).isDead())
    Folded->getOperand(0).setIsDead();
  if (Destination != SH::R0) {
    MachineInstrBuilder Copy =
        BuildMI(*Load.getParent(), Load, Load.getDebugLoc(), TII.get(SH::MOVrr),
                Destination)
            .addReg(SH::R0, RegState::Kill);
    Copy->getOperand(0).setIsDead(Load.getOperand(0).isDead());
    Copy->getOperand(0).setIsRenamable(Load.getOperand(0).isRenamable());
    ++FoldedNarrowDisplacementLoadCopies;
  }
  Load.eraseFromParent();
  Add->eraseFromParent();
  if (Move)
    Move->eraseFromParent();
  ++FoldedNarrowDisplacementLoads;
  return true;
}

static bool foldNarrowDisplacementStore(MachineInstr &Store,
                                        const SHInstrInfo &TII) {
  unsigned FoldedOpcode;
  switch (Store.getOpcode()) {
  case SH::MOVB_store_reg:
    FoldedOpcode = SH::MOVB_store_disp;
    break;
  case SH::MOVW_store_reg:
    FoldedOpcode = SH::MOVW_store_disp;
    break;
  default:
    return false;
  }
  if (Store.getNumExplicitOperands() != 2 || Store.isBundledWithPred() ||
      Store.isBundledWithSucc() || Store.getFlag(MachineInstr::FrameSetup) ||
      Store.getFlag(MachineInstr::FrameDestroy) ||
      !Store.getOperand(0).isReg() || !Store.getOperand(1).isReg())
    return false;
  Register Source = Store.getOperand(0).getReg();
  Register Address = Store.getOperand(1).getReg();
  bool CopySourceToR0 = Source != SH::R0;
  if (Address == SH::R15 || !Store.getOperand(1).isKill() ||
      (CopySourceToR0 ? Address != SH::R0 : Address == SH::R0))
    return false;
  MachineInstr *Add = Store.getPrevNode();
  if (!Add || Add->getOpcode() != SH::ADDri ||
      Add->getNumExplicitOperands() != 3 || Add->isBundledWithPred() ||
      Add->isBundledWithSucc() || Add->getFlag(MachineInstr::FrameSetup) ||
      Add->getFlag(MachineInstr::FrameDestroy) ||
      Add->getFlag(MachineInstr::NoMerge) || !Add->getOperand(0).isReg() ||
      !Add->getOperand(1).isReg() || !Add->getOperand(2).isImm() ||
      Add->getOperand(0).getReg() != Address ||
      Add->getOperand(1).getReg() != Address)
    return false;
  int64_t Displacement = Add->getOperand(2).getImm();
  if ((Store.getOpcode() == SH::MOVB_store_reg &&
       (Displacement < 1 || Displacement > 15)) ||
      (Store.getOpcode() == SH::MOVW_store_reg &&
       (Displacement < 2 || Displacement > 30 || Displacement % 2 != 0)))
    return false;
  MachineInstr *Move = Add->getPrevNode();
  MachineOperand *Base = &Add->getOperand(1);
  if (Move && (Move->getOpcode() == SH::MOVrr || Move->isFullCopy()) &&
      Move->getNumExplicitOperands() == 2 && !Move->isBundledWithPred() &&
      !Move->isBundledWithSucc() && !Move->getFlag(MachineInstr::FrameSetup) &&
      !Move->getFlag(MachineInstr::FrameDestroy) &&
      !Move->getFlag(MachineInstr::NoMerge) && Move->getOperand(0).isReg() &&
      Move->getOperand(1).isReg() && Move->getOperand(0).getReg() == Address &&
      Move->getOperand(1).getReg() != Address &&
      SH::GPRRegClass.contains(Move->getOperand(1).getReg()))
    Base = &Move->getOperand(1);
  else
    Move = nullptr;
  if (CopySourceToR0 && !Move)
    return false;
  if (CopySourceToR0) {
    MachineInstrBuilder Copy =
        BuildMI(*Store.getParent(), Store, Store.getDebugLoc(),
                TII.get(SH::MOVrr), SH::R0)
            .addReg(Source);
    if (Store.getOperand(0).isKill() && Source != Base->getReg())
      Copy->getOperand(1).setIsKill();
    ++FoldedNarrowDisplacementStoreCopies;
  }
  MachineInstrBuilder Folded = BuildMI(
      *Store.getParent(), Store, Store.getDebugLoc(), TII.get(FoldedOpcode));
  if (CopySourceToR0)
    Folded.addReg(SH::R0, RegState::Kill);
  else
    Folded.add(Store.getOperand(0));
  Folded.add(*Base)
      .addImm(Displacement)
      .setMIFlags(Store.getFlags())
      .cloneMemRefs(Store);
  if (!Move)
    Folded->getOperand(1).setIsKill();
  if (CopySourceToR0 && Source == Base->getReg() &&
      Store.getOperand(0).isKill())
    Folded->getOperand(1).setIsKill();
  Store.eraseFromParent();
  Add->eraseFromParent();
  if (Move)
    Move->eraseFromParent();
  ++FoldedNarrowDisplacementStores;
  return true;
}

static bool foldLongDisplacementAccess(MachineInstr &Memory,
                                       const SHInstrInfo &TII) {
  bool IsLoad = Memory.getOpcode() == SH::MOVL_load_reg;
  if ((!IsLoad && Memory.getOpcode() != SH::MOVL_store_reg) ||
      Memory.getNumExplicitOperands() != 2 || Memory.isBundledWithPred() ||
      Memory.isBundledWithSucc() || Memory.getFlag(MachineInstr::FrameSetup) ||
      Memory.getFlag(MachineInstr::FrameDestroy) ||
      !Memory.getOperand(0).isReg() || !Memory.getOperand(1).isReg())
    return false;
  Register Address = Memory.getOperand(1).getReg();
  if (Address == SH::R15 ||
      (!Memory.getOperand(1).isKill() &&
       (!IsLoad || Memory.getOperand(0).getReg() != Address)) ||
      (!IsLoad && Memory.getOperand(0).getReg() == Address))
    return false;
  MachineInstr *Add = Memory.getPrevNode();
  if (!Add || Add->getOpcode() != SH::ADDri ||
      Add->getNumExplicitOperands() != 3 || Add->isBundledWithPred() ||
      Add->isBundledWithSucc() || Add->getFlag(MachineInstr::FrameSetup) ||
      Add->getFlag(MachineInstr::FrameDestroy) ||
      Add->getFlag(MachineInstr::NoMerge) || !Add->getOperand(0).isReg() ||
      !Add->getOperand(1).isReg() || !Add->getOperand(2).isImm() ||
      Add->getOperand(0).getReg() != Address ||
      Add->getOperand(1).getReg() != Address)
    return false;
  int64_t Displacement = Add->getOperand(2).getImm();
  if (Displacement < 4 || Displacement > 60 || Displacement % 4 != 0)
    return false;
  MachineInstr *Move = Add->getPrevNode();
  MachineOperand *Base = &Add->getOperand(1);
  if (Move && (Move->getOpcode() == SH::MOVrr || Move->isFullCopy()) &&
      Move->getNumExplicitOperands() == 2 && !Move->isBundledWithPred() &&
      !Move->isBundledWithSucc() && !Move->getFlag(MachineInstr::FrameSetup) &&
      !Move->getFlag(MachineInstr::FrameDestroy) &&
      !Move->getFlag(MachineInstr::NoMerge) && Move->getOperand(0).isReg() &&
      Move->getOperand(1).isReg() && Move->getOperand(0).getReg() == Address &&
      Move->getOperand(1).getReg() != Address &&
      SH::GPRRegClass.contains(Move->getOperand(1).getReg()))
    Base = &Move->getOperand(1);
  else
    Move = nullptr;
  unsigned FoldedOpcode = IsLoad ? SH::MOVL_load_disp : SH::MOVL_store_disp;
  MachineInstrBuilder Folded =
      BuildMI(*Memory.getParent(), Memory, Memory.getDebugLoc(),
              TII.get(FoldedOpcode))
          .add(Memory.getOperand(0))
          .add(*Base)
          .addImm(Displacement)
          .setMIFlags(Memory.getFlags())
          .cloneMemRefs(Memory);
  if (!Move && Memory.getOperand(1).isKill())
    Folded->getOperand(1).setIsKill();
  Memory.eraseFromParent();
  Add->eraseFromParent();
  if (Move)
    Move->eraseFromParent();
  ++FoldedLongDisplacementAccesses;
  return true;
}

static bool isJumpTableDispatch(const MachineInstr *MI);

static bool foldImmediateIndexedLoad(MachineInstr &Load,
                                     const SHInstrInfo &TII) {
  unsigned FoldedOpcode;
  switch (Load.getOpcode()) {
  case SH::MOVB_load_reg:
    FoldedOpcode = SH::MOVB_load_indexed;
    break;
  case SH::MOVW_load_reg:
    FoldedOpcode = SH::MOVW_load_indexed;
    break;
  case SH::MOVL_load_reg:
    FoldedOpcode = SH::MOVL_load_indexed;
    break;
  default:
    return false;
  }
  if (Load.getNumOperands() != 2 || Load.getNumExplicitOperands() != 2 ||
      Load.isBundledWithPred() || Load.isBundledWithSucc() ||
      Load.getFlag(MachineInstr::FrameSetup) ||
      Load.getFlag(MachineInstr::FrameDestroy) ||
      Load.getFlag(MachineInstr::NoMerge) || !Load.getOperand(0).isReg() ||
      !Load.getOperand(0).isDef() || !Load.getOperand(1).isReg() ||
      !Load.getOperand(1).isUse() || Load.getOperand(1).getReg() != SH::R0 ||
      (Load.getOperand(0).getReg() != SH::R0 && !Load.getOperand(1).isKill()) ||
      isJumpTableDispatch(Load.getNextNode()))
    return false;
  MachineInstr *Add = Load.getPrevNode();
  if (!Add || Add->getOpcode() != SH::ADDri || Add->getNumOperands() != 3 ||
      Add->getNumExplicitOperands() != 3 || Add->isBundledWithPred() ||
      Add->isBundledWithSucc() ||
      (Add->getFlags() & ~(MachineInstr::NoUWrap | MachineInstr::NoSWrap)) ||
      !Add->getOperand(0).isReg() || !Add->getOperand(0).isDef() ||
      !Add->getOperand(1).isReg() || !Add->getOperand(1).isUse() ||
      !Add->getOperand(2).isImm() || Add->getOperand(0).getReg() != SH::R0 ||
      Add->getOperand(1).getReg() != SH::R0)
    return false;
  int64_t Immediate = Add->getOperand(2).getImm();
  if (!isInt<8>(Immediate))
    return false;
  MachineInstr *Move = Add->getPrevNode();
  if (!Move || (Move->getOpcode() != SH::MOVrr && !Move->isFullCopy()) ||
      Move->getNumOperands() != 2 || Move->getNumExplicitOperands() != 2 ||
      Move->isBundledWithPred() || Move->isBundledWithSucc() ||
      Move->getFlags() != MachineInstr::NoFlags ||
      !Move->getOperand(0).isReg() || !Move->getOperand(0).isDef() ||
      !Move->getOperand(1).isReg() || !Move->getOperand(1).isUse() ||
      Move->getOperand(1).isUndef() || Move->getOperand(0).getReg() != SH::R0 ||
      Move->getOperand(1).getReg() == SH::R0 ||
      !Move->getOperand(1).getReg().isPhysical() ||
      !SH::GPRRegClass.contains(Move->getOperand(1).getReg()))
    return false;

  BuildMI(*Load.getParent(), Load, MIMetadata(Load), TII.get(SH::MOVri), SH::R0)
      .addImm(Immediate);
  MachineInstrBuilder Folded =
      BuildMI(*Load.getParent(), Load, MIMetadata(Load), TII.get(FoldedOpcode))
          .add(Load.getOperand(0))
          .add(Move->getOperand(1))
          .setMIFlags(Load.getFlags())
          .cloneMemRefs(Load);
  for (MachineOperand &MO : Folded->implicit_operands())
    if (MO.isReg() && MO.isUse() && MO.getReg() == SH::R0)
      MO.setIsKill();
  Load.eraseFromParent();
  Add->eraseFromParent();
  Move->eraseFromParent();
  ++FoldedImmediateIndexedLoads;
  return true;
}

static bool foldImmediateIndexedStore(MachineInstr &Store,
                                      const SHInstrInfo &TII) {
  unsigned FoldedOpcode;
  switch (Store.getOpcode()) {
  case SH::MOVB_store_reg:
    FoldedOpcode = SH::MOVB_store_indexed;
    break;
  case SH::MOVW_store_reg:
    FoldedOpcode = SH::MOVW_store_indexed;
    break;
  case SH::MOVL_store_reg:
    FoldedOpcode = SH::MOVL_store_indexed;
    break;
  default:
    return false;
  }
  if (Store.getNumOperands() != 2 || Store.getNumExplicitOperands() != 2 ||
      Store.isBundledWithPred() || Store.isBundledWithSucc() ||
      Store.getFlag(MachineInstr::FrameSetup) ||
      Store.getFlag(MachineInstr::FrameDestroy) ||
      Store.getFlag(MachineInstr::NoMerge) || !Store.getOperand(0).isReg() ||
      !Store.getOperand(0).isUse() || !Store.getOperand(1).isReg() ||
      !Store.getOperand(1).isUse() || Store.getOperand(0).getReg() == SH::R0 ||
      Store.getOperand(1).getReg() != SH::R0 || !Store.getOperand(1).isKill())
    return false;
  MachineInstr *Add = Store.getPrevNode();
  if (!Add || Add->getOpcode() != SH::ADDri || Add->getNumOperands() != 3 ||
      Add->getNumExplicitOperands() != 3 || Add->isBundledWithPred() ||
      Add->isBundledWithSucc() ||
      (Add->getFlags() & ~(MachineInstr::NoUWrap | MachineInstr::NoSWrap)) ||
      !Add->getOperand(0).isReg() || !Add->getOperand(0).isDef() ||
      !Add->getOperand(1).isReg() || !Add->getOperand(1).isUse() ||
      !Add->getOperand(2).isImm() || Add->getOperand(0).getReg() != SH::R0 ||
      Add->getOperand(1).getReg() != SH::R0)
    return false;
  int64_t Immediate = Add->getOperand(2).getImm();
  if (!isInt<8>(Immediate))
    return false;
  MachineInstr *Move = Add->getPrevNode();
  if (!Move || (Move->getOpcode() != SH::MOVrr && !Move->isFullCopy()) ||
      Move->getNumOperands() != 2 || Move->getNumExplicitOperands() != 2 ||
      Move->isBundledWithPred() || Move->isBundledWithSucc() ||
      Move->getFlags() != MachineInstr::NoFlags ||
      !Move->getOperand(0).isReg() || !Move->getOperand(0).isDef() ||
      !Move->getOperand(1).isReg() || !Move->getOperand(1).isUse() ||
      Move->getOperand(1).isUndef() || Move->getOperand(0).getReg() != SH::R0 ||
      Move->getOperand(1).getReg() == SH::R0 ||
      !Move->getOperand(1).getReg().isPhysical() ||
      !SH::GPRRegClass.contains(Move->getOperand(1).getReg()) ||
      (Store.getOperand(0).getReg() == Move->getOperand(1).getReg() &&
       Move->getOperand(1).isKill()))
    return false;

  BuildMI(*Store.getParent(), Store, MIMetadata(Store), TII.get(SH::MOVri),
          SH::R0)
      .addImm(Immediate);
  MachineInstrBuilder Folded = BuildMI(*Store.getParent(), Store,
                                       MIMetadata(Store), TII.get(FoldedOpcode))
                                   .add(Store.getOperand(0))
                                   .add(Move->getOperand(1))
                                   .setMIFlags(Store.getFlags())
                                   .cloneMemRefs(Store);
  if (Store.getOperand(0).getReg() == Move->getOperand(1).getReg() &&
      Store.getOperand(0).isKill()) {
    Folded->getOperand(0).setIsKill(false);
    Folded->getOperand(1).setIsKill();
  }
  for (MachineOperand &MO : Folded->implicit_operands())
    if (MO.isReg() && MO.isUse() && MO.getReg() == SH::R0)
      MO.setIsKill();
  Store.eraseFromParent();
  Add->eraseFromParent();
  Move->eraseFromParent();
  ++FoldedImmediateIndexedStores;
  return true;
}

static bool isJumpTableDispatch(const MachineInstr *MI) {
  if (!MI || MI->getOpcode() != SH::JMP)
    return false;
  for (const MachineOperand &MO : MI->operands())
    if (MO.isJTI())
      return true;
  return false;
}

static MachineInstr *getR0IndexedBaseCopy(MachineInstr &Add, Register Address) {
  if (Address == SH::R0)
    return nullptr;
  MachineInstr *Move = Add.getPrevNode();
  if (!Move || (Move->getOpcode() != SH::MOVrr && !Move->isFullCopy()) ||
      Move->getNumExplicitOperands() != 2 || Move->isBundledWithPred() ||
      Move->isBundledWithSucc() || Move->getFlag(MachineInstr::FrameSetup) ||
      Move->getFlag(MachineInstr::FrameDestroy) ||
      Move->getFlag(MachineInstr::NoMerge) || !Move->getOperand(0).isReg() ||
      !Move->getOperand(0).isDef() || !Move->getOperand(1).isReg() ||
      !Move->getOperand(1).isUse() || Move->getOperand(0).getReg() != Address ||
      Move->getOperand(1).getReg() == Address ||
      Move->getOperand(1).getReg() == SH::R0 ||
      !SH::GPRRegClass.contains(Move->getOperand(1).getReg()))
    return nullptr;
  return Move;
}

static bool foldR0IndexedLoad(MachineInstr &Load, const SHInstrInfo &TII) {
  unsigned FoldedOpcode;
  switch (Load.getOpcode()) {
  case SH::MOVB_load_reg:
    FoldedOpcode = SH::MOVB_load_indexed;
    break;
  case SH::MOVW_load_reg:
    FoldedOpcode = SH::MOVW_load_indexed;
    break;
  case SH::MOVL_load_reg:
    FoldedOpcode = SH::MOVL_load_indexed;
    break;
  default:
    return false;
  }
  if (Load.getNumOperands() != 2 || Load.getNumExplicitOperands() != 2 ||
      Load.isBundledWithPred() || Load.isBundledWithSucc() ||
      Load.getFlag(MachineInstr::FrameSetup) ||
      Load.getFlag(MachineInstr::FrameDestroy) ||
      Load.getFlag(MachineInstr::NoMerge) || !Load.getOperand(0).isReg() ||
      !Load.getOperand(0).isDef() || !Load.getOperand(1).isReg() ||
      !Load.getOperand(1).isUse() ||
      (Load.getOperand(0).getReg() != Load.getOperand(1).getReg() &&
       !Load.getOperand(1).isKill()) ||
      isJumpTableDispatch(Load.getNextNode()))
    return false;
  Register Address = Load.getOperand(1).getReg();
  MachineInstr *Add = Load.getPrevNode();
  if (!Add || Add->getOpcode() != SH::ADDrr || Add->getNumOperands() != 3 ||
      Add->getNumExplicitOperands() != 3 || Add->isBundledWithPred() ||
      Add->isBundledWithSucc() ||
      (Add->getFlags() & ~(MachineInstr::NoUWrap | MachineInstr::NoSWrap)) ||
      !Add->getOperand(0).isReg() || !Add->getOperand(0).isDef() ||
      !Add->getOperand(1).isReg() || !Add->getOperand(1).isUse() ||
      !Add->getOperand(2).isReg() || !Add->getOperand(2).isUse() ||
      Add->getOperand(0).getReg() != Address ||
      Add->getOperand(1).getReg() != Address)
    return false;
  MachineOperand *BaseOperand = &Add->getOperand(2);
  MachineOperand *R0Operand = &Add->getOperand(1);
  if (Address != SH::R0) {
    if (Address == SH::R15 || Add->getOperand(2).getReg() != SH::R0)
      return false;
    BaseOperand = &Add->getOperand(1);
    R0Operand = &Add->getOperand(2);
  }
  MachineInstr *Move = getR0IndexedBaseCopy(*Add, Address);
  if (Move)
    BaseOperand = &Move->getOperand(1);
  Register Base = BaseOperand->getReg();
  if (!Base.isPhysical() || Base == SH::R0 || !SH::GPRRegClass.contains(Base))
    return false;

  // No-wrap flags describe poison on the removed address calculation; the
  // indexed instruction computes the same effective address when it executes.
  MachineInstrBuilder Folded =
      BuildMI(*Load.getParent(), Load, MIMetadata(Load), TII.get(FoldedOpcode))
          .add(Load.getOperand(0))
          .add(*BaseOperand)
          .setMIFlags(Load.getFlags())
          .cloneMemRefs(Load);
  for (MachineOperand &MO : Folded->implicit_operands())
    if (MO.isReg() && MO.isUse() && MO.getReg() == SH::R0 &&
        (Load.getOperand(0).getReg() == SH::R0 || R0Operand->isKill()))
      MO.setIsKill();
  Load.eraseFromParent();
  Add->eraseFromParent();
  if (Move) {
    Move->eraseFromParent();
    ++FoldedR0IndexedBaseCopies;
  }
  ++FoldedR0IndexedLoads;
  return true;
}

static bool foldR0IndexedStore(MachineInstr &Store, const SHInstrInfo &TII) {
  unsigned FoldedOpcode;
  switch (Store.getOpcode()) {
  case SH::MOVB_store_reg:
    FoldedOpcode = SH::MOVB_store_indexed;
    break;
  case SH::MOVW_store_reg:
    FoldedOpcode = SH::MOVW_store_indexed;
    break;
  case SH::MOVL_store_reg:
    FoldedOpcode = SH::MOVL_store_indexed;
    break;
  default:
    return false;
  }
  if (Store.getNumOperands() != 2 || Store.getNumExplicitOperands() != 2 ||
      Store.isBundledWithPred() || Store.isBundledWithSucc() ||
      Store.getFlag(MachineInstr::FrameSetup) ||
      Store.getFlag(MachineInstr::FrameDestroy) ||
      Store.getFlag(MachineInstr::NoMerge) || !Store.getOperand(0).isReg() ||
      !Store.getOperand(0).isUse() || !Store.getOperand(1).isReg() ||
      !Store.getOperand(1).isUse() || !Store.getOperand(1).isKill())
    return false;
  Register Address = Store.getOperand(1).getReg();
  if (Store.getOperand(0).getReg() == SH::R0 ||
      Store.getOperand(0).getReg() == Address)
    return false;
  MachineInstr *Add = Store.getPrevNode();
  if (!Add || Add->getOpcode() != SH::ADDrr || Add->getNumOperands() != 3 ||
      Add->getNumExplicitOperands() != 3 || Add->isBundledWithPred() ||
      Add->isBundledWithSucc() ||
      (Add->getFlags() & ~(MachineInstr::NoUWrap | MachineInstr::NoSWrap)) ||
      !Add->getOperand(0).isReg() || !Add->getOperand(0).isDef() ||
      !Add->getOperand(1).isReg() || !Add->getOperand(1).isUse() ||
      !Add->getOperand(2).isReg() || !Add->getOperand(2).isUse() ||
      Add->getOperand(0).getReg() != Address ||
      Add->getOperand(1).getReg() != Address)
    return false;
  MachineOperand *BaseOperand = &Add->getOperand(2);
  MachineOperand *R0Operand = &Add->getOperand(1);
  if (Address != SH::R0) {
    if (Address == SH::R15 || Add->getOperand(2).getReg() != SH::R0)
      return false;
    BaseOperand = &Add->getOperand(1);
    R0Operand = &Add->getOperand(2);
  }
  MachineInstr *Move = getR0IndexedBaseCopy(*Add, Address);
  if (Move)
    BaseOperand = &Move->getOperand(1);
  Register Base = BaseOperand->getReg();
  if (!Base.isPhysical() || Base == SH::R0 || !SH::GPRRegClass.contains(Base))
    return false;

  MachineInstrBuilder Folded = BuildMI(*Store.getParent(), Store,
                                       MIMetadata(Store), TII.get(FoldedOpcode))
                                   .add(Store.getOperand(0))
                                   .add(*BaseOperand)
                                   .setMIFlags(Store.getFlags())
                                   .cloneMemRefs(Store);
  for (MachineOperand &MO : Folded->implicit_operands())
    if (MO.isReg() && MO.isUse() && MO.getReg() == SH::R0 &&
        R0Operand->isKill())
      MO.setIsKill();
  Store.eraseFromParent();
  Add->eraseFromParent();
  if (Move) {
    Move->eraseFromParent();
    ++FoldedR0IndexedBaseCopies;
  }
  ++FoldedR0IndexedStores;
  return true;
}

static bool isExtendedR0RoundTripOpcode(unsigned Opcode) {
  switch (Opcode) {
  case SH::EXTUB:
  case SH::EXTUW:
  case SH::EXTSB:
  case SH::EXTSW:
  case SH::ADDrr:
  case SH::ADDri:
  case SH::SHLR:
  case SH::SHLR2:
    return true;
  default:
    return false;
  }
}

static bool removeExtendedR0CopyRoundTrip(MachineInstr &MoveBack) {
  if (MoveBack.getOpcode() != SH::MOVrr ||
      MoveBack.getNumExplicitOperands() != 2 || MoveBack.isBundledWithPred() ||
      MoveBack.isBundledWithSucc() ||
      MoveBack.getFlags() != MachineInstr::NoFlags ||
      !MoveBack.getOperand(0).isReg() || !MoveBack.getOperand(0).isDef() ||
      MoveBack.getOperand(0).getReg() != SH::R0 ||
      !MoveBack.getOperand(1).isReg() || !MoveBack.getOperand(1).isUse() ||
      !MoveBack.getOperand(1).isKill() || MoveBack.getOperand(1).isUndef() ||
      MoveBack.getOperand(0).getSubReg() || MoveBack.getOperand(1).getSubReg())
    return false;
  Register Temporary = MoveBack.getOperand(1).getReg();
  if (Temporary == SH::R0 || Temporary == SH::R15 || !Temporary.isPhysical() ||
      !SH::GPRRegClass.contains(Temporary))
    return false;

  SmallVector<MachineInstr *> Chain;
  MachineInstr *Current = MoveBack.getPrevNode();
  bool FoundStart = false;
  while (Current && Chain.size() != 8) {
    if (!isExtendedR0RoundTripOpcode(Current->getOpcode()) ||
        Current->isBundledWithPred() || Current->isBundledWithSucc() ||
        Current->getFlags() != MachineInstr::NoFlags ||
        Current->getNumExplicitOperands() < 2 ||
        !Current->getOperand(0).isReg() || !Current->getOperand(0).isDef() ||
        Current->getOperand(0).getReg() != Temporary ||
        Current->getOperand(0).getSubReg())
      return false;
    bool ReadsTemporary = false;
    bool ReadsR0 = false;
    for (const MachineOperand &MO : Current->operands()) {
      if (!MO.isReg() || !MO.getReg().isPhysical())
        continue;
      if (MO.getReg() == Temporary && MO.isUse()) {
        if (MO.isImplicit() || MO.isUndef() || MO.getSubReg() || !MO.isKill())
          return false;
        ReadsTemporary = true;
      }
      if (MO.getReg() != SH::R0)
        continue;
      if (MO.isDef() || MO.isImplicit() || MO.isUndef() || MO.getSubReg() ||
          !MO.isKill())
        return false;
      ReadsR0 = true;
    }
    Chain.push_back(Current);
    if (ReadsR0) {
      if (ReadsTemporary || (Current->getOpcode() != SH::EXTUB &&
                             Current->getOpcode() != SH::EXTUW &&
                             Current->getOpcode() != SH::EXTSB &&
                             Current->getOpcode() != SH::EXTSW))
        return false;
      FoundStart = true;
      break;
    }
    if (!ReadsTemporary)
      return false;
    Current = Current->getPrevNode();
  }
  if (!FoundStart)
    return false;
  for (MachineInstr *MI : Chain)
    for (MachineOperand &MO : MI->operands())
      if (MO.isReg() && MO.getReg() == Temporary)
        MO.setReg(SH::R0);
  MoveBack.eraseFromParent();
  ++RemovedExtendedR0CopyRoundTrips;
  return true;
}

static bool runPostRAPeephole(MachineFunction &MF) {
  if (MF.getTarget().getOptLevel() == CodeGenOptLevel::None ||
      MF.getFunction().hasOptNone())
    return false;
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  bool Modified = rematerializeImmediateCopyRegisters(MF, TII);
  for (MachineBasicBlock &MBB : MF)
    for (auto I = MBB.begin(); I != MBB.end();) {
      MachineInstr &MI = *I++;
      bool Changed =
          foldImmediateComparison(MI, TII) ||
          foldSignedUnaryComparison(MI, TII) || foldImmediateLogic(MI, TII) ||
          foldMaskedZeroTest(MI, TII) || foldRegisterMaskedZeroTest(MI, TII) ||
          foldAdjacentRegisterAdds(MI) || foldPredecrementStore(MI, TII) ||
          foldNarrowDisplacementLoad(MI, TII) ||
          foldNarrowDisplacementStore(MI, TII) ||
          foldLongDisplacementAccess(MI, TII) ||
          foldImmediateIndexedLoad(MI, TII) ||
          foldImmediateIndexedStore(MI, TII) || foldR0IndexedLoad(MI, TII) ||
          foldR0IndexedStore(MI, TII) || removeExtendedR0CopyRoundTrip(MI);
      Modified |= Changed;
    }
  return Modified;
}

class SHPostRAPeepholeLegacy : public MachineFunctionPass {
public:
  static char ID;
  SHPostRAPeepholeLegacy() : MachineFunctionPass(ID) {}
  StringRef getPassName() const override { return "SH Post-RA Peephole"; }
  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().setNoVRegs();
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }
  bool runOnMachineFunction(MachineFunction &MF) override {
    return runPostRAPeephole(MF);
  }
};

char SHPostRAPeepholeLegacy::ID = 0;

} // namespace

INITIALIZE_PASS(SHPostRAPeepholeLegacy, DEBUG_TYPE,
                "Fold SH post-register-allocation sequences", false, false)

FunctionPass *llvm::createSHPostRAPeepholeLegacyPass() {
  return new SHPostRAPeepholeLegacy();
}

PreservedAnalyses
SHPostRAPeepholePass::run(MachineFunction &MF,
                          MachineFunctionAnalysisManager &MFAM) {
  return runPostRAPeephole(MF) ? getMachineFunctionPassPreservedAnalyses()
                                     .preserveSet<CFGAnalyses>()
                               : PreservedAnalyses::all();
}
