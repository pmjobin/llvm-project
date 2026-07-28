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
