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
  return CSR_SH_SaveList;
}

const uint32_t *SHRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                                     CallingConv::ID CC) const {
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
  return Reserved;
}

const TargetRegisterClass *
SHRegisterInfo::getPointerRegClass(unsigned Kind) const {
  return &SH::GPRRegClass;
}

bool SHRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MI,
                                         int SPAdj, unsigned FIOperandNum,
                                         RegScavenger *RS) const {
  if (SPAdj != 0)
    report_fatal_error("SH call-frame adjustments are not supported");

  MachineInstr &Instr = *MI;
  if (Instr.getOpcode() != SH::MOVL_load_disp &&
      Instr.getOpcode() != SH::MOVL_store_disp)
    report_fatal_error("SH frame index used by an unsupported instruction");
  if (FIOperandNum + 1 >= Instr.getNumOperands() ||
      !Instr.getOperand(FIOperandNum + 1).isImm())
    report_fatal_error("SH frame reference is missing its byte displacement");

  MachineFunction &MF = *Instr.getParent()->getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  int FrameIndex = Instr.getOperand(FIOperandNum).getIndex();
  int64_t ByteOffset = MFI.getObjectOffset(FrameIndex) +
                       static_cast<int64_t>(MFI.getStackSize()) +
                       Instr.getOperand(FIOperandNum + 1).getImm();

  if (ByteOffset < 0 || ByteOffset > 60 || ByteOffset % 4 != 0)
    report_fatal_error(
        "SH frame reference must be a four-byte aligned offset in [0, 60] "
        "from the post-prologue r15");

  Instr.getOperand(FIOperandNum).ChangeToRegister(SH::R15, false);
  if (ByteOffset == 0) {
    const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
    Instr.setDesc(TII->get(Instr.getOpcode() == SH::MOVL_load_disp
                               ? SH::MOVL_load_reg
                               : SH::MOVL_store_reg));
    Instr.removeOperand(FIOperandNum + 1);
  } else {
    Instr.getOperand(FIOperandNum + 1).setImm(ByteOffset);
  }
  return false;
}

Register SHRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return SH::R15;
}
