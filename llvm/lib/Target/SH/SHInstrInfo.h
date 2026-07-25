//===-- SHInstrInfo.h - SH instruction information -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHINSTRINFO_H
#define LLVM_LIB_TARGET_SH_SHINSTRINFO_H

#include "MCTargetDesc/SHMCTargetDesc.h"
#include "SHRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "SHGenInstrInfo.inc"

namespace llvm {

class SHSubtarget;

class SHInstrInfo : public SHGenInstrInfo {
  SHRegisterInfo RI;

public:
  explicit SHInstrInfo(const SHSubtarget &STI);

  const SHRegisterInfo &getRegisterInfo() const { return RI; }
  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                   const DebugLoc &DL, Register DestReg, Register SrcReg,
                   bool KillSrc, bool RenamableDest = false,
                   bool RenamableSrc = false) const override;
  Register isLoadFromStackSlot(const MachineInstr &MI,
                               int &FrameIndex) const override;
  Register isStoreToStackSlot(const MachineInstr &MI,
                              int &FrameIndex) const override;
  void storeRegToStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
      bool IsKill, int FrameIndex, const TargetRegisterClass *RC, Register VReg,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;
  void loadRegFromStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register DestReg,
      int FrameIndex, const TargetRegisterClass *RC, Register VReg,
      unsigned SubReg = 0,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;
};

} // namespace llvm

#endif
