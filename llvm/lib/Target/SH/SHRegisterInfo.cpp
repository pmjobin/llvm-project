//===-- SHRegisterInfo.cpp - SH register information ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHRegisterInfo.h"
#include "MCTargetDesc/SHMCTargetDesc.h"
#include "SHFrameLowering.h"
#include "SHInstrInfo.h"
#include "SHSubtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/CFIInstBuilder.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "SHGenRegisterInfo.inc"

SHRegisterInfo::SHRegisterInfo() : SHGenRegisterInfo(SH::PR) {}

const MCPhysReg *
SHRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_SH_Save_SaveList;
}

const uint32_t *SHRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                                     CallingConv::ID CC) const {
  if (CC != CallingConv::C)
    report_fatal_error("SH only supports the C calling convention");
  return CSR_SH_RegMask;
}

BitVector SHRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(SH::R15);
  Reserved.set(SH::PC);
  Reserved.set(SH::PR);
  Reserved.set(SH::GBR);
  Reserved.set(SH::VBR);
  Reserved.set(SH::MACH);
  Reserved.set(SH::MACL);
  Reserved.set(SH::SR);
  Reserved.set(SH::TBit);
  Reserved.set(SH::MBit);
  Reserved.set(SH::QBit);
  return Reserved;
}

const TargetRegisterClass *
SHRegisterInfo::getPointerRegClass(unsigned Kind) const {
  return &SH::GPRRegClass;
}

bool SHRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MI,
                                         int SPAdj, unsigned FIOperandNum,
                                         RegScavenger *RS) const {
  MachineInstr &Instr = *MI;
  unsigned Opcode = Instr.getOpcode();
  if (Opcode == SH::LEA_FI) {
    if (FIOperandNum != 1 || Instr.getNumOperands() != 3 ||
        !Instr.getOperand(0).isReg() ||
        !Instr.getOperand(FIOperandNum + 1).isImm())
      report_fatal_error("SH frame address pseudo is malformed");
    MachineFunction &MF = *Instr.getParent()->getParent();
    const MachineFrameInfo &MFI = MF.getFrameInfo();
    int FrameIndex = Instr.getOperand(FIOperandNum).getIndex();
    int64_t ByteOffset = MFI.getObjectOffset(FrameIndex) +
                         static_cast<int64_t>(MFI.getStackSize()) +
                         static_cast<int64_t>(SPAdj) +
                         Instr.getOperand(FIOperandNum + 1).getImm();
    if (ByteOffset < 0 || ByteOffset > 127)
      report_fatal_error(Twine("SH finalized frame address offset ") +
                         Twine(ByteOffset) + " must be in [0, 127] from r15");
    const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
    MachineBasicBlock &MBB = *Instr.getParent();
    Register Destination = Instr.getOperand(0).getReg();
    BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(SH::MOVrr), Destination)
        .addReg(SH::R15);
    if (ByteOffset != 0)
      BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(SH::ADDri), Destination)
          .addReg(Destination)
          .addImm(ByteOffset);
    MBB.erase(MI);
    return false;
  }
  bool IsLong = Opcode == SH::MOVL_load_disp || Opcode == SH::MOVL_store_disp;
  bool IsByte = Opcode == SH::MOVB_load_frame || Opcode == SH::MOVB_store_frame;
  bool IsWord = Opcode == SH::MOVW_load_frame || Opcode == SH::MOVW_store_frame;
  if (!IsLong && !IsByte && !IsWord)
    report_fatal_error("SH frame index used by an unsupported instruction");
  if (FIOperandNum + 1 >= Instr.getNumOperands() ||
      !Instr.getOperand(FIOperandNum + 1).isImm())
    report_fatal_error("SH frame reference is missing its byte displacement");

  MachineFunction &MF = *Instr.getParent()->getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  int FrameIndex = Instr.getOperand(FIOperandNum).getIndex();
  int64_t ByteOffset = MFI.getObjectOffset(FrameIndex) +
                       static_cast<int64_t>(MFI.getStackSize()) +
                       static_cast<int64_t>(SPAdj) +
                       Instr.getOperand(FIOperandNum + 1).getImm();

  bool IsLoad = Opcode == SH::MOVL_load_disp || Opcode == SH::MOVB_load_frame ||
                Opcode == SH::MOVW_load_frame;
  if (IsLong && ByteOffset % 4 != 0)
    report_fatal_error(Twine("SH finalized longword frame reference offset ") +
                       Twine(ByteOffset) + " must be four-byte aligned");
  if (IsWord && ByteOffset % 2 != 0)
    report_fatal_error(Twine("SH finalized word frame reference offset ") +
                       Twine(ByteOffset) +
                       " must be even and in [0, 30] from r15");
  bool NeedsAddressRegister =
      (IsLong && IsLoad && (ByteOffset < 0 || ByteOffset > 60)) ||
      (IsByte && (ByteOffset < 0 || ByteOffset > 15)) ||
      (IsWord && (ByteOffset < 0 || ByteOffset > 30));
  if (NeedsAddressRegister) {
    if (ByteOffset < 0 || ByteOffset > 127)
      report_fatal_error(Twine("SH finalized materialized frame offset ") +
                         Twine(ByteOffset) + " must be in [0, 127] from r15");
    const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
    MachineBasicBlock &MBB = *Instr.getParent();
    unsigned RegisterOpcode =
        IsLong   ? SH::MOVL_load_reg
        : IsByte ? (IsLoad ? SH::MOVB_load_reg : SH::MOVB_store_reg)
                 : (IsLoad ? SH::MOVW_load_reg : SH::MOVW_store_reg);
    Register Address = IsLoad ? Instr.getOperand(0).getReg() : SH::R0;
    BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(SH::MOVrr), Address)
        .addReg(SH::R15);
    if (ByteOffset != 0)
      BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(SH::ADDri), Address)
          .addReg(Address)
          .addImm(ByteOffset);
    if (IsLoad) {
      MachineInstrBuilder Load =
          BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(RegisterOpcode),
                  Instr.getOperand(0).getReg())
              .addReg(Address)
              .setMIFlags(Instr.getFlags())
              .cloneMemRefs(Instr);
      Load->getOperand(0).setIsDead(Instr.getOperand(0).isDead());
    } else {
      BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(RegisterOpcode))
          .add(Instr.getOperand(0))
          .addReg(Address, RegState::Kill)
          .setMIFlags(Instr.getFlags())
          .cloneMemRefs(Instr);
    }
    MBB.erase(MI);
    return false;
  }

  if (IsLong && !IsLoad && ByteOffset > 60) {
    if (ByteOffset > 120)
      report_fatal_error(Twine("SH finalized longword frame store offset ") +
                         Twine(ByteOffset) +
                         " must be four-byte aligned and in [0, 120] from r15");
    const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
    MachineBasicBlock &MBB = *Instr.getParent();
    int64_t Adjustment = ByteOffset - 60;
    BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(SH::ADDri), SH::R15)
        .addReg(SH::R15)
        .addImm(Adjustment);
    if (MF.needsFrameMoves())
      CFIInstBuilder(MBB, MI, MachineInstr::NoFlags)
          .buildAdjustCFAOffset(-Adjustment);
    BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(SH::MOVL_store_disp))
        .add(Instr.getOperand(0))
        .addReg(SH::R15)
        .addImm(60)
        .setMIFlags(Instr.getFlags())
        .cloneMemRefs(Instr);
    BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(SH::ADDri), SH::R15)
        .addReg(SH::R15)
        .addImm(-Adjustment);
    if (MF.needsFrameMoves())
      CFIInstBuilder(MBB, MI, MachineInstr::NoFlags)
          .buildAdjustCFAOffset(Adjustment);
    MBB.erase(MI);
    return false;
  }

  if (IsLong && (ByteOffset < 0 || ByteOffset > 60 || ByteOffset % 4 != 0))
    report_fatal_error(Twine("SH finalized frame reference offset ") +
                       Twine(ByteOffset) +
                       " must be four-byte aligned and in [0, 60] from r15");
  if (IsByte && (ByteOffset < 0 || ByteOffset > 15))
    report_fatal_error(Twine("SH finalized byte frame reference offset ") +
                       Twine(ByteOffset) + " must be in [0, 15] from r15");
  if (IsWord && (ByteOffset < 0 || ByteOffset > 30 || ByteOffset % 2 != 0))
    report_fatal_error(Twine("SH finalized word frame reference offset ") +
                       Twine(ByteOffset) +
                       " must be even and in [0, 30] from r15");

  Instr.getOperand(FIOperandNum).ChangeToRegister(SH::R15, false);
  if (ByteOffset == 0) {
    const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
    unsigned RegisterOpcode;
    switch (Opcode) {
    case SH::MOVL_load_disp:
      RegisterOpcode = SH::MOVL_load_reg;
      break;
    case SH::MOVL_store_disp:
      RegisterOpcode = SH::MOVL_store_reg;
      break;
    case SH::MOVB_load_frame:
      RegisterOpcode = SH::MOVB_load_reg;
      break;
    case SH::MOVB_store_frame:
      RegisterOpcode = SH::MOVB_store_reg;
      break;
    case SH::MOVW_load_frame:
      RegisterOpcode = SH::MOVW_load_reg;
      break;
    case SH::MOVW_store_frame:
      RegisterOpcode = SH::MOVW_store_reg;
      break;
    default:
      llvm_unreachable("unexpected SH frame instruction");
    }
    for (unsigned I = Instr.getNumOperands(); I != 0; --I) {
      MachineOperand &MO = Instr.getOperand(I - 1);
      if (MO.isReg() && MO.isImplicit() && MO.getReg() == SH::R0)
        Instr.removeOperand(I - 1);
    }
    Instr.setDesc(TII->get(RegisterOpcode));
    Instr.removeOperand(FIOperandNum + 1);
  } else {
    Instr.getOperand(FIOperandNum + 1).setImm(ByteOffset);
  }
  return false;
}

Register SHRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return SH::R15;
}
