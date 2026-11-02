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

namespace SHCC {
enum CondCode { TClear, TSet };
}

namespace SHII {
enum class FPURequirement : unsigned {
  None,
  PrimaryBank,
  Single,
  Double,
  DoubleArithmetic,
  ABIBoundary,
  ScalarMove,
  PairedSingle,
  SingleArithmetic,
};

enum : uint64_t {
  FPURequirementMask = 0x7,
  HasFPURequirement = 1u << 3,
  IsDelaySlotIllegal = 1u << 4,
  MayModifyFPSCRStatus = 1u << 5,
};

inline bool hasFPURequirement(const MCInstrDesc &Desc) {
  return Desc.TSFlags & HasFPURequirement;
}

inline FPURequirement getFPURequirement(const MCInstrDesc &Desc) {
  return static_cast<FPURequirement>((Desc.TSFlags & FPURequirementMask) |
                                     ((Desc.TSFlags >> 3) & 8));
}

inline bool isDelaySlotIllegal(const MCInstrDesc &Desc) {
  return Desc.TSFlags & IsDelaySlotIllegal;
}

// Arithmetic status effects do not change the required PR/SZ/FR mode.
inline bool mayModifyFPSCRStatus(const MCInstrDesc &Desc) {
  return Desc.TSFlags & MayModifyFPSCRStatus;
}
} // namespace SHII

class SHInstrInfo : public SHGenInstrInfo {
  SHRegisterInfo RI;

  std::optional<DestSourcePair>
  isCopyInstrImpl(const MachineInstr &MI) const override;

public:
  explicit SHInstrInfo(const SHSubtarget &STI);

  const TargetRegisterClass *getInlineAsmMemoryOperandRegClass(
      InlineAsm::ConstraintCode) const override {
    // Selection represents each admitted memory constraint by one GPR address.
    return &SH::GPRRegClass;
  }

  void materializeI32(MachineBasicBlock &MBB,
                      MachineBasicBlock::iterator Insert, const DebugLoc &DL,
                      Register DestReg, uint32_t Value,
                      MachineInstr::MIFlag Flag = MachineInstr::NoFlags) const;
  bool isLegalInDelaySlot(unsigned Opcode, const SHSubtarget &STI) const;
  bool isDelaySlotCandidate(const MachineInstr &MI,
                            const SHSubtarget &STI) const;
  const SHRegisterInfo &getRegisterInfo() const { return RI; }
  bool isDataSymbolAddress(const MachineInstr &MI) const;
  bool shouldHoist(const MachineInstr &MI,
                   const MachineLoop *FromLoop) const override;
  unsigned getTailMergeSize(const MachineFunction &MF) const override;
  unsigned getInstSizeInBytes(const MachineInstr &MI) const override;
  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                   const DebugLoc &DL, Register DestReg, Register SrcReg,
                   bool KillSrc, bool RenamableDest = false,
                   bool RenamableSrc = false) const override;
  bool expandPostRAPseudo(MachineInstr &MI) const override;
  bool expandGraphicsPseudo(MachineInstr &MI) const;
  Register isLoadFromStackSlot(const MachineInstr &MI,
                               int &FrameIndex) const override;
  Register isStoreToStackSlot(const MachineInstr &MI,
                              int &FrameIndex) const override;
  bool analyzeCompare(const MachineInstr &MI, Register &SrcReg,
                      Register &SrcReg2, int64_t &CmpMask,
                      int64_t &CmpValue) const override;
  bool optimizeCompareInstr(MachineInstr &CmpInstr, Register SrcReg,
                            Register SrcReg2, int64_t CmpMask, int64_t CmpValue,
                            const MachineRegisterInfo *MRI) const override;
  void storeRegToStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
      bool IsKill, int FrameIndex, const TargetRegisterClass *RC, Register VReg,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;
  void loadRegFromStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register DestReg,
      int FrameIndex, const TargetRegisterClass *RC, Register VReg,
      unsigned SubReg = 0,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;
  bool analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                     MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond,
                     bool AllowModify = false) const override;
  unsigned insertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                        MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond,
                        const DebugLoc &DL,
                        int *BytesAdded = nullptr) const override;
  unsigned removeBranch(MachineBasicBlock &MBB,
                        int *BytesRemoved = nullptr) const override;
  bool
  reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const override;
  MachineBasicBlock *getBranchDestBlock(const MachineInstr &MI) const override;
  bool isBranchOffsetInRange(unsigned BranchOpc,
                             int64_t BrOffset) const override;
  void insertIndirectBranch(MachineBasicBlock &MBB,
                            MachineBasicBlock &NewDestBB,
                            MachineBasicBlock &RestoreBB, const DebugLoc &DL,
                            int64_t BrOffset,
                            RegScavenger *RS = nullptr) const override;
};

} // namespace llvm

#endif
