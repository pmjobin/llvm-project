//===-- SHInstrInfo.cpp - SH instruction information ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHInstrInfo.h"
#include "SHSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "SHGenInstrInfo.inc"

SHInstrInfo::SHInstrInfo(const SHSubtarget &STI)
    : SHGenInstrInfo(STI, RI, SH::ADJCALLSTACKDOWN, SH::ADJCALLSTACKUP), RI() {}

unsigned SHInstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  if (MI.isInlineAsm()) {
    const MachineFunction &MF = *MI.getParent()->getParent();
    return getInlineAsmLength(MI.getOperand(0).getSymbolName(),
                              MF.getTarget().getMCAsmInfo());
  }

  if (MI.getOpcode() == TargetOpcode::BUNDLE)
    return getInstBundleSize(MI);

  unsigned Size = MI.getDesc().getSize();
  if (MI.getDesc().hasDelaySlot() && !MI.isBundledWithSucc())
    Size += get(SH::NOP).getSize();
  return Size;
}

void SHInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MI,
                              const DebugLoc &DL, Register DestReg,
                              Register SrcReg, bool KillSrc, bool RenamableDest,
                              bool RenamableSrc) const {
  if (!SH::GPRRegClass.contains(DestReg, SrcReg))
    report_fatal_error("SH cannot copy the requested physical registers");
  BuildMI(MBB, MI, DL, get(SH::MOVrr), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
}

Register SHInstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
                                          int &FrameIndex) const {
  if (MI.getOpcode() == SH::MOVL_load_disp && MI.getOperand(1).isFI() &&
      MI.getOperand(2).isImm() && MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return MI.getOperand(0).getReg();
  }
  return Register();
}

Register SHInstrInfo::isStoreToStackSlot(const MachineInstr &MI,
                                         int &FrameIndex) const {
  if (MI.getOpcode() == SH::MOVL_store_disp && MI.getOperand(1).isFI() &&
      MI.getOperand(2).isImm() && MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return MI.getOperand(0).getReg();
  }
  return Register();
}

void SHInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
    bool IsKill, int FrameIndex, const TargetRegisterClass *RC, Register VReg,
    MachineInstr::MIFlag Flags) const {
  if (!SH::GPRRegClass.hasSubClassEq(RC))
    report_fatal_error("SH only supports 32-bit GPR spills");

  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.getObjectSize(FrameIndex) != 4 ||
      MFI.getObjectAlign(FrameIndex) < Align(4))
    report_fatal_error("SH GPR spill slots must be four-byte aligned words");

  MachineMemOperand *MMO =
      MF.getMachineMemOperand(MachinePointerInfo::getFixedStack(MF, FrameIndex),
                              MachineMemOperand::MOStore, 4, Align(4));
  BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(SH::MOVL_store_disp))
      .addReg(SrcReg, getKillRegState(IsKill))
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO)
      .setMIFlag(Flags);
}

void SHInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator MI,
                                       Register DestReg, int FrameIndex,
                                       const TargetRegisterClass *RC,
                                       Register VReg, unsigned SubReg,
                                       MachineInstr::MIFlag Flags) const {
  if (!SH::GPRRegClass.hasSubClassEq(RC) || SubReg != 0)
    report_fatal_error("SH only supports 32-bit GPR reloads");

  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.getObjectSize(FrameIndex) != 4 ||
      MFI.getObjectAlign(FrameIndex) < Align(4))
    report_fatal_error("SH GPR spill slots must be four-byte aligned words");

  MachineMemOperand *MMO =
      MF.getMachineMemOperand(MachinePointerInfo::getFixedStack(MF, FrameIndex),
                              MachineMemOperand::MOLoad, 4, Align(4));
  BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(SH::MOVL_load_disp), DestReg)
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO)
      .setMIFlag(Flags);
}

static std::optional<SHCC::CondCode> getSHBranchCondition(unsigned Opcode) {
  if (Opcode == SH::BT)
    return SHCC::TSet;
  if (Opcode == SH::BF)
    return SHCC::TClear;
  return std::nullopt;
}

bool SHInstrInfo::analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                                MachineBasicBlock *&FBB,
                                SmallVectorImpl<MachineOperand> &Cond,
                                bool AllowModify) const {
  MachineBasicBlock::iterator I = MBB.end();
  while (I != MBB.begin()) {
    --I;
    if (I->isDebugInstr())
      continue;
    if (!isUnpredicatedTerminator(*I))
      break;
    if (!I->isBranch())
      return true;

    if (I->getOpcode() == SH::BRA) {
      if (!AllowModify) {
        TBB = I->getOperand(0).getMBB();
        continue;
      }

      MBB.erase(std::next(I), MBB.end());
      Cond.clear();
      FBB = nullptr;
      if (MBB.isLayoutSuccessor(I->getOperand(0).getMBB())) {
        TBB = nullptr;
        I->eraseFromParent();
        I = MBB.end();
        continue;
      }
      TBB = I->getOperand(0).getMBB();
      continue;
    }

    std::optional<SHCC::CondCode> BranchCode =
        getSHBranchCondition(I->getOpcode());
    if (!BranchCode)
      return true;
    if (Cond.empty()) {
      FBB = TBB;
      TBB = I->getOperand(0).getMBB();
      Cond.push_back(MachineOperand::CreateImm(*BranchCode));
      continue;
    }
    if (Cond.size() != 1 || TBB != I->getOperand(0).getMBB() ||
        Cond[0].getImm() != *BranchCode)
      return true;
  }
  return false;
}

unsigned SHInstrInfo::insertBranch(MachineBasicBlock &MBB,
                                   MachineBasicBlock *TBB,
                                   MachineBasicBlock *FBB,
                                   ArrayRef<MachineOperand> Cond,
                                   const DebugLoc &DL, int *BytesAdded) const {
  assert(TBB && "SH branch must have a destination");
  assert(Cond.size() <= 1 && "invalid SH branch condition");
  if (BytesAdded)
    *BytesAdded = 0;

  if (Cond.empty()) {
    assert(!FBB && "unconditional SH branch has a false destination");
    BuildMI(&MBB, DL, get(SH::BRA)).addMBB(TBB);
    if (BytesAdded)
      *BytesAdded = 4;
    return 1;
  }

  assert(Cond[0].isImm() && "invalid SH branch condition operand");
  unsigned Opcode = Cond[0].getImm() == SHCC::TSet ? SH::BT : SH::BF;
  BuildMI(&MBB, DL, get(Opcode)).addMBB(TBB);
  unsigned Count = 1;
  if (BytesAdded)
    *BytesAdded = 2;
  if (FBB) {
    BuildMI(&MBB, DL, get(SH::BRA)).addMBB(FBB);
    ++Count;
    if (BytesAdded)
      *BytesAdded += 4;
  }
  return Count;
}

unsigned SHInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                   int *BytesRemoved) const {
  if (BytesRemoved)
    *BytesRemoved = 0;
  unsigned Count = 0;

  while (!MBB.empty()) {
    MachineBasicBlock::iterator I = MBB.end();
    do {
      --I;
    } while (I != MBB.begin() && I->isDebugInstr());

    if (I->getOpcode() == SH::NOP && I->isBundledWithPred()) {
      MachineBasicBlock::iterator Slot = I;
      --I;
      if (I->getOpcode() != SH::BRA || !I->isBundledWithSucc())
        break;
      if (BytesRemoved)
        *BytesRemoved += 4;
      MBB.erase(I, std::next(Slot));
      ++Count;
      continue;
    }

    if (I->getOpcode() != SH::BRA && I->getOpcode() != SH::BT &&
        I->getOpcode() != SH::BF)
      break;
    if (BytesRemoved)
      *BytesRemoved += I->getOpcode() == SH::BRA ? 4 : 2;
    I->eraseFromParent();
    ++Count;
  }
  return Count;
}

bool SHInstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  if (Cond.size() != 1 || !Cond[0].isImm())
    return true;
  if (Cond[0].getImm() == SHCC::TSet)
    Cond[0].setImm(SHCC::TClear);
  else if (Cond[0].getImm() == SHCC::TClear)
    Cond[0].setImm(SHCC::TSet);
  else
    return true;
  return false;
}

MachineBasicBlock *
SHInstrInfo::getBranchDestBlock(const MachineInstr &MI) const {
  assert((MI.getOpcode() == SH::BRA || MI.getOpcode() == SH::BT ||
          MI.getOpcode() == SH::BF) &&
         "not an SH direct branch");
  return MI.getOperand(0).getMBB();
}

bool SHInstrInfo::isBranchOffsetInRange(unsigned BranchOpc,
                                        int64_t BrOffset) const {
  int64_t ByteDisp = BrOffset - 4;
  if (ByteDisp % 2 != 0)
    return false;
  if (BranchOpc == SH::BT || BranchOpc == SH::BF)
    return isInt<8>(ByteDisp / 2);
  if (BranchOpc == SH::BRA)
    return isInt<12>(ByteDisp / 2);
  llvm_unreachable("not an SH direct branch");
}

void SHInstrInfo::insertIndirectBranch(MachineBasicBlock &MBB,
                                       MachineBasicBlock &NewDestBB,
                                       MachineBasicBlock &RestoreBB,
                                       const DebugLoc &DL, int64_t BrOffset,
                                       RegScavenger *RS) const {
  report_fatal_error(
      "SH branch target is out of range; long branches are not supported");
}
