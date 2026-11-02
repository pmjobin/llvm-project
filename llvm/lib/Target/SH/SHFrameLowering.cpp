//===-- SHFrameLowering.cpp - SH frame lowering ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHFrameLowering.h"
#include "SH.h"
#include "SHConstantPoolValue.h"
#include "SHInstrInfo.h"
#include "SHLiteralPool.h"
#include "SHMachineFunctionInfo.h"
#include "SHSubtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/CFIInstBuilder.h"
#include "llvm/CodeGen/LiveRegUnits.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

SHFrameLowering::SHFrameLowering()
    : TargetFrameLowering(StackGrowsDown, Align(4), 0, Align(4), true) {}

bool SHFrameLowering::hasReservedCallFrame(const MachineFunction &MF) const {
  // Keep outgoing arguments at a stable SP offset when optimizing fixed
  // frames for size. PEI includes the maximum call area in the local frame.
  return MF.getFunction().hasOptSize() && !hasFP(MF) &&
         MF.getInfo<SHMachineFunctionInfo>()->useReservedCallFrame() &&
         !MF.getInfo<SHMachineFunctionInfo>()->isHandler() &&
         !MF.getFunction().hasFnAttribute(Attribute::Naked) &&
         !MF.getFrameInfo().hasOpaqueSPAdjustment() && !MF.callsEHReturn();
}

bool SHFrameLowering::canSimplifyCallFramePseudos(
    const MachineFunction &MF) const {
  // Keep the size decision open until allocation and scavenger slots are
  // known. Nonreserved SP frames still need PEI's adjustment tracking.
  return hasFP(MF);
}

bool SHFrameLowering::needsFrameIndexResolution(
    const MachineFunction &MF) const {
  return MF.getFrameInfo().hasStackObjects() ||
         (MF.getInfo<SHMachineFunctionInfo>()->isHandler() &&
          MF.getFrameInfo().adjustsStack());
}

static bool isSHDisplacementFrameAccess(const MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case SH::MOVL_load_disp:
  case SH::MOVL_store_disp:
  case SH::MOVB_load_frame:
  case SH::MOVB_store_frame:
  case SH::MOVW_load_frame:
  case SH::MOVW_store_frame:
    return true;
  default:
    return false;
  }
}

void SHFrameLowering::orderFrameObjects(
    const MachineFunction &MF, SmallVectorImpl<int> &ObjectsToAllocate) const {
  if (ObjectsToAllocate.size() < 2)
    return;

  const MachineFrameInfo &MFI = MF.getFrameInfo();
  SmallVector<uint64_t, 32> UseCounts(MFI.getObjectIndexEnd(), 0);
  BitVector IsAllocating(MFI.getObjectIndexEnd());
  for (int FrameIndex : ObjectsToAllocate)
    IsAllocating.set(FrameIndex);
  for (const MachineBasicBlock &MBB : MF)
    for (const MachineInstr &MI : MBB) {
      if (!isSHDisplacementFrameAccess(MI))
        continue;
      for (const MachineOperand &MO : MI.operands())
        if (MO.isFI() && MO.getIndex() >= 0 && IsAllocating.test(MO.getIndex()))
          ++UseCounts[MO.getIndex()];
    }

  // Objects at the end are allocated closest to the final stack pointer,
  // where SH's short displacement forms can address them directly.
  llvm::stable_sort(ObjectsToAllocate, [&](int LHS, int RHS) {
    uint64_t LHSSize =
        static_cast<uint64_t>(std::max<int64_t>(MFI.getObjectSize(LHS), 1));
    uint64_t RHSSize =
        static_cast<uint64_t>(std::max<int64_t>(MFI.getObjectSize(RHS), 1));
    uint64_t LHSDensity = UseCounts[LHS] * RHSSize;
    uint64_t RHSDensity = UseCounts[RHS] * LHSSize;
    if (LHSDensity != RHSDensity)
      return LHSDensity < RHSDensity;
    return MFI.getObjectAlign(LHS).value() < MFI.getObjectAlign(RHS).value();
  });
}

static uint64_t requireSupportedSHFrame(const MachineFunction &MF) {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.getMaxAlign().value() > SHMaxStackAlignment)
    report_fatal_error(Twine("SH stack object alignment cannot exceed ") +
                       Twine(SHMaxStackAlignment) + " bytes");

  uint64_t StackSize = MFI.getStackSize();
  if (StackSize > SHMaxStackFrameSize)
    report_fatal_error(Twine("SH stack frame size cannot exceed ") +
                       Twine(SHMaxStackFrameSize) + " bytes");
  if (StackSize % 4 != 0)
    report_fatal_error("SH stack frame size must be four-byte aligned");
  return StackSize;
}

static int64_t getSHAdjustmentChunk(int64_t Amount) {
  if (Amount < -128)
    return -128;
  if (Amount > 127)
    return 127;
  return Amount;
}

static MachineBasicBlock::iterator
emitSHSPAdjustment(MachineFunction &MF, MachineBasicBlock &MBB,
                   MachineBasicBlock::iterator Insert, int64_t Amount,
                   MachineInstr::MIFlag Flag, uint64_t *CFAOffset,
                   Register Scratch) {
  const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
  if ((Amount < -256 || Amount > 256) && Scratch.isValid()) {
    uint64_t Magnitude = Amount < 0 ? static_cast<uint64_t>(-Amount)
                                    : static_cast<uint64_t>(Amount);
    TII->materializeI32(MBB, Insert, DebugLoc(), Scratch,
                        static_cast<uint32_t>(Magnitude), Flag);
    unsigned Opcode = Amount < 0 ? SH::SUBrr : SH::ADDrr;
    MachineInstrBuilder Adjust =
        BuildMI(MBB, Insert, DebugLoc(), TII->get(Opcode), SH::R15)
            .addReg(SH::R15)
            .addReg(Scratch, RegState::Kill)
            .setMIFlag(Flag);
    Insert = std::next(Adjust->getIterator());
    if (CFAOffset && MF.needsFrameMoves()) {
      int64_t NewCFAOffset = static_cast<int64_t>(*CFAOffset) - Amount;
      if (NewCFAOffset < 0)
        report_fatal_error(
            "SH stack adjustment produced a negative CFA offset");
      *CFAOffset = static_cast<uint64_t>(NewCFAOffset);
      CFIInstBuilder(MBB, Insert, Flag).buildDefCFAOffset(*CFAOffset);
    }
    return Insert;
  }
  while (Amount != 0) {
    int64_t Chunk = getSHAdjustmentChunk(Amount);
    MachineInstrBuilder Adjust =
        BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::ADDri), SH::R15)
            .addReg(SH::R15)
            .addImm(Chunk)
            .setMIFlag(Flag);
    Insert = std::next(Adjust->getIterator());
    Amount -= Chunk;
    if (!CFAOffset || !MF.needsFrameMoves())
      continue;
    int64_t NewCFAOffset = static_cast<int64_t>(*CFAOffset) - Chunk;
    if (NewCFAOffset < 0)
      report_fatal_error("SH stack adjustment produced a negative CFA offset");
    *CFAOffset = static_cast<uint64_t>(NewCFAOffset);
    CFIInstBuilder(MBB, Insert, Flag).buildDefCFAOffset(*CFAOffset);
  }
  return Insert;
}

static void emitSHCallFrameAdjustment(MachineFunction &MF,
                                      MachineBasicBlock &MBB,
                                      MachineBasicBlock::iterator Insert,
                                      int64_t Amount, bool AdjustCFA,
                                      uint32_t Flags) {
  const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
  if (Amount < -256 || Amount > 256) {
    uint64_t Magnitude = Amount < 0 ? static_cast<uint64_t>(-Amount)
                                    : static_cast<uint64_t>(Amount);
    TII->materializeI32(MBB, Insert, Insert->getDebugLoc(), SH::R3,
                        static_cast<uint32_t>(Magnitude));
    unsigned Opcode = Amount < 0 ? SH::SUBrr : SH::ADDrr;
    BuildMI(MBB, Insert, Insert->getDebugLoc(), TII->get(Opcode), SH::R15)
        .addReg(SH::R15)
        .addReg(SH::R3, RegState::Kill)
        .setMIFlags(Flags);
    if (AdjustCFA && MF.needsFrameMoves())
      CFIInstBuilder(MBB, Insert, MachineInstr::NoFlags)
          .buildAdjustCFAOffset(-Amount);
    return;
  }
  while (Amount != 0) {
    int64_t Chunk = getSHAdjustmentChunk(Amount);
    BuildMI(MBB, Insert, Insert->getDebugLoc(), TII->get(SH::ADDri), SH::R15)
        .addReg(SH::R15)
        .addImm(Chunk)
        .setMIFlags(Flags);
    Amount -= Chunk;
    if (AdjustCFA && MF.needsFrameMoves())
      CFIInstBuilder(MBB, Insert, MachineInstr::NoFlags)
          .buildAdjustCFAOffset(-Chunk);
  }
}

static bool isSHPhysRegModified(const MachineFunction &MF, MCRegister Reg,
                                bool IncludeCallClobbers = false);

static bool needsSHPRSave(const MachineFunction &MF) {
  if (MF.getInfo<SHMachineFunctionInfo>()->usesResbank())
    return false;
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  return MFI.hasCalls() || MFI.isReturnAddressTaken() ||
         isSHPhysRegModified(MF, SH::PR);
}

static int getPRSpillFrameIndex(const MachineFunction &MF) {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const SHMachineFunctionInfo *FuncInfo = MF.getInfo<SHMachineFunctionInfo>();
  if (FuncInfo->hasReturnAddressFrameIndex())
    return FuncInfo->getReturnAddressFrameIndex();
  for (int FI = MFI.getObjectIndexBegin(); FI != 0; ++FI)
    if (MFI.isSpillSlotObjectIndex(FI) && MFI.getObjectSize(FI) == 4)
      return FI;
  report_fatal_error("SH non-leaf function is missing its PR save area");
}

struct SHHandlerStateSpill {
  enum class Kind { Special, FPR } SpillKind;
  MCRegister Reg;
  int FrameIndex;
  unsigned SaveOpcode;
  unsigned RestoreOpcode;
};

static SmallVector<SHHandlerStateSpill, 21>
getSHHandlerStateSpills(const MachineFunction &MF) {
  SmallVector<SHHandlerStateSpill, 21> Spills;
  const SHMachineFunctionInfo *FuncInfo = MF.getInfo<SHMachineFunctionInfo>();
  if (needsSHPRSave(MF))
    Spills.push_back({SHHandlerStateSpill::Kind::Special, SH::PR,
                      getPRSpillFrameIndex(MF), SH::STS_L_PR, SH::LDS_L_PR});
  if (FuncInfo->savesMACH())
    Spills.push_back({SHHandlerStateSpill::Kind::Special, SH::MACH,
                      FuncInfo->getMACHFrameIndex(), SH::STS_L_MACH,
                      SH::LDS_L_MACH});
  if (FuncInfo->savesMACL())
    Spills.push_back({SHHandlerStateSpill::Kind::Special, SH::MACL,
                      FuncInfo->getMACLFrameIndex(), SH::STS_L_MACL,
                      SH::LDS_L_MACL});
  if (FuncInfo->savesFPSCR())
    Spills.push_back({SHHandlerStateSpill::Kind::Special, SH::FPSCR,
                      FuncInfo->getFPSCRFrameIndex(), SH::STS_L_FPSCR,
                      SH::LDS_L_FPSCR});
  if (FuncInfo->savesFPUL())
    Spills.push_back({SHHandlerStateSpill::Kind::Special, SH::FPUL,
                      FuncInfo->getFPULFrameIndex(), SH::STS_L_FPUL,
                      SH::LDS_L_FPUL});
  for (const auto &[Reg, FrameIndex] : FuncInfo->getFPRSpills())
    Spills.push_back(
        {SHHandlerStateSpill::Kind::FPR, Reg, FrameIndex, SH::FLDS, SH::FSTS});
  llvm::sort(Spills, [&](const SHHandlerStateSpill &LHS,
                         const SHHandlerStateSpill &RHS) {
    return MF.getFrameInfo().getObjectOffset(LHS.FrameIndex) >
           MF.getFrameInfo().getObjectOffset(RHS.FrameIndex);
  });
  return Spills;
}

static int createSHHandlerStateSpillFrameIndex(MachineFunction &MF) {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  int64_t Offset = -4;
  for (int FI = MFI.getObjectIndexBegin(); FI != 0; ++FI)
    if (MFI.isFixedObjectIndex(FI) && MFI.getObjectOffset(FI) < 0)
      Offset = std::min(Offset, MFI.getObjectOffset(FI) - 4);
  return MFI.CreateFixedSpillStackObject(4, Offset, true);
}

int llvm::createSHPRSpillFrameIndex(MachineFunction &MF) {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  unsigned VarArgsSaveSize =
      MF.getInfo<SHMachineFunctionInfo>()->getVarArgsSaveSize();
  int64_t PROffset = -static_cast<int64_t>(VarArgsSaveSize + 4);
  // Nonvariadic byval formals may still use a fixed register/stack bridge.
  if (!MF.getFunction().isVarArg())
    for (int FI = MFI.getObjectIndexBegin(); FI != 0; ++FI)
      PROffset = std::min(PROffset, MFI.getObjectOffset(FI) - 4);
  return MFI.CreateFixedSpillStackObject(4, PROffset, true);
}

static uint64_t getSHRealignmentFirstAdjust(const MachineFunction &MF) {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  uint64_t FirstAdjust = 0;
  for (const CalleeSavedInfo &Info : MFI.getCalleeSavedInfo()) {
    int64_t Offset = MFI.getObjectOffset(Info.getFrameIdx());
    if (Offset >= 0)
      report_fatal_error("SH realigned callee-save area has an invalid offset");
    FirstAdjust = std::max(FirstAdjust, static_cast<uint64_t>(-Offset));
  }
  if (needsSHPRSave(MF)) {
    int64_t Offset = MFI.getObjectOffset(getPRSpillFrameIndex(MF));
    if (Offset >= 0)
      report_fatal_error("SH realigned PR save area has an invalid offset");
    FirstAdjust = std::max(FirstAdjust, static_cast<uint64_t>(-Offset));
  }
  if (MF.getInfo<SHMachineFunctionInfo>()->isHandler())
    for (const SHHandlerStateSpill &Spill : getSHHandlerStateSpills(MF)) {
      int64_t Offset = MFI.getObjectOffset(Spill.FrameIndex);
      if (Offset >= 0)
        report_fatal_error(
            "SH realigned handler state area has an invalid offset");
      FirstAdjust = std::max(FirstAdjust, static_cast<uint64_t>(-Offset));
    }
  if (FirstAdjust > MFI.getStackSize() || FirstAdjust % 4 != 0)
    report_fatal_error("SH realigned callee-save area exceeds the fixed frame");
  return FirstAdjust;
}

static bool canUseSHGPRPushes(const MachineFunction &MF, uint64_t FrameSize,
                              uint64_t Allocated) {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const SHMachineFunctionInfo *FuncInfo = MF.getInfo<SHMachineFunctionInfo>();
  ArrayRef<CalleeSavedInfo> CSI = MFI.getCalleeSavedInfo();
  if (CSI.empty() || MF.needsFrameMoves() || MFI.shouldRealignStack() ||
      FuncInfo->isHandler() || FuncInfo->hasStackSwitch() ||
      FrameSize < Allocated || FrameSize - Allocated < 4 * CSI.size())
    return false;
  for (auto [Index, Info] : llvm::enumerate(CSI)) {
    if (!SH::GPRRegClass.contains(Info.getReg()))
      return false;
    int64_t ExpectedOffset = -static_cast<int64_t>(Allocated + 4 * (Index + 1));
    if (MFI.getObjectOffset(Info.getFrameIdx()) != ExpectedOffset)
      return false;
  }
  return true;
}

// PEI invokes the spill/restore hooks before assigning ordinary frame offsets.
// Defer emission until emitPrologue/emitEpilogue, where the native CSI and
// final frame layout are authoritative. PEI still performs its CSI-based
// liveness update; no instruction search or second save-area plan is needed.
bool SHFrameLowering::spillCalleeSavedRegisters(
    MachineBasicBlock &, MachineBasicBlock::iterator, ArrayRef<CalleeSavedInfo>,
    const TargetRegisterInfo *) const {
  return true;
}

bool SHFrameLowering::restoreCalleeSavedRegisters(
    MachineBasicBlock &, MachineBasicBlock::iterator,
    MutableArrayRef<CalleeSavedInfo>, const TargetRegisterInfo *) const {
  return true;
}

static void emitSHCalleeSavedRegisters(const TargetFrameLowering &TFI,
                                       MachineFunction &MF,
                                       MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator Insert,
                                       bool Restore, bool PushPop) {
  const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
  const TargetRegisterInfo *TRI = MF.getSubtarget().getRegisterInfo();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  auto Emit = [&](const CalleeSavedInfo &Info) {
    if (PushPop) {
      MachineMemOperand *MMO = MF.getMachineMemOperand(
          MachinePointerInfo::getFixedStack(MF, Info.getFrameIdx()),
          Restore ? MachineMemOperand::MOLoad : MachineMemOperand::MOStore, 4,
          Align(4));
      if (Restore)
        BuildMI(MBB, Insert, MBB.findDebugLoc(Insert),
                TII->get(SH::MOVL_load_postinc), Info.getReg())
            .addDef(SH::R15)
            .addReg(SH::R15)
            .addMemOperand(MMO)
            .setMIFlag(MachineInstr::FrameDestroy);
      else
        BuildMI(MBB, Insert, MBB.findDebugLoc(Insert),
                TII->get(SH::MOVL_store_predec), SH::R15)
            .addReg(Info.getReg(),
                    getKillRegState(!MF.getRegInfo().isLiveIn(Info.getReg())))
            .addReg(SH::R15)
            .addMemOperand(MMO)
            .setMIFlag(MachineInstr::FrameSetup);
    } else if (Restore) {
      TFI.restoreCalleeSavedRegister(MBB, Insert, Info, TII, TRI);
    } else {
      TFI.spillCalleeSavedRegister(MBB, Insert, Info, TII, TRI);
    }
    if (MF.needsFrameMoves()) {
      CFIInstBuilder CFIBuilder(MBB, Insert,
                                Restore ? MachineInstr::FrameDestroy
                                        : MachineInstr::FrameSetup);
      if (Restore)
        CFIBuilder.buildRestore(Info.getReg());
      else
        CFIBuilder.buildOffset(Info.getReg(),
                               MFI.getObjectOffset(Info.getFrameIdx()));
    }
  };
  if (Restore)
    for (const CalleeSavedInfo &Info : llvm::reverse(MFI.getCalleeSavedInfo()))
      Emit(Info);
  else
    for (const CalleeSavedInfo &Info : MFI.getCalleeSavedInfo())
      Emit(Info);
}

static uint64_t getSHCalleeSavedPopBytes(const MachineFunction &MF,
                                         uint64_t FrameSize,
                                         uint64_t &LocalBytes) {
  LocalBytes = 0;
  ArrayRef<CalleeSavedInfo> CSI = MF.getFrameInfo().getCalleeSavedInfo();
  if (CSI.empty())
    return 0;
  uint64_t SavedBytes = 4 * CSI.size();
  int64_t FirstOffset =
      MF.getFrameInfo().getObjectOffset(CSI.front().getFrameIdx());
  if (FirstOffset > -4 || FirstOffset % 4 != 0)
    return 0;
  uint64_t Allocated = static_cast<uint64_t>(-FirstOffset) - 4;
  if (!canUseSHGPRPushes(MF, FrameSize, Allocated))
    return 0;
  LocalBytes = FrameSize - Allocated - SavedBytes;
  return SavedBytes;
}

static void emitSHStackSwitchEntry(MachineFunction &MF, MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator Insert) {
  const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
  SHMachineFunctionInfo *FuncInfo = MF.getInfo<SHMachineFunctionInfo>();
  StringRef Symbol = FuncInfo->getStackSwitchSymbol();
  if (Symbol.empty())
    report_fatal_error("SH stack switch is missing its stack-pointer symbol");
  MBB.addLiveIn(SH::R0);

  MachineMemOperand *OriginalStackMMO = MF.getMachineMemOperand(
      MachinePointerInfo(), MachineMemOperand::MOStore, 4, Align(4));
  BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::MOVL_store_predec), SH::R15)
      .addReg(SH::R0)
      .addReg(SH::R15)
      .addMemOperand(OriginalStackMMO)
      .setMIFlag(MachineInstr::FrameSetup);

  MF.setAlignment(std::max(MF.getAlignment(), Align(SHLiteralIslandAlignment)));
  SHConstantPoolValue *CPV =
      SHConstantPoolValue::create(MF.getFunction().getContext(), Symbol, 0,
                                  SHConstantPoolValue::Modifier::None);
  unsigned CPI = MF.getConstantPool()->getConstantPoolIndex(CPV, Align(4));
  MachineMemOperand *LiteralMMO = MF.getMachineMemOperand(
      MachinePointerInfo::getConstantPool(MF),
      MachineMemOperand::MOLoad | MachineMemOperand::MODereferenceable |
          MachineMemOperand::MOInvariant,
      4, Align(4));
  BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::MOVL_load_pc_island), SH::R0)
      .addConstantPoolIndex(CPI)
      .addImm(SHUnassignedLiteralIsland)
      .addMemOperand(LiteralMMO)
      .setMIFlag(MachineInstr::FrameSetup);

  MachineMemOperand *StackPointerObjectMMO = MF.getMachineMemOperand(
      MachinePointerInfo(), MachineMemOperand::MOLoad, 4, Align(4));
  BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::MOVL_load_reg), SH::R0)
      .addReg(SH::R0)
      .addMemOperand(StackPointerObjectMMO)
      .setMIFlag(MachineInstr::FrameSetup);

  MachineMemOperand *AlternateStackMMO = MF.getMachineMemOperand(
      MachinePointerInfo(), MachineMemOperand::MOStore, 4, Align(4));
  BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::MOVL_store_predec), SH::R0)
      .addReg(SH::R15)
      .addReg(SH::R0)
      .addMemOperand(AlternateStackMMO)
      .setMIFlag(MachineInstr::FrameSetup);
  BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::MOVrr), SH::R15)
      .addReg(SH::R0)
      .setMIFlag(MachineInstr::FrameSetup);
}

static void emitSHStackSwitchExit(MachineFunction &MF, MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator Insert) {
  const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
  MachineMemOperand *AlternateStackMMO = MF.getMachineMemOperand(
      MachinePointerInfo(), MachineMemOperand::MOLoad, 4, Align(4));
  BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::MOVL_load_reg), SH::R15)
      .addReg(SH::R15)
      .addMemOperand(AlternateStackMMO)
      .setMIFlag(MachineInstr::FrameDestroy);
  MachineMemOperand *OriginalStackMMO = MF.getMachineMemOperand(
      MachinePointerInfo(), MachineMemOperand::MOLoad, 4, Align(4));
  BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::MOVL_load_postinc), SH::R0)
      .addDef(SH::R15)
      .addReg(SH::R15)
      .addMemOperand(OriginalStackMMO)
      .setMIFlag(MachineInstr::FrameDestroy);
}

void SHFrameLowering::emitPrologue(MachineFunction &MF,
                                   MachineBasicBlock &MBB) const {
  uint64_t StackSize = requireSupportedSHFrame(MF);
  const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
  MachineBasicBlock::iterator Insert = MBB.begin();
  SHMachineFunctionInfo *FuncInfo = MF.getInfo<SHMachineFunctionInfo>();
  if (FuncInfo->hasStackSwitch())
    emitSHStackSwitchEntry(MF, MBB, Insert);
  if (StackSize == 0)
    return;

  MachineFrameInfo &MFI = MF.getFrameInfo();
  bool Realign = MFI.shouldRealignStack();
  // The static chain arrives in r3. Choose a caller-clobbered register that is
  // actually dead on entry, before any argument copies have executed.
  Register Scratch;
  if (!FuncInfo->isHandler()) {
    LiveRegUnits Live(*MF.getSubtarget().getRegisterInfo());
    Live.addLiveIns(MBB);
    for (MCRegister Reg : {SH::R3, SH::R1, SH::R0, SH::R2})
      if (Live.available(Reg)) {
        Scratch = Reg;
        break;
      }
  }
  uint64_t FirstAdjust = Realign ? getSHRealignmentFirstAdjust(MF) : StackSize;
  MF.getInfo<SHMachineFunctionInfo>()->setRealignmentFirstAdjust(FirstAdjust);
  uint64_t CFAOffset = 0;
  uint64_t Allocated = 0;
  if (FuncInfo->isHandler()) {
    for (const SHHandlerStateSpill &Spill : getSHHandlerStateSpills(MF)) {
      int64_t Offset = MFI.getObjectOffset(Spill.FrameIndex);
      if (Offset > -4 || Offset % 4 != 0 ||
          static_cast<uint64_t>(-Offset) > FirstAdjust)
        report_fatal_error(
            "SH handler state save area has an invalid frame offset");
      uint64_t BeforeSpill = static_cast<uint64_t>(-Offset) - 4;
      if (BeforeSpill < Allocated)
        report_fatal_error("SH handler state save areas overlap");
      if (BeforeSpill != Allocated)
        Insert = emitSHSPAdjustment(
            MF, MBB, Insert, -static_cast<int64_t>(BeforeSpill - Allocated),
            MachineInstr::FrameSetup, &CFAOffset, Register());
      MachineMemOperand *MMO = MF.getMachineMemOperand(
          MachinePointerInfo::getFixedStack(MF, Spill.FrameIndex),
          MachineMemOperand::MOStore, 4, Align(4));
      MachineInstrBuilder Save;
      if (Spill.SpillKind == SHHandlerStateSpill::Kind::FPR) {
        BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::FLDS))
            .addReg(Spill.Reg)
            .setMIFlag(MachineInstr::FrameSetup);
        Save =
            BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::STS_L_FPUL), SH::R15)
                .addReg(SH::R15)
                .addMemOperand(MMO)
                .setMIFlag(MachineInstr::FrameSetup);
      } else {
        Save = BuildMI(MBB, Insert, DebugLoc(), TII->get(Spill.SaveOpcode),
                       SH::R15)
                   .addReg(SH::R15)
                   .addMemOperand(MMO)
                   .setMIFlag(MachineInstr::FrameSetup);
      }
      Insert = std::next(Save->getIterator());
      CFAOffset += 4;
      Allocated = static_cast<uint64_t>(-Offset);
      if (MF.needsFrameMoves()) {
        CFIInstBuilder CFIBuilder(MBB, Insert, MachineInstr::FrameSetup);
        CFIBuilder.buildDefCFAOffset(CFAOffset);
        CFIBuilder.buildOffset(Spill.Reg, Offset);
      }
    }
  } else if (needsSHPRSave(MF)) {
    int FI = getPRSpillFrameIndex(MF);
    int64_t PROffset = MFI.getObjectOffset(FI);
    unsigned VarArgsSaveSize =
        MF.getInfo<SHMachineFunctionInfo>()->getVarArgsSaveSize();
    bool HasDedicatedReturnAddress =
        MF.getInfo<SHMachineFunctionInfo>()->hasReturnAddressFrameIndex();
    bool IsValidOffset =
        MF.getFunction().isVarArg() && !HasDedicatedReturnAddress
            ? PROffset == -static_cast<int64_t>(VarArgsSaveSize + 4)
            : PROffset <= -4 && PROffset % 4 == 0;
    if (!IsValidOffset || static_cast<uint64_t>(-PROffset) > FirstAdjust)
      report_fatal_error("SH PR save area has an invalid frame offset");
    uint64_t BeforePR = static_cast<uint64_t>(-PROffset) - 4;
    Insert =
        emitSHSPAdjustment(MF, MBB, Insert, -static_cast<int64_t>(BeforePR),
                           MachineInstr::FrameSetup, &CFAOffset, Scratch);
    MachineMemOperand *MMO =
        MF.getMachineMemOperand(MachinePointerInfo::getFixedStack(MF, FI),
                                MachineMemOperand::MOStore, 4, Align(4));
    MachineInstrBuilder Save =
        BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::STS_L_PR), SH::R15)
            .addReg(SH::R15)
            .addMemOperand(MMO)
            .setMIFlag(MachineInstr::FrameSetup);
    Insert = std::next(Save->getIterator());
    CFAOffset += 4;
    Allocated = static_cast<uint64_t>(-PROffset);
    if (MF.needsFrameMoves()) {
      CFIInstBuilder CFIBuilder(MBB, Insert, MachineInstr::FrameSetup);
      CFIBuilder.buildDefCFAOffset(CFAOffset);
      CFIBuilder.buildOffset(SH::PR, PROffset);
    }
  }
  uint64_t CalleeSavedPushBytes = canUseSHGPRPushes(MF, FirstAdjust, Allocated)
                                      ? 4 * MFI.getCalleeSavedInfo().size()
                                      : 0;
  if (CalleeSavedPushBytes)
    emitSHCalleeSavedRegisters(*this, MF, MBB, Insert, false, true);
  Allocated += CalleeSavedPushBytes;
  if (FirstAdjust != Allocated)
    Insert = emitSHSPAdjustment(MF, MBB, Insert,
                                -static_cast<int64_t>(FirstAdjust - Allocated),
                                MachineInstr::FrameSetup, &CFAOffset, Scratch);
  if (!CalleeSavedPushBytes)
    emitSHCalleeSavedRegisters(*this, MF, MBB, Insert, false, false);
  if (hasFP(MF)) {
    MachineBasicBlock::iterator FPInsert =
        MFI.getCalleeSavedInfo().empty() ? MBB.begin() : Insert;
    MachineInstrBuilder Setup =
        BuildMI(MBB, FPInsert, DebugLoc(), TII->get(SH::MOVrr), SH::R14)
            .addReg(SH::R15)
            .setMIFlag(MachineInstr::FrameSetup);
    if (MF.needsFrameMoves())
      CFIInstBuilder(MBB, std::next(Setup->getIterator()),
                     MachineInstr::FrameSetup)
          .buildDefCFA(SH::R14, FirstAdjust);
    if (Realign) {
      // Supported calling conventions leave at least one of r0-r3 free.
      assert(Scratch && "SH realignment requires a dead entry register");
      MachineBasicBlock::iterator RealignInsert = FPInsert;
      if (StackSize != FirstAdjust)
        RealignInsert =
            emitSHSPAdjustment(MF, MBB, RealignInsert,
                               -static_cast<int64_t>(StackSize - FirstAdjust),
                               MachineInstr::FrameSetup, nullptr, Scratch);
      Align MaxAlign = MFI.getMaxAlign();
      TII->materializeI32(
          MBB, RealignInsert, DebugLoc(), Scratch,
          static_cast<uint32_t>(-static_cast<uint32_t>(MaxAlign.value())),
          MachineInstr::FrameSetup);
      BuildMI(MBB, RealignInsert, DebugLoc(), TII->get(SH::ANDrr), SH::R15)
          .addReg(SH::R15)
          .addReg(Scratch, RegState::Kill)
          .setMIFlag(MachineInstr::FrameSetup);
      BuildMI(MBB, RealignInsert, DebugLoc(), TII->get(SH::MOVrr), SH::R13)
          .addReg(SH::R15)
          .setMIFlag(MachineInstr::FrameSetup);
    }
  }
}

static void emitSHHandlerFrameAddress(MachineFunction &MF,
                                      MachineBasicBlock &MBB,
                                      MachineBasicBlock::iterator Insert,
                                      uint64_t StackSize, int FrameIndex) {
  const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
  int64_t ByteOffset = static_cast<int64_t>(StackSize) +
                       MF.getFrameInfo().getObjectOffset(FrameIndex);
  if (ByteOffset < 0 || ByteOffset > static_cast<int64_t>(StackSize))
    report_fatal_error("SH handler FPU save area has an invalid frame offset");
  BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::MOVrr), SH::R0)
      .addReg(SH::R15)
      .setMIFlag(MachineInstr::FrameDestroy);
  while (ByteOffset != 0) {
    int64_t Chunk = std::min<int64_t>(ByteOffset, 127);
    BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::ADDri), SH::R0)
        .addReg(SH::R0)
        .addImm(Chunk)
        .setMIFlag(MachineInstr::FrameDestroy);
    ByteOffset -= Chunk;
  }
}

static void emitSHHandlerFPUStateRestores(MachineFunction &MF,
                                          MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator Insert,
                                          uint64_t StackSize) {
  SHMachineFunctionInfo *FuncInfo = MF.getInfo<SHMachineFunctionInfo>();
  if (!FuncInfo->savesFPSCR() && !FuncInfo->savesFPUL() &&
      !FuncInfo->hasFPRSpills())
    return;
  const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
  auto RestoreSpecial = [&](MCRegister Reg, int FrameIndex, unsigned Opcode) {
    emitSHHandlerFrameAddress(MF, MBB, Insert, StackSize, FrameIndex);
    MachineMemOperand *MMO = MF.getMachineMemOperand(
        MachinePointerInfo::getFixedStack(MF, FrameIndex),
        MachineMemOperand::MOLoad, 4, Align(4));
    BuildMI(MBB, Insert, DebugLoc(), TII->get(Opcode), SH::R0)
        .addReg(SH::R0)
        .addMemOperand(MMO)
        .setMIFlag(MachineInstr::FrameDestroy);
    if (MF.needsFrameMoves())
      CFIInstBuilder(MBB, Insert, MachineInstr::FrameDestroy).buildRestore(Reg);
  };
  if (FuncInfo->savesFPSCR())
    RestoreSpecial(SH::FPSCR, FuncInfo->getFPSCRFrameIndex(), SH::LDS_L_FPSCR);
  if (FuncInfo->hasFPRSpills()) {
    ArrayRef<std::pair<MCRegister, int>> FPRSpills = FuncInfo->getFPRSpills();
    emitSHHandlerFrameAddress(MF, MBB, Insert, StackSize,
                              FPRSpills.back().second);
    int64_t ExpectedOffset =
        MF.getFrameInfo().getObjectOffset(FPRSpills.back().second);
    for (const auto &[Reg, FrameIndex] : llvm::reverse(FPRSpills)) {
      if (MF.getFrameInfo().getObjectOffset(FrameIndex) != ExpectedOffset)
        report_fatal_error("SH handler FPU save areas are not contiguous");
      MachineMemOperand *MMO = MF.getMachineMemOperand(
          MachinePointerInfo::getFixedStack(MF, FrameIndex),
          MachineMemOperand::MOLoad, 4, Align(4));
      BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::LDS_L_FPUL), SH::R0)
          .addReg(SH::R0)
          .addMemOperand(MMO)
          .setMIFlag(MachineInstr::FrameDestroy);
      BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::FSTS), Reg)
          .setMIFlag(MachineInstr::FrameDestroy);
      if (MF.needsFrameMoves())
        CFIInstBuilder(MBB, Insert, MachineInstr::FrameDestroy)
            .buildRestore(Reg);
      ExpectedOffset += 4;
    }
    if (FuncInfo->savesFPUL()) {
      if (MF.getFrameInfo().getObjectOffset(FuncInfo->getFPULFrameIndex()) !=
          ExpectedOffset)
        report_fatal_error("SH handler FPUL save area is not contiguous");
      MachineMemOperand *MMO = MF.getMachineMemOperand(
          MachinePointerInfo::getFixedStack(MF, FuncInfo->getFPULFrameIndex()),
          MachineMemOperand::MOLoad, 4, Align(4));
      BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::LDS_L_FPUL), SH::R0)
          .addReg(SH::R0)
          .addMemOperand(MMO)
          .setMIFlag(MachineInstr::FrameDestroy);
      if (MF.needsFrameMoves())
        CFIInstBuilder(MBB, Insert, MachineInstr::FrameDestroy)
            .buildRestore(SH::FPUL);
    }
  } else if (FuncInfo->savesFPUL())
    RestoreSpecial(SH::FPUL, FuncInfo->getFPULFrameIndex(), SH::LDS_L_FPUL);
}

void SHFrameLowering::emitEpilogue(MachineFunction &MF,
                                   MachineBasicBlock &MBB) const {
  uint64_t TotalStackSize = requireSupportedSHFrame(MF);
  MachineBasicBlock::iterator Insert = MBB.getFirstTerminator();
  SHMachineFunctionInfo *FuncInfo = MF.getInfo<SHMachineFunctionInfo>();
  if (TotalStackSize == 0) {
    if (FuncInfo->hasStackSwitch())
      emitSHStackSwitchExit(MF, MBB, Insert);
    return;
  }
  bool Realign = MF.getFrameInfo().shouldRealignStack();
  uint64_t StackSize =
      Realign ? getSHRealignmentFirstAdjust(MF) : TotalStackSize;
  uint64_t CFAOffset = StackSize;
  // Preserve the ABI-mandated r3 stack adjustment on an EH return.
  Register Scratch = FuncInfo->isHandler()                  ? Register()
                     : Insert->getOpcode() == SH::EH_RETURN ? Register(SH::R0)
                                                            : Register(SH::R3);
  const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
  uint64_t CalleeSavedLocalBytes = 0;
  uint64_t CalleeSavedPopBytes =
      getSHCalleeSavedPopBytes(MF, StackSize, CalleeSavedLocalBytes);
  if (hasFP(MF)) {
    MachineInstrBuilder Reset =
        BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::MOVrr), SH::R15)
            .addReg(SH::R14)
            .setMIFlag(MachineInstr::FrameDestroy);
    if (MF.needsFrameMoves())
      CFIInstBuilder(MBB, std::next(Reset->getIterator()),
                     MachineInstr::FrameDestroy)
          .buildDefCFA(SH::R15, StackSize);
  }
  if (CalleeSavedPopBytes) {
    if (CalleeSavedLocalBytes)
      emitSHSPAdjustment(MF, MBB, Insert,
                         static_cast<int64_t>(CalleeSavedLocalBytes),
                         MachineInstr::FrameDestroy, &CFAOffset, Scratch);
    StackSize -= CalleeSavedPopBytes + CalleeSavedLocalBytes;
  }

  if (FuncInfo->isHandler())
    emitSHHandlerFPUStateRestores(MF, MBB, Insert, TotalStackSize);
  emitSHCalleeSavedRegisters(*this, MF, MBB, Insert, true,
                             CalleeSavedPopBytes != 0);
  if (FuncInfo->isHandler()) {
    uint64_t CurrentDepth = StackSize;
    SmallVector<SHHandlerStateSpill, 21> Spills = getSHHandlerStateSpills(MF);
    auto IsFPUState = [](const SHHandlerStateSpill &Spill) {
      return Spill.SpillKind == SHHandlerStateSpill::Kind::FPR ||
             Spill.Reg == SH::FPUL || Spill.Reg == SH::FPSCR;
    };
    for (size_t I = Spills.size(); I != 0;) {
      const SHHandlerStateSpill &Spill = Spills[I - 1];
      int64_t Offset = MF.getFrameInfo().getObjectOffset(Spill.FrameIndex);
      if (Offset > -4 || Offset % 4 != 0)
        report_fatal_error(
            "SH handler state restore area has an invalid frame offset");
      uint64_t Depth = static_cast<uint64_t>(-Offset);
      if (Depth > CurrentDepth)
        report_fatal_error("SH handler state restore area exceeds the frame");
      if (IsFPUState(Spill)) {
        uint64_t TargetDepth = Depth - 4;
        --I;
        while (I != 0 && IsFPUState(Spills[I - 1])) {
          int64_t NextOffset =
              MF.getFrameInfo().getObjectOffset(Spills[I - 1].FrameIndex);
          if (NextOffset > -4 || NextOffset % 4 != 0 ||
              static_cast<uint64_t>(-NextOffset) != TargetDepth)
            break;
          TargetDepth -= 4;
          --I;
        }
        if (CurrentDepth != TargetDepth)
          Insert = emitSHSPAdjustment(
              MF, MBB, Insert, static_cast<int64_t>(CurrentDepth - TargetDepth),
              MachineInstr::FrameDestroy, &CFAOffset, Scratch);
        CurrentDepth = TargetDepth;
        continue;
      }
      if (CurrentDepth != Depth)
        Insert = emitSHSPAdjustment(
            MF, MBB, Insert, static_cast<int64_t>(CurrentDepth - Depth),
            MachineInstr::FrameDestroy, &CFAOffset, Scratch);
      MachineMemOperand *MMO = MF.getMachineMemOperand(
          MachinePointerInfo::getFixedStack(MF, Spill.FrameIndex),
          MachineMemOperand::MOLoad, 4, Align(4));
      BuildMI(MBB, Insert, DebugLoc(), TII->get(Spill.RestoreOpcode), SH::R15)
          .addReg(SH::R15)
          .addMemOperand(MMO)
          .setMIFlag(MachineInstr::FrameDestroy);
      CurrentDepth = Depth - 4;
      if (MF.needsFrameMoves()) {
        CFIInstBuilder CFIBuilder(MBB, Insert, MachineInstr::FrameDestroy);
        CFIBuilder.buildRestore(Spill.Reg);
        CFAOffset -= 4;
        CFIBuilder.buildDefCFAOffset(CFAOffset);
      }
      --I;
    }
    if (CurrentDepth != 0)
      emitSHSPAdjustment(MF, MBB, Insert, static_cast<int64_t>(CurrentDepth),
                         MachineInstr::FrameDestroy, &CFAOffset, Scratch);
    if (FuncInfo->hasStackSwitch())
      emitSHStackSwitchExit(MF, MBB, Insert);
    return;
  }
  uint64_t BeforePR = 0;
  if (needsSHPRSave(MF)) {
    int FI = getPRSpillFrameIndex(MF);
    int64_t PROffset = MF.getFrameInfo().getObjectOffset(FI);
    unsigned VarArgsSaveSize =
        MF.getInfo<SHMachineFunctionInfo>()->getVarArgsSaveSize();
    bool HasDedicatedReturnAddress =
        MF.getInfo<SHMachineFunctionInfo>()->hasReturnAddressFrameIndex();
    bool IsValidOffset =
        MF.getFunction().isVarArg() && !HasDedicatedReturnAddress
            ? PROffset == -static_cast<int64_t>(VarArgsSaveSize + 4)
            : PROffset <= -4 && PROffset % 4 == 0;
    if (!IsValidOffset || static_cast<uint64_t>(-PROffset) > StackSize)
      report_fatal_error("SH PR restore area has an invalid frame offset");
    BeforePR = static_cast<uint64_t>(-PROffset) - 4;
    StackSize -= static_cast<uint64_t>(-PROffset);
  }
  if (StackSize != 0) {
    Insert =
        emitSHSPAdjustment(MF, MBB, Insert, StackSize,
                           MachineInstr::FrameDestroy, &CFAOffset, Scratch);
  }
  if (needsSHPRSave(MF)) {
    int FI = getPRSpillFrameIndex(MF);
    MachineMemOperand *MMO =
        MF.getMachineMemOperand(MachinePointerInfo::getFixedStack(MF, FI),
                                MachineMemOperand::MOLoad, 4, Align(4));
    BuildMI(MBB, Insert, DebugLoc(), TII->get(SH::LDS_L_PR), SH::R15)
        .addReg(SH::R15)
        .addMemOperand(MMO)
        .setMIFlag(MachineInstr::FrameDestroy);
    if (MF.needsFrameMoves()) {
      CFIInstBuilder CFIBuilder(MBB, Insert, MachineInstr::FrameDestroy);
      CFIBuilder.buildRestore(SH::PR);
      CFAOffset -= 4;
      CFIBuilder.buildDefCFAOffset(BeforePR);
    }
    Insert =
        emitSHSPAdjustment(MF, MBB, Insert, BeforePR,
                           MachineInstr::FrameDestroy, &CFAOffset, Scratch);
  }
}

static bool isSHPhysRegModified(const MachineFunction &MF, MCRegister Reg,
                                bool IncludeCallClobbers) {
  const TargetRegisterInfo *TRI = MF.getSubtarget().getRegisterInfo();
  for (const MachineBasicBlock &MBB : MF)
    for (const MachineInstr &MI : MBB) {
      if (MI.isInlineAsm() && MI.modifiesRegister(Reg, TRI))
        return true;
      for (const MachineOperand &MO : MI.operands()) {
        if (MO.isReg() && MO.isDef() && MO.getReg().isPhysical() &&
            TRI->regsOverlap(MO.getReg(), Reg))
          return true;
        if (IncludeCallClobbers && MI.isCall() && MO.isRegMask() &&
            MO.clobbersPhysReg(Reg))
          return true;
      }
    }
  return false;
}

static bool mayModifySHFPSCRStatus(const MachineFunction &MF) {
  for (const MachineBasicBlock &MBB : MF)
    for (const MachineInstr &MI : MBB)
      if (SHII::mayModifyFPSCRStatus(MI.getDesc()))
        return true;
  return false;
}

void SHFrameLowering::determineCalleeSaves(MachineFunction &MF,
                                           BitVector &SavedRegs,
                                           RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);
  SHMachineFunctionInfo *FuncInfo = MF.getInfo<SHMachineFunctionInfo>();
  if (FuncInfo->noSaveLowRegs() &&
      !MF.getSubtarget<SHSubtarget>().hasBankedGPRs())
    report_fatal_error(
        "SH nosave_low_regs requires hardware-banked r0-r7 registers");
  if (FuncInfo->usesResbank() && !MF.getSubtarget<SHSubtarget>().hasSH2A())
    report_fatal_error("SH resbank requires an SH2A target");
  if (FuncInfo->hasStackSwitch() && MF.getTarget().isPositionIndependent())
    report_fatal_error(
        "SH sp_switch is unsupported for position-independent code");
  if (FuncInfo->isHandler() && MF.getFrameInfo().shouldRealignStack())
    report_fatal_error(
        "SH core hardware handlers do not support stack realignment");
  if (FuncInfo->isInterruptHandler() && !FuncInfo->usesResbank()) {
    if (MF.getFrameInfo().hasCalls() && !FuncInfo->noSaveLowRegs()) {
      for (MCRegister Reg :
           {SH::R0, SH::R1, SH::R2, SH::R3, SH::R4, SH::R5, SH::R6, SH::R7})
        if (Reg != SH::R0 || !FuncInfo->hasStackSwitch())
          SavedRegs.set(Reg);
      FuncInfo->setNeedsMACHSave();
      FuncInfo->setNeedsMACLSave();
    }
    if (MF.getRegInfo().isPhysRegModified(SH::MACH))
      FuncInfo->setNeedsMACHSave();
    if (MF.getRegInfo().isPhysRegModified(SH::MACL))
      FuncInfo->setNeedsMACLSave();
  }
  if (FuncInfo->isHandler()) {
    const SHSubtarget &ST = MF.getSubtarget<SHSubtarget>();
    for (MCRegister Reg : SH::XFRegClass)
      if (isSHPhysRegModified(MF, Reg))
        report_fatal_error(
            "SH hardware handlers do not support alternate-bank FPU state");
    if (FuncInfo->isInterruptHandler()) {
      bool SavesFPR = false;
      for (MCRegister Reg : SH::FRRegClass)
        SavedRegs.reset(Reg);
      // Hardware effects are independent of argument transport. In
      // particular, the soft argument ABI can clobber every FR register.
      // Special helper masks may instead preserve all FPU state.
      for (MCRegister Reg : SH::FRRegClass)
        if (isSHPhysRegModified(MF, Reg, ST.hasSingleFPU())) {
          SavedRegs.set(Reg);
          SavesFPR = true;
        }
      if (SavesFPR || isSHPhysRegModified(MF, SH::FPUL, ST.hasSingleFPU()))
        FuncInfo->setNeedsFPULSave();
      if (FuncInfo->savesFPUL() && !ST.hasSingleFPU())
        report_fatal_error(
            "SH hardware handler FPU state requires an FPU target");
      if (SavesFPR || FuncInfo->savesFPUL())
        SavedRegs.set(SH::R0);
    }
    // Save the full architectural value, including cause and sticky
    // flags. Status effects do not invalidate the FPSCR mode analysis.
    if (isSHPhysRegModified(MF, SH::FPSCR, ST.hasSingleFPU()) ||
        mayModifySHFPSCRStatus(MF)) {
      if (!ST.hasSingleFPU())
        report_fatal_error(
            "SH hardware handler FPU state requires an FPU target");
      FuncInfo->setNeedsFPSCRSave();
      if (FuncInfo->isInterruptHandler())
        SavedRegs.set(SH::R0);
    }
    if (ST.hasDSP())
      for (MCRegister Reg : {SH::A0, SH::A1, SH::X0, SH::X1, SH::Y0, SH::Y1,
                             SH::M0, SH::M1, SH::DSR, SH::RS, SH::RE, SH::MOD})
        if (isSHPhysRegModified(MF, Reg))
          report_fatal_error(
              "SH core hardware handlers do not preserve DSP state");
  }
  if (needsSHPRSave(MF))
    SavedRegs.set(SH::PR);
  else
    SavedRegs.reset(SH::PR);
  if (FuncInfo->usesPICBase() && !FuncInfo->usesResbank())
    SavedRegs.set(SH::R12);
  if (hasFP(MF) && !FuncInfo->usesResbank())
    SavedRegs.set(SH::R14);
  if (MF.getFrameInfo().shouldRealignStack())
    SavedRegs.set(SH::R13);
  if (MF.callsEHReturn())
    for (MCRegister Reg : {SH::R4, SH::R5, SH::R6, SH::R7})
      SavedRegs.set(Reg);
}

static bool needsSHScavengingFrameIndex(const MachineFunction &MF) {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  uint64_t MaximumOffset = MFI.estimateStackSize(MF);
  if (!MF.getSubtarget<SHSubtarget>().getFrameLowering()->hasReservedCallFrame(
          MF))
    MaximumOffset += MFI.getMaxCallFrameSize();
  // CSR emission is deferred until the frame offsets are final. Reserve the
  // same scavenger slot that their ordinary spill instructions required before
  // deciding whether the final layout can use compact GPR pushes instead.
  if (!MF.getFunction().hasFnAttribute(Attribute::Naked))
    for (const CalleeSavedInfo &Info : MFI.getCalleeSavedInfo())
      if (SH::FRRegClass.contains(Info.getReg()) ||
          SH::DRRegClass.contains(Info.getReg()) ||
          (SH::GPRRegClass.contains(Info.getReg()) && MaximumOffset > 60))
        return true;
  for (const MachineBasicBlock &MBB : MF)
    for (const MachineInstr &MI : MBB) {
      bool HasFrameIndex = llvm::any_of(
          MI.operands(), [](const MachineOperand &MO) { return MO.isFI(); });
      if (!HasFrameIndex)
        continue;
      switch (MI.getOpcode()) {
      case SH::FMOV_D_LOAD_FI:
      case SH::FMOV_D_STORE_FI:
      case SH::FMOV_S_LOAD_FI:
      case SH::FMOV_S_STORE_FI:
      case SH::FMOV_V_LOAD_FI:
      case SH::FMOV_V_STORE_FI:
        return true;
      case SH::MOVB_store_frame:
        if (MaximumOffset > 15)
          return true;
        break;
      case SH::MOVW_store_frame:
        if (MaximumOffset > 30)
          return true;
        break;
      case SH::MOVL_store_disp:
        if (MaximumOffset > 60)
          return true;
        break;
      }
    }
  return false;
}

static unsigned estimateSHFrameAddressBytes(int64_t Offset) {
  if (Offset == 0)
    return 2;
  if (Offset > 0 && Offset <= 127)
    return 4;
  if ((Offset > 0 && Offset <= 254) ||
      (Offset % 4 == 0 && isInt<8>(Offset / 4)))
    return 6;
  // Charge the load, add, and word/long literal. Island sharing and final
  // padding are not known at frame layout time.
  return Offset > 0 && Offset <= 32767 ? 6 : 8;
}

static unsigned estimateSHFrameAccessBytes(const MachineInstr &MI,
                                           int64_t Offset, bool R0Available) {
  unsigned Opcode = MI.getOpcode();
  unsigned Width =
      Opcode == SH::MOVL_load_disp || Opcode == SH::MOVL_store_disp     ? 4
      : Opcode == SH::MOVW_load_frame || Opcode == SH::MOVW_store_frame ? 2
                                                                        : 1;
  if (Offset >= 0 && Offset <= 15 * Width && Offset % Width == 0)
    return 2;
  bool IsLoad = MI.mayLoad();
  Register Data = MI.getOperand(0).getReg();
  if (Width < 4 && Offset > 0 && Offset <= 127 && (IsLoad || Data != SH::R0))
    return 4;
  if (Width == 4 && Offset >= 64 && Offset <= 127 &&
      ((IsLoad && Data == SH::R0) || (R0Available && Data != SH::R0)))
    return 4;
  if (Width == 4 && Offset >= 128 && Offset <= 187)
    return 2 + estimateSHFrameAddressBytes(Offset - 60);
  return 2 + estimateSHFrameAddressBytes(Offset);
}

static unsigned estimateSHSPAdjustmentBytes(int64_t Amount) {
  uint64_t Magnitude = Amount < 0 ? -Amount : Amount;
  if (Magnitude > 256)
    return Magnitude <= 32767 ? 6 : 8;
  return 2 * divideCeil(Magnitude, Amount < 0 ? 128 : 127);
}

static bool isProfitableSHReservedCallFrame(const MachineFunction &MF,
                                            RegScavenger *RS) {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const auto &TFI = *MF.getSubtarget<SHSubtarget>().getFrameLowering();
  int64_t Area = MFI.getMaxCallFrameSize();
  if (!Area || Area > 127 || MFI.getMaxAlign() > Align(4) ||
      MFI.getUseLocalStackAllocationBlock() ||
      MFI.getStackProtectorIndex() >= 0)
    return true;
  for (const CalleeSavedInfo &Info : MFI.getCalleeSavedInfo())
    if (!SH::GPRRegClass.contains(Info.getReg()))
      return true;

  // Predict the ordinary PEI layout in the established object order. This
  // leaves object placement unchanged and compares the same allocated values
  // with and without the outgoing area below them.
  DenseMap<int, int64_t> Offsets;
  int64_t Size = 0;
  for (int FI = MFI.getObjectIndexBegin(); FI < 0; ++FI) {
    if (MFI.getStackID(FI) != TargetStackID::Default)
      return true;
    Offsets[FI] = MFI.getObjectOffset(FI);
    Size = std::max(Size, -Offsets[FI]);
  }
  auto Allocate = [&](int FI) {
    Size = alignTo(Size + MFI.getObjectSize(FI), MFI.getObjectAlign(FI));
    Offsets[FI] = -Size;
  };
  for (int FI = MFI.getObjectIndexBegin(); FI < MFI.getObjectIndexEnd(); ++FI)
    if (MFI.isCalleeSavedObjectIndex(FI))
      Allocate(FI);
  int64_t SavedSize = Size;
  SmallVector<int, 16> Objects;
  for (int FI = 0; FI < MFI.getObjectIndexEnd(); ++FI) {
    if (MFI.isDeadObjectIndex(FI) || MFI.isCalleeSavedObjectIndex(FI) ||
        (RS && RS->isScavengingFrameIndex(FI)))
      continue;
    if (MFI.getStackID(FI) != TargetStackID::Default)
      return true;
    Objects.push_back(FI);
  }
  if (MF.getTarget().getOptLevel() != CodeGenOptLevel::None &&
      MF.getTarget().Options.StackSymbolOrdering)
    TFI.orderFrameObjects(MF, Objects);
  for (int FI : Objects)
    Allocate(FI);
  SmallVector<int, 2> ScavengerSlots;
  if (RS)
    RS->getScavengingFrameIndices(ScavengerSlots);
  for (int FI : ScavengerSlots)
    Allocate(FI);
  Size = alignTo(Size, Align(4));
  int64_t LocalSize = Size - SavedSize;
  int64_t Cost = estimateSHSPAdjustmentBytes(-LocalSize - Area) -
                 static_cast<int64_t>(estimateSHSPAdjustmentBytes(-LocalSize));
  const auto &TRI = *MF.getSubtarget<SHSubtarget>().getRegisterInfo();
  for (const MachineBasicBlock &MBB : MF) {
    DenseMap<const MachineInstr *, int64_t> Adjustments;
    int64_t SPAdj = MBB.getCallFrameSize();
    for (const MachineInstr &MI : MBB) {
      Adjustments[&MI] = SPAdj;
      if (MI.getOpcode() == SH::ADJCALLSTACKDOWN ||
          MI.getOpcode() == SH::ADJCALLSTACKUP) {
        int64_t Amount = MI.getOperand(0).getImm();
        if (MI.getOpcode() == SH::ADJCALLSTACKDOWN)
          Amount = -Amount;
        SPAdj -= Amount;
        Cost -= estimateSHSPAdjustmentBytes(Amount);
      }
      if (MI.isReturn())
        Cost += estimateSHSPAdjustmentBytes(LocalSize + Area) -
                static_cast<int64_t>(estimateSHSPAdjustmentBytes(LocalSize));
    }
    LiveRegUnits Live(TRI);
    Live.addLiveOuts(MBB);
    for (const MachineInstr &MI : llvm::reverse(MBB)) {
      Live.stepBackward(MI);
      if (MI.isDebugInstr() || MI.getFlag(MachineInstr::FrameSetup) ||
          MI.getFlag(MachineInstr::FrameDestroy))
        continue;
      for (const MachineOperand &MO : MI.operands()) {
        if (!MO.isFI() || MFI.isCalleeSavedObjectIndex(MO.getIndex()))
          continue;
        auto It = Offsets.find(MO.getIndex());
        unsigned Next = MO.getOperandNo() + 1;
        if (It == Offsets.end() || Next >= MI.getNumOperands() ||
            !MI.getOperand(Next).isImm())
          return true;
        int64_t Offset = It->second + Size + MI.getOperand(Next).getImm();
        int64_t OldOffset = Offset + Adjustments[&MI];
        int64_t NewOffset = Offset + Area;
        if (MI.getOpcode() == SH::LEA_FI)
          Cost += estimateSHFrameAddressBytes(NewOffset) -
                  static_cast<int64_t>(estimateSHFrameAddressBytes(OldOffset));
        else if (isSHDisplacementFrameAccess(MI))
          Cost += estimateSHFrameAccessBytes(MI, NewOffset,
                                             Live.available(SH::R0)) -
                  static_cast<int64_t>(estimateSHFrameAccessBytes(
                      MI, OldOffset, Live.available(SH::R0)));
        else
          return true;
      }
    }
  }
  // Keep every conservative special-frame return above intact. Only the
  // fully modeled, ordinary-frame profitability result is optional.
  return Cost <= 0;
}

void SHFrameLowering::processFunctionBeforeFrameFinalized(
    MachineFunction &MF, RegScavenger *RS) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  if (needsSHScavengingFrameIndex(MF)) {
    if (!RS)
      report_fatal_error("SH large stack frames require register scavenging");
    RS->addScavengingFrameIndex(MFI.CreateSpillStackObject(4, Align(4)));
  }
  if (!hasReservedCallFrame(MF))
    return;
  MF.getInfo<SHMachineFunctionInfo>()->setUseReservedCallFrame(
      isProfitableSHReservedCallFrame(MF, RS));
  if (!hasReservedCallFrame(MF))
    return;
  for (MachineBasicBlock &MBB : MF) {
    for (MachineInstr &MI : llvm::make_early_inc_range(MBB))
      if (MI.getOpcode() == SH::ADJCALLSTACKDOWN ||
          MI.getOpcode() == SH::ADJCALLSTACKUP)
        eliminateCallFramePseudoInstr(MF, MBB, MI.getIterator());
    MBB.setCallFrameSize(0);
  }
}

bool SHFrameLowering::assignCalleeSavedSpillSlots(
    MachineFunction &MF, const TargetRegisterInfo *TRI,
    std::vector<CalleeSavedInfo> &CSI) const {
  SHMachineFunctionInfo *FuncInfo = MF.getInfo<SHMachineFunctionInfo>();
  bool SavePR = needsSHPRSave(MF);
  bool SaveFPR = FuncInfo->isHandler() &&
                 llvm::any_of(CSI, [](const CalleeSavedInfo &Info) {
                   return SH::FRRegClass.contains(Info.getReg());
                 });
  if (!SavePR && !FuncInfo->savesMACH() && !FuncInfo->savesMACL() &&
      !FuncInfo->savesFPUL() && !FuncInfo->savesFPSCR() && !SaveFPR)
    return false;

  if (SavePR && !FuncInfo->hasReturnAddressFrameIndex())
    FuncInfo->setReturnAddressFrameIndex(createSHPRSpillFrameIndex(MF));
  if (FuncInfo->savesMACH())
    FuncInfo->setMACHFrameIndex(createSHHandlerStateSpillFrameIndex(MF));
  if (FuncInfo->savesMACL())
    FuncInfo->setMACLFrameIndex(createSHHandlerStateSpillFrameIndex(MF));
  if (FuncInfo->savesFPSCR())
    FuncInfo->setFPSCRFrameIndex(createSHHandlerStateSpillFrameIndex(MF));
  if (FuncInfo->savesFPUL())
    FuncInfo->setFPULFrameIndex(createSHHandlerStateSpillFrameIndex(MF));
  for (auto I = CSI.begin(); I != CSI.end();) {
    if (FuncInfo->isHandler() && SH::FRRegClass.contains(I->getReg())) {
      FuncInfo->addFPRSpill(I->getReg(),
                            createSHHandlerStateSpillFrameIndex(MF));
      I = CSI.erase(I);
      continue;
    }
    if (I->getReg() == SH::PR) {
      I = CSI.erase(I);
      SavePR = false;
      continue;
    }
    ++I;
  }
  if (SavePR)
    report_fatal_error("SH non-leaf function did not reserve PR");
  return false;
}

MachineBasicBlock::iterator SHFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  if (I->getOpcode() != SH::ADJCALLSTACKDOWN &&
      I->getOpcode() != SH::ADJCALLSTACKUP)
    report_fatal_error("SH encountered an unknown call-frame pseudo");

  int64_t Amount = I->getOperand(0).getImm();
  int64_t CalleePopAmount = I->getOperand(1).getImm();
  if (CalleePopAmount != 0)
    reportFatalUsageError("SH callee-popped call frames are not supported");
  if (Amount < 0 || static_cast<uint64_t>(Amount) > SHMaxCallFrameSize ||
      Amount % 4 != 0)
    report_fatal_error(
        Twine(
            "SH call-frame adjustment must be four-byte aligned and in [0, ") +
        Twine(SHMaxCallFrameSize) + "]");

  if (Amount != 0 && !hasReservedCallFrame(MF)) {
    int64_t Immediate =
        I->getOpcode() == SH::ADJCALLSTACKDOWN ? -Amount : Amount;
    emitSHCallFrameAdjustment(MF, MBB, I, Immediate, !hasFP(MF), I->getFlags());
  }
  return MBB.erase(I);
}

void SHFrameLowering::resetCFIToInitialState(MachineBasicBlock &MBB) const {
  MachineFunction &MF = *MBB.getParent();
  CFIInstBuilder CFIBuilder(MBB, MBB.begin(), MachineInstr::NoFlags);
  CFIBuilder.buildDefCFA(SH::R15, 0);
  if (needsSHPRSave(MF))
    CFIBuilder.buildSameValue(SH::PR);
  if (MF.getInfo<SHMachineFunctionInfo>()->savesMACH())
    CFIBuilder.buildSameValue(SH::MACH);
  if (MF.getInfo<SHMachineFunctionInfo>()->savesMACL())
    CFIBuilder.buildSameValue(SH::MACL);
  if (MF.getInfo<SHMachineFunctionInfo>()->savesFPUL())
    CFIBuilder.buildSameValue(SH::FPUL);
  if (MF.getInfo<SHMachineFunctionInfo>()->savesFPSCR())
    CFIBuilder.buildSameValue(SH::FPSCR);
  for (const auto &[Reg, FrameIndex] :
       MF.getInfo<SHMachineFunctionInfo>()->getFPRSpills()) {
    (void)FrameIndex;
    CFIBuilder.buildSameValue(Reg);
  }
  for (const CalleeSavedInfo &Info : MF.getFrameInfo().getCalleeSavedInfo())
    CFIBuilder.buildSameValue(Info.getReg());
}

bool SHFrameLowering::enableShrinkWrapping(const MachineFunction &MF) const {
  return !MF.getInfo<SHMachineFunctionInfo>()->isHandler() &&
         TargetFrameLowering::enableShrinkWrapping(MF);
}

bool SHFrameLowering::hasFPImpl(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  // GNU's unwinder enters a landing pad with the call-site SP, which can
  // still include outgoing stack arguments. A stable frame base keeps
  // local objects addressable until the normal epilogue restores SP.
  return MFI.hasVarSizedObjects() || MFI.isFrameAddressTaken() ||
         !MF.getLandingPads().empty() || MFI.shouldRealignStack() ||
         MF.disableFramePointerElim();
}
