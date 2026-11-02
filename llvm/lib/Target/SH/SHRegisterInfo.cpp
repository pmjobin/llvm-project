//===-- SHRegisterInfo.cpp - SH register information ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHRegisterInfo.h"
#include "MCTargetDesc/SHMCTargetDesc.h"
#include "SH.h"
#include "SHFrameLowering.h"
#include "SHInstrInfo.h"
#include "SHMachineFunctionInfo.h"
#include "SHSubtarget.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/CFIInstBuilder.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"
#include <algorithm>

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "SHGenRegisterInfo.inc"

SHRegisterInfo::SHRegisterInfo() : SHGenRegisterInfo(SH::PR) {}

static void emitSHFrameAddress(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator Insert,
                               const DebugLoc &DL, const SHInstrInfo &TII,
                               Register Address, Register Base,
                               int64_t ByteOffset) {
  if (ByteOffset % 4 == 0 && isInt<8>(ByteOffset / 4) &&
      (ByteOffset < -127 || ByteOffset > 254)) {
    BuildMI(MBB, Insert, DL, TII.get(SH::MOVri), Address)
        .addImm(ByteOffset / 4);
    BuildMI(MBB, Insert, DL, TII.get(SH::SHLL2), Address).addReg(Address);
    BuildMI(MBB, Insert, DL, TII.get(SH::ADDrr), Address)
        .addReg(Address)
        .addReg(Base);
    return;
  }
  if (ByteOffset < 0 || ByteOffset > 254) {
    uint64_t Magnitude = ByteOffset < 0 ? static_cast<uint64_t>(-ByteOffset)
                                        : static_cast<uint64_t>(ByteOffset);
    TII.materializeI32(MBB, Insert, DL, Address,
                       static_cast<uint32_t>(Magnitude));
    if (ByteOffset < 0)
      BuildMI(MBB, Insert, DL, TII.get(SH::NEG), Address).addReg(Address);
    BuildMI(MBB, Insert, DL, TII.get(SH::ADDrr), Address)
        .addReg(Address)
        .addReg(Base);
    return;
  }
  BuildMI(MBB, Insert, DL, TII.get(SH::MOVrr), Address).addReg(Base);
  while (ByteOffset != 0) {
    int64_t Chunk = ByteOffset > 127 ? 127 : ByteOffset;
    BuildMI(MBB, Insert, DL, TII.get(SH::ADDri), Address)
        .addReg(Address)
        .addImm(Chunk);
    ByteOffset -= Chunk;
  }
}

static void
emitSHTemporaryFrameAdjustment(MachineFunction &MF, MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator Insert,
                               const DebugLoc &DL, const SHInstrInfo &TII,
                               Register Base, int64_t Amount, bool AdjustCFA) {
  while (Amount != 0) {
    int64_t Chunk = std::clamp<int64_t>(Amount, -128, 127);
    BuildMI(MBB, Insert, DL, TII.get(SH::ADDri), Base)
        .addReg(Base)
        .addImm(Chunk);
    Amount -= Chunk;
    if (AdjustCFA && MF.needsFrameMoves())
      CFIInstBuilder(MBB, Insert, MachineInstr::NoFlags)
          .buildAdjustCFAOffset(-Chunk);
  }
}

static bool isSHCalleeSavedFrameIndex(const MachineFrameInfo &MFI,
                                      int FrameIndex) {
  return llvm::any_of(MFI.getCalleeSavedInfo(),
                      [FrameIndex](const CalleeSavedInfo &Info) {
                        return Info.getFrameIdx() == FrameIndex;
                      });
}

static Register getSHFrameReferenceBase(const MachineFunction &MF,
                                        int FrameIndex, int SPAdj,
                                        int64_t &BaseOffset) {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const SHFrameLowering *TFI =
      MF.getSubtarget<SHSubtarget>().getFrameLowering();
  uint64_t StackSize = MFI.getStackSize();
  if (isSHCalleeSavedFrameIndex(MFI, FrameIndex)) {
    BaseOffset = static_cast<int64_t>(
        MFI.shouldRealignStack()
            ? MF.getInfo<SHMachineFunctionInfo>()->getRealignmentFirstAdjust()
            : StackSize);
    return SH::R15;
  }
  if (MFI.shouldRealignStack()) {
    const SHMachineFunctionInfo *FuncInfo = MF.getInfo<SHMachineFunctionInfo>();
    bool IsReturnAddress = FuncInfo->hasReturnAddressFrameIndex() &&
                           FuncInfo->getReturnAddressFrameIndex() == FrameIndex;
    if (MFI.isFixedObjectIndex(FrameIndex) || IsReturnAddress) {
      BaseOffset = static_cast<int64_t>(FuncInfo->getRealignmentFirstAdjust());
      return SH::R14;
    }
    BaseOffset = static_cast<int64_t>(StackSize);
    return SH::R13;
  }
  BaseOffset = static_cast<int64_t>(StackSize);
  if (TFI->hasFP(MF))
    return SH::R14;
  BaseOffset += static_cast<int64_t>(SPAdj);
  return SH::R15;
}

struct SHFPUFrameAccess {
  MachineInstr *Instr;
  bool IsLoad;
  bool IsDouble;
  Register Value;
  Register Base;
  int64_t Offset;
  unsigned NumWords;
  const MachineMemOperand *MMO;
};

static bool getSHFPUFrameAccess(MachineInstr &Instr, int SPAdj,
                                RegScavenger &RS, SHFPUFrameAccess &Access) {
  unsigned Opcode = Instr.getOpcode();
  bool IsLoad = Opcode == SH::FMOV_S_LOAD_FI || Opcode == SH::FMOV_D_LOAD_FI;
  bool IsStore = Opcode == SH::FMOV_S_STORE_FI || Opcode == SH::FMOV_D_STORE_FI;
  if ((!IsLoad && !IsStore) || Instr.getNumOperands() < 3 ||
      !Instr.getOperand(0).isReg() || !Instr.getOperand(1).isFI() ||
      !Instr.getOperand(2).isImm() || Instr.memoperands().size() != 1)
    return false;
  MachineFunction &MF = *Instr.getParent()->getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  int FrameIndex = Instr.getOperand(1).getIndex();
  if (RS.isScavengingFrameIndex(FrameIndex))
    return false;
  int64_t BaseOffset;
  Register Base = getSHFrameReferenceBase(MF, FrameIndex, SPAdj, BaseOffset);
  int64_t Offset = MFI.getObjectOffset(FrameIndex) + BaseOffset +
                   Instr.getOperand(2).getImm();
  if (Offset < SHMinFrameAddressOffset || Offset > SHMaxFrameAddressOffset ||
      Offset % 4 != 0)
    return false;
  bool IsDouble = Opcode == SH::FMOV_D_LOAD_FI || Opcode == SH::FMOV_D_STORE_FI;
  Access = {&Instr, IsLoad, IsDouble,           Instr.getOperand(0).getReg(),
            Base,   Offset, IsDouble ? 2U : 1U, Instr.memoperands().front()};
  return true;
}

static bool areSHFPUFrameAccessesContiguous(const SHFPUFrameAccess &First,
                                            const SHFPUFrameAccess &Second) {
  if (First.IsLoad)
    return Second.Offset == First.Offset + 4 * First.NumWords;
  return Second.Offset + 4 * Second.NumWords == First.Offset;
}

static bool emitSHAdjacentFPUFrameAccesses(MachineBasicBlock::iterator MI,
                                           int SPAdj, RegScavenger *RS,
                                           const SHInstrInfo &TII) {
  MachineFunction &MF = *MI->getParent()->getParent();
  if (!RS || MF.getTarget().getOptLevel() == CodeGenOptLevel::None ||
      MF.getFunction().hasOptNone())
    return false;
  SmallVector<SHFPUFrameAccess, 8> Accesses;
  for (auto I = MI;;) {
    SHFPUFrameAccess Access;
    if (!getSHFPUFrameAccess(*I, SPAdj, *RS, Access) ||
        (!Accesses.empty() && (Access.Base != Accesses.back().Base ||
                               Access.IsLoad != Accesses.back().IsLoad)))
      break;
    Accesses.push_back(Access);
    if (I == I->getParent()->begin())
      break;
    --I;
  }
  if (Accesses.empty())
    return false;
  unsigned CurrentIndex = Accesses.size() - 1;
  std::reverse(Accesses.begin(), Accesses.end());
  MachineBasicBlock &MBB = *MI->getParent();
  auto I = MI;
  ++I;
  for (; I != MBB.end(); ++I) {
    SHFPUFrameAccess Access;
    if (!getSHFPUFrameAccess(*I, SPAdj, *RS, Access) ||
        Access.Base != Accesses.back().Base ||
        Access.IsLoad != Accesses.back().IsLoad)
      break;
    Accesses.push_back(Access);
  }
  unsigned Start = CurrentIndex;
  while (Start != 0 &&
         areSHFPUFrameAccessesContiguous(Accesses[Start - 1], Accesses[Start]))
    --Start;
  unsigned End = CurrentIndex + 1;
  while (End != Accesses.size() &&
         areSHFPUFrameAccessesContiguous(Accesses[End - 1], Accesses[End]))
    ++End;
  if (End - Start < 2)
    return false;
  ArrayRef<SHFPUFrameAccess> Group =
      ArrayRef<SHFPUFrameAccess>(Accesses).slice(Start, End - Start);

  RS->setRegUsed(Group.front().Base);
  Register Address = RS->FindUnusedReg(&SH::GPRRegClass);
  if (!Address)
    return false;
  RS->setRegUsed(Address);
  auto Insert = Group.front().Instr->getIterator();
  int64_t InitialOffset =
      Group.front().Offset +
      (Group.front().IsLoad ? 0 : 4 * Group.front().NumWords);
  emitSHFrameAddress(MBB, Insert, Group.front().Instr->getDebugLoc(), TII,
                     Address, Group.front().Base, InitialOffset);
  bool IsBigEndian = MF.getDataLayout().isBigEndian();
  for (unsigned AccessIndex = 0; AccessIndex != Group.size(); ++AccessIndex) {
    const SHFPUFrameAccess &Access = Group[AccessIndex];
    auto GetFPUWord = [&](unsigned MemoryWord) {
      if (!Access.IsDouble)
        return Access.Value;
      unsigned PhysicalWord = IsBigEndian ? MemoryWord : 1 - MemoryWord;
      return Register(TII.getRegisterInfo().getSubReg(
          Access.Value, PhysicalWord == 0 ? SH::sub_fpu32_0 : SH::sub_fpu32_1));
    };
    auto GetWordMMO = [&](unsigned Word) {
      return MF.getMachineMemOperand(Access.MMO, 4 * Word, 4);
    };
    if (Access.IsLoad) {
      for (unsigned Word = 0; Word != Access.NumWords; ++Word) {
        bool IsLast =
            AccessIndex + 1 == Group.size() && Word + 1 == Access.NumWords;
        MachineInstrBuilder Load =
            BuildMI(MBB, Insert, Access.Instr->getDebugLoc(),
                    TII.get(SH::FMOV_S_LOAD_POSTINC_ALIAS), GetFPUWord(Word))
                .addDef(Address)
                .addReg(Address, RegState::Kill)
                .addMemOperand(GetWordMMO(Word))
                .setMIFlags(Access.Instr->getFlags());
        if (IsLast)
          Load->getOperand(1).setIsDead();
      }
    } else {
      for (unsigned WordIndex = 0; WordIndex != Access.NumWords; ++WordIndex) {
        unsigned Word = Access.NumWords - WordIndex - 1;
        bool IsLast =
            AccessIndex + 1 == Group.size() && WordIndex + 1 == Access.NumWords;
        MachineInstrBuilder Store =
            BuildMI(MBB, Insert, Access.Instr->getDebugLoc(),
                    TII.get(SH::FMOV_S_STORE_PREDEC_ALIAS), Address)
                .addReg(
                    GetFPUWord(Word),
                    getKillRegState(Access.Instr->getOperand(0).isKill() &&
                                    WordIndex + 1 == Access.NumWords) |
                        getUndefRegState(Access.Instr->getOperand(0).isUndef()))
                .addReg(Address, RegState::Kill)
                .addMemOperand(GetWordMMO(Word))
                .setMIFlags(Access.Instr->getFlags());
        if (IsLast)
          Store->getOperand(0).setIsDead();
      }
    }
  }
  for (const SHFPUFrameAccess &Access : Group)
    Access.Instr->eraseFromParent();
  return true;
}

struct SHFrameStore {
  MachineInstr *Instr;
  Register Source;
  Register Base;
  int64_t Offset;
  unsigned RegisterOpcode;
};

struct SHFrameLoad {
  MachineInstr *Instr;
  Register Destination;
  Register Base;
  int64_t Offset;
  unsigned Size;
  unsigned RegisterOpcode;
  unsigned PostIncrementOpcode;
};

struct SHFrameAccess {
  MachineInstr *Instr;
  bool IsLoad;
  Register Value;
  Register Base;
  int64_t Offset;
  unsigned Size;
  unsigned RegisterOpcode;
  unsigned PostIncrementOpcode;
};

static unsigned getSHImmediateAdjustmentCost(int64_t Amount) {
  uint64_t Magnitude = Amount < 0 ? static_cast<uint64_t>(-Amount)
                                  : static_cast<uint64_t>(Amount);
  uint64_t Step = Amount < 0 ? 128 : 127;
  return static_cast<unsigned>((Magnitude + Step - 1) / Step);
}

static bool canReuseSHFrameAddress(const MachineFunction &MF, int64_t Offset) {
  return Offset <= 384 ||
         (MF.getFunction().hasOptSize() && !MF.getFunction().hasMinSize());
}

static unsigned getSHIndependentFrameAddressCost(int64_t Offset) {
  if (Offset <= 384)
    return 1 + getSHImmediateAdjustmentCost(Offset);
  if (Offset % 4 == 0 && isInt<8>(Offset / 4))
    return 3;
  // Far -Os addresses use one literal load and one register add.
  return 2;
}

static bool getSHFrameLoad(MachineInstr &Instr, int SPAdj, RegScavenger *RS,
                           SHFrameLoad &Load) {
  unsigned Opcode = Instr.getOpcode();
  bool IsLong = Opcode == SH::MOVL_load_disp;
  bool IsByte = Opcode == SH::MOVB_load_frame;
  bool IsWord = Opcode == SH::MOVW_load_frame;
  if ((!IsLong && !IsByte && !IsWord) || Instr.getNumOperands() < 3 ||
      !Instr.getOperand(0).isReg() || !Instr.getOperand(1).isFI() ||
      !Instr.getOperand(2).isImm())
    return false;

  MachineFunction &MF = *Instr.getParent()->getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  int FrameIndex = Instr.getOperand(1).getIndex();
  if (RS && RS->isScavengingFrameIndex(FrameIndex))
    return false;
  int64_t BaseOffset;
  Register Base = getSHFrameReferenceBase(MF, FrameIndex, SPAdj, BaseOffset);
  if (Base != SH::R14 && Base != SH::R15)
    return false;
  int64_t Offset = MFI.getObjectOffset(FrameIndex) + BaseOffset +
                   Instr.getOperand(2).getImm();
  bool NeedsAddress = (IsLong && Offset > 60) || (IsByte && Offset > 15) ||
                      (IsWord && Offset > 30);
  if (!NeedsAddress || !canReuseSHFrameAddress(MF, Offset) ||
      (IsLong && Offset % 4 != 0) || (IsWord && Offset % 2 != 0))
    return false;

  unsigned Size = IsLong ? 4 : IsByte ? 1 : 2;
  unsigned RegisterOpcode = IsLong   ? SH::MOVL_load_reg
                            : IsByte ? SH::MOVB_load_reg
                                     : SH::MOVW_load_reg;
  unsigned PostIncrementOpcode = IsLong   ? SH::MOVL_load_postinc
                                 : IsByte ? SH::MOVB_load_postinc
                                          : SH::MOVW_load_postinc;
  Load = {&Instr,         Instr.getOperand(0).getReg(), Base, Offset, Size,
          RegisterOpcode, PostIncrementOpcode};
  return true;
}

static bool getSHFrameStore(MachineInstr &Instr, int SPAdj, RegScavenger &RS,
                            SHFrameStore &Store) {
  unsigned Opcode = Instr.getOpcode();
  bool IsLong = Opcode == SH::MOVL_store_disp;
  bool IsByte = Opcode == SH::MOVB_store_frame;
  bool IsWord = Opcode == SH::MOVW_store_frame;
  if ((!IsLong && !IsByte && !IsWord) || Instr.getNumOperands() < 3 ||
      !Instr.getOperand(0).isReg() || !Instr.getOperand(1).isFI() ||
      !Instr.getOperand(2).isImm())
    return false;

  MachineFunction &MF = *Instr.getParent()->getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  int FrameIndex = Instr.getOperand(1).getIndex();
  if (RS.isScavengingFrameIndex(FrameIndex))
    return false;
  int64_t BaseOffset;
  Register Base = getSHFrameReferenceBase(MF, FrameIndex, SPAdj, BaseOffset);
  if (Base != SH::R14 && Base != SH::R15)
    return false;
  int64_t Offset = MFI.getObjectOffset(FrameIndex) + BaseOffset +
                   Instr.getOperand(2).getImm();
  bool NeedsAddress = (IsLong && Offset > 60) || (IsByte && Offset > 15) ||
                      (IsWord && Offset > 30);
  if (!NeedsAddress || !canReuseSHFrameAddress(MF, Offset) ||
      (IsLong && Offset % 4 != 0) || (IsWord && Offset % 2 != 0))
    return false;

  unsigned RegisterOpcode = IsLong   ? SH::MOVL_store_reg
                            : IsByte ? SH::MOVB_store_reg
                                     : SH::MOVW_store_reg;
  Store = {&Instr, Instr.getOperand(0).getReg(), Base, Offset, RegisterOpcode};
  return true;
}

static bool getSHFrameAccess(MachineInstr &Instr, int SPAdj, RegScavenger &RS,
                             SHFrameAccess &Access) {
  SHFrameLoad Load;
  if (getSHFrameLoad(Instr, SPAdj, &RS, Load)) {
    Access = {
        Load.Instr,  true,      Load.Destination,    Load.Base,
        Load.Offset, Load.Size, Load.RegisterOpcode, Load.PostIncrementOpcode};
    return true;
  }
  SHFrameStore Store;
  if (!getSHFrameStore(Instr, SPAdj, RS, Store))
    return false;
  unsigned Size = Store.RegisterOpcode == SH::MOVL_store_reg   ? 4
                  : Store.RegisterOpcode == SH::MOVB_store_reg ? 1
                                                               : 2;
  Access = {Store.Instr,  false, Store.Source,         Store.Base,
            Store.Offset, Size,  Store.RegisterOpcode, 0};
  return true;
}

static bool emitSHAdjacentMixedFrameAccesses(MachineBasicBlock::iterator MI,
                                             int SPAdj, RegScavenger *RS,
                                             const SHInstrInfo &TII) {
  MachineFunction &MF = *MI->getParent()->getParent();
  if (!RS || MF.getTarget().getOptLevel() == CodeGenOptLevel::None ||
      MF.getFunction().hasOptNone())
    return false;

  SmallVector<SHFrameAccess, 8> Accesses;
  for (auto I = MI;;) {
    SHFrameAccess Access;
    if (!getSHFrameAccess(*I, SPAdj, *RS, Access) ||
        (!Accesses.empty() && Access.Base != Accesses.back().Base))
      break;
    Accesses.push_back(Access);
    if (I == I->getParent()->begin())
      break;
    --I;
  }
  std::reverse(Accesses.begin(), Accesses.end());
  MachineBasicBlock &MBB = *MI->getParent();
  auto I = MI;
  ++I;
  for (; I != MBB.end(); ++I) {
    SHFrameAccess Access;
    if (!getSHFrameAccess(*I, SPAdj, *RS, Access) ||
        (!Accesses.empty() && Access.Base != Accesses.back().Base))
      break;
    Accesses.push_back(Access);
  }
  if (Accesses.size() < 2 ||
      !llvm::any_of(
          Accesses,
          [](const SHFrameAccess &Access) { return Access.IsLoad; }) ||
      !llvm::any_of(Accesses,
                    [](const SHFrameAccess &Access) { return !Access.IsLoad; }))
    return false;

  unsigned IndependentCost = 0;
  for (const SHFrameAccess &Access : Accesses)
    IndependentCost += getSHIndependentFrameAddressCost(Access.Offset);
  unsigned ReusedCost =
      getSHIndependentFrameAddressCost(Accesses.front().Offset);
  for (unsigned Index = 0; Index + 1 != Accesses.size(); ++Index) {
    int64_t CurrentOffset = Accesses[Index].Offset;
    if (Accesses[Index].IsLoad &&
        Accesses[Index + 1].Offset - CurrentOffset == Accesses[Index].Size)
      CurrentOffset += Accesses[Index].Size;
    ReusedCost += getSHImmediateAdjustmentCost(Accesses[Index + 1].Offset -
                                               CurrentOffset);
  }
  if (IndependentCost <= ReusedCost)
    return false;

  Register Address;
  if (Accesses.back().IsLoad) {
    Register Candidate = Accesses.back().Value;
    bool Conflicts = llvm::any_of(ArrayRef<SHFrameAccess>(Accesses).drop_back(),
                                  [Candidate](const SHFrameAccess &Access) {
                                    return Access.Value == Candidate;
                                  });
    if (!Conflicts)
      Address = Candidate;
  }
  if (!Address) {
    BitVector Available = RS->getRegsAvailable(&SH::GPRRegClass);
    for (Register Candidate : SH::GPRRegClass) {
      if (!Available.test(Candidate.id()) ||
          llvm::any_of(Accesses, [Candidate](const SHFrameAccess &Access) {
            return Access.Value == Candidate;
          }))
        continue;
      Address = Candidate;
      break;
    }
  }
  if (!Address)
    return false;

  auto Insert = Accesses.front().Instr->getIterator();
  emitSHFrameAddress(MBB, Insert, Accesses.front().Instr->getDebugLoc(), TII,
                     Address, Accesses.front().Base, Accesses.front().Offset);
  for (unsigned Index = 0; Index != Accesses.size(); ++Index) {
    const SHFrameAccess &Access = Accesses[Index];
    bool UsePostIncrement =
        Access.IsLoad && Index + 1 != Accesses.size() &&
        Accesses[Index + 1].Offset - Access.Offset == Access.Size;
    if (Access.IsLoad) {
      MachineInstrBuilder Load;
      if (UsePostIncrement) {
        Load = BuildMI(MBB, Insert, Access.Instr->getDebugLoc(),
                       TII.get(Access.PostIncrementOpcode), Access.Value)
                   .addDef(Address)
                   .addReg(Address, RegState::Kill);
      } else {
        Load =
            BuildMI(MBB, Insert, Access.Instr->getDebugLoc(),
                    TII.get(Access.RegisterOpcode), Access.Value)
                .addReg(Address, getKillRegState(Index + 1 == Accesses.size()));
      }
      Load->getOperand(0).setIsDead(Access.Instr->getOperand(0).isDead());
      Load.setMIFlags(Access.Instr->getFlags()).cloneMemRefs(*Access.Instr);
    } else {
      BuildMI(MBB, Insert, Access.Instr->getDebugLoc(),
              TII.get(Access.RegisterOpcode))
          .add(Access.Instr->getOperand(0))
          .addReg(Address, getKillRegState(Index + 1 == Accesses.size()))
          .setMIFlags(Access.Instr->getFlags())
          .cloneMemRefs(*Access.Instr);
    }
    if (Index + 1 != Accesses.size()) {
      int64_t CurrentOffset =
          Access.Offset + (UsePostIncrement ? Access.Size : 0);
      emitSHTemporaryFrameAdjustment(
          MF, MBB, Insert, Accesses[Index + 1].Instr->getDebugLoc(), TII,
          Address, Accesses[Index + 1].Offset - CurrentOffset,
          /*AdjustCFA=*/false);
    }
  }
  for (const SHFrameAccess &Access : Accesses)
    Access.Instr->eraseFromParent();
  return true;
}

static bool emitSHAdjacentFrameStores(MachineBasicBlock::iterator MI, int SPAdj,
                                      RegScavenger *RS,
                                      const SHInstrInfo &TII) {
  MachineFunction &MF = *MI->getParent()->getParent();
  if (!RS || MF.getTarget().getOptLevel() == CodeGenOptLevel::None ||
      MF.getFunction().hasOptNone())
    return false;

  SmallVector<SHFrameStore, 8> ReverseStores;
  for (auto I = MI;;) {
    SHFrameStore Store;
    if (!getSHFrameStore(*I, SPAdj, *RS, Store) ||
        (!ReverseStores.empty() && Store.Base != ReverseStores.back().Base))
      break;
    ReverseStores.push_back(Store);
    if (I == I->getParent()->begin())
      break;
    --I;
  }
  if (ReverseStores.size() < 2)
    return false;
  std::reverse(ReverseStores.begin(), ReverseStores.end());

  unsigned BestStart = ReverseStores.size();
  unsigned BestSaving = 0;
  for (unsigned Start = 0, End = ReverseStores.size(); Start + 1 < End;
       ++Start) {
    unsigned IndependentCost = 0;
    for (unsigned I = Start; I != End; ++I)
      IndependentCost +=
          getSHIndependentFrameAddressCost(ReverseStores[I].Offset);
    unsigned ReusedCost =
        getSHIndependentFrameAddressCost(ReverseStores[Start].Offset);
    for (unsigned I = Start + 1; I != End; ++I)
      ReusedCost += getSHImmediateAdjustmentCost(ReverseStores[I].Offset -
                                                 ReverseStores[I - 1].Offset);
    if (IndependentCost > ReusedCost &&
        IndependentCost - ReusedCost > BestSaving) {
      BestStart = Start;
      BestSaving = IndependentCost - ReusedCost;
    }
  }
  if (BestStart == ReverseStores.size())
    return false;
  ArrayRef<SHFrameStore> Stores =
      ArrayRef<SHFrameStore>(ReverseStores).drop_front(BestStart);
  BitVector Available = RS->getRegsAvailable(&SH::GPRRegClass);
  Register Address;
  for (Register Candidate : SH::GPRRegClass) {
    if (!Available.test(Candidate.id()) ||
        llvm::any_of(Stores, [&](const SHFrameStore &Store) {
          return Store.Source == Candidate;
        }))
      continue;
    Address = Candidate;
    break;
  }
  if (!Address)
    return false;

  MachineBasicBlock &MBB = *MI->getParent();
  auto Insert = Stores.front().Instr->getIterator();
  emitSHFrameAddress(MBB, Insert, Stores.front().Instr->getDebugLoc(), TII,
                     Address, Stores.front().Base, Stores.front().Offset);
  for (unsigned I = 0; I != Stores.size(); ++I) {
    if (I != 0)
      emitSHTemporaryFrameAdjustment(
          MF, MBB, Insert, Stores[I].Instr->getDebugLoc(), TII, Address,
          Stores[I].Offset - Stores[I - 1].Offset,
          /*AdjustCFA=*/false);
    BuildMI(MBB, Insert, Stores[I].Instr->getDebugLoc(),
            TII.get(Stores[I].RegisterOpcode))
        .add(Stores[I].Instr->getOperand(0))
        .addReg(Address, getKillRegState(I + 1 == Stores.size()))
        .setMIFlags(Stores[I].Instr->getFlags())
        .cloneMemRefs(*Stores[I].Instr);
  }
  for (const SHFrameStore &Store : Stores)
    Store.Instr->eraseFromParent();
  return true;
}

static bool emitSHAdjacentFrameLoads(MachineBasicBlock::iterator MI, int SPAdj,
                                     RegScavenger *RS, const SHInstrInfo &TII) {
  MachineFunction &MF = *MI->getParent()->getParent();
  if (MF.getTarget().getOptLevel() == CodeGenOptLevel::None ||
      MF.getFunction().hasOptNone())
    return false;

  SmallVector<SHFrameLoad, 8> Loads;
  for (auto I = MI;;) {
    SHFrameLoad Load;
    if (!getSHFrameLoad(*I, SPAdj, RS, Load) ||
        (!Loads.empty() && Load.Base != Loads.back().Base))
      break;
    Loads.push_back(Load);
    if (I == I->getParent()->begin())
      break;
    --I;
  }
  unsigned CurrentIndex = Loads.size() - 1;
  std::reverse(Loads.begin(), Loads.end());
  MachineBasicBlock &MBB = *MI->getParent();
  auto I = MI;
  ++I;
  for (; I != MBB.end(); ++I) {
    SHFrameLoad Load;
    if (!getSHFrameLoad(*I, SPAdj, RS, Load) ||
        (!Loads.empty() && Load.Base != Loads.back().Base))
      break;
    Loads.push_back(Load);
  }
  if (Loads.size() < 2)
    return false;

  Register Address = Loads.back().Destination;
  unsigned BestStart = Loads.size();
  unsigned BestSaving = 0;
  for (unsigned Start = 0, End = Loads.size(); Start + 1 < End; ++Start) {
    if (Start > CurrentIndex)
      continue;
    if (llvm::any_of(ArrayRef<SHFrameLoad>(Loads).slice(Start, End - Start - 1),
                     [Address](const SHFrameLoad &Load) {
                       return Load.Destination == Address;
                     }))
      continue;
    unsigned IndependentCost = 0;
    for (unsigned Index = Start; Index != End; ++Index)
      IndependentCost += getSHIndependentFrameAddressCost(Loads[Index].Offset);
    unsigned ReusedCost = getSHIndependentFrameAddressCost(Loads[Start].Offset);
    for (unsigned Index = Start; Index + 1 != End; ++Index) {
      int64_t Difference = Loads[Index + 1].Offset - Loads[Index].Offset;
      if (Difference != Loads[Index].Size)
        ReusedCost += getSHImmediateAdjustmentCost(Difference);
    }
    if (IndependentCost > ReusedCost &&
        IndependentCost - ReusedCost > BestSaving) {
      BestStart = Start;
      BestSaving = IndependentCost - ReusedCost;
    }
  }
  if (BestStart == Loads.size())
    return false;
  ArrayRef<SHFrameLoad> ReusedLoads =
      ArrayRef<SHFrameLoad>(Loads).drop_front(BestStart);

  auto Insert = ReusedLoads.front().Instr->getIterator();
  emitSHFrameAddress(MBB, Insert, ReusedLoads.front().Instr->getDebugLoc(), TII,
                     Address, ReusedLoads.front().Base,
                     ReusedLoads.front().Offset);
  for (unsigned Index = 0; Index != ReusedLoads.size(); ++Index) {
    const SHFrameLoad &Load = ReusedLoads[Index];
    bool UsePostIncrement =
        Index + 1 != ReusedLoads.size() &&
        ReusedLoads[Index + 1].Offset - Load.Offset == Load.Size;
    MachineInstrBuilder NewLoad;
    if (UsePostIncrement) {
      NewLoad = BuildMI(MBB, Insert, Load.Instr->getDebugLoc(),
                        TII.get(Load.PostIncrementOpcode), Load.Destination)
                    .addDef(Address)
                    .addReg(Address, RegState::Kill);
    } else {
      NewLoad = BuildMI(MBB, Insert, Load.Instr->getDebugLoc(),
                        TII.get(Load.RegisterOpcode), Load.Destination)
                    .addReg(Address,
                            getKillRegState(Index + 1 == ReusedLoads.size()));
    }
    NewLoad->getOperand(0).setIsDead(Load.Instr->getOperand(0).isDead());
    NewLoad.setMIFlags(Load.Instr->getFlags()).cloneMemRefs(*Load.Instr);
    if (!UsePostIncrement && Index + 1 != ReusedLoads.size())
      emitSHTemporaryFrameAdjustment(
          MF, MBB, Insert, ReusedLoads[Index + 1].Instr->getDebugLoc(), TII,
          Address, ReusedLoads[Index + 1].Offset - Load.Offset,
          /*AdjustCFA=*/false);
  }
  for (const SHFrameLoad &Load : ReusedLoads)
    Load.Instr->eraseFromParent();
  return true;
}

const MCPhysReg *
SHRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  if (MF && MF->getInfo<SHMachineFunctionInfo>()->isInterruptHandler())
    return MF->getInfo<SHMachineFunctionInfo>()->usesResbank()
               ? CSR_SH_Interrupt_Resbank_Save_SaveList
           : MF->getInfo<SHMachineFunctionInfo>()->noSaveLowRegs()
               ? CSR_SH_Interrupt_NoLow_Save_SaveList
           : MF->getInfo<SHMachineFunctionInfo>()->hasStackSwitch()
               ? CSR_SH_Interrupt_StackSwitch_Save_SaveList
               : CSR_SH_Interrupt_Save_SaveList;
  if (MF && MF->getSubtarget<SHSubtarget>().isFDPIC()) {
    if (MF->callsEHReturn())
      return MF->getSubtarget<SHSubtarget>().useHardFloatABI()
                 ? CSR_SH_FDPIC_Hard_EHRet_Save_SaveList
                 : CSR_SH_FDPIC_EHRet_Save_SaveList;
    return MF->getSubtarget<SHSubtarget>().useHardFloatABI()
               ? CSR_SH_FDPIC_Hard_Save_SaveList
               : CSR_SH_FDPIC_Save_SaveList;
  }
  if (MF && MF->callsEHReturn())
    return MF->getSubtarget<SHSubtarget>().useHardFloatABI()
               ? CSR_SH_Hard_EHRet_Save_SaveList
               : CSR_SH_EHRet_Save_SaveList;
  if (MF && MF->getSubtarget<SHSubtarget>().useHardFloatABI())
    return CSR_SH_Hard_Save_SaveList;
  return CSR_SH_Save_SaveList;
}

const uint32_t *SHRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                                     CallingConv::ID CC) const {
  if (CC != CallingConv::C && CC != CallingConv::Fast)
    report_fatal_error("SH only supports the C and fast calling conventions");
  if (MF.getSubtarget<SHSubtarget>().isFDPIC())
    return MF.getSubtarget<SHSubtarget>().useHardFloatABI()
               ? CSR_SH_FDPIC_Hard_RegMask
               : CSR_SH_FDPIC_RegMask;
  return MF.getSubtarget<SHSubtarget>().useHardFloatABI() ? CSR_SH_Hard_RegMask
                                                          : CSR_SH_RegMask;
}

const uint32_t *
SHRegisterInfo::getShiftLibcallPreservedMask(bool IsArithmetic) const {
  return IsArithmetic ? CSR_SH_AShr_RegMask : CSR_SH_Shift_RegMask;
}

BitVector SHRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(SH::R15);
  const SHFrameLowering *TFI =
      MF.getSubtarget<SHSubtarget>().getFrameLowering();
  if (TFI->hasFP(MF))
    Reserved.set(SH::R14);
  if (MF.getFrameInfo().shouldRealignStack())
    Reserved.set(SH::R13);
  bool NeedsCallFrameScratch =
      MF.getInfo<SHMachineFunctionInfo>()->needsCallFrameScratch();
  if (!NeedsCallFrameScratch)
    for (const MachineBasicBlock &MBB : MF)
      for (const MachineInstr &MI : MBB)
        if ((MI.getOpcode() == SH::ADJCALLSTACKDOWN ||
             MI.getOpcode() == SH::ADJCALLSTACKUP) &&
            MI.getNumOperands() != 0 && MI.getOperand(0).isImm() &&
            MI.getOperand(0).getImm() > 256)
          NeedsCallFrameScratch = true;
  if (NeedsCallFrameScratch)
    Reserved.set(SH::R3);
  if (MF.getTarget().isPositionIndependent() ||
      MF.getInfo<SHMachineFunctionInfo>()->usesPICBase())
    Reserved.set(SH::R12);
  Reserved.set(SH::PC);
  Reserved.set(SH::PR);
  Reserved.set(SH::GBR);
  Reserved.set(SH::VBR);
  Reserved.set(SH::MACH);
  Reserved.set(SH::MACL);
  Reserved.set(SH::SR);
  Reserved.set(SH::SSR);
  Reserved.set(SH::SPC);
  Reserved.set(SH::SGR);
  Reserved.set(SH::DBR);
  Reserved.set(SH::FPUL);
  Reserved.set(SH::FPSCR);
  Reserved.set(SH::TBit);
  Reserved.set(SH::MBit);
  Reserved.set(SH::QBit);
  Reserved.set(SH::SBit);
  for (MCRegister Reg :
       {SH::A0L, SH::A1L, SH::A0G, SH::A1G, SH::X0, SH::X1, SH::Y0, SH::Y1,
        SH::M0, SH::M1, SH::DSR, SH::RS, SH::RE, SH::MOD})
    markSuperRegs(Reserved, Reg);
  for (MCRegister Reg : SH::BankedGPRRegClass)
    Reserved.set(Reg);
  for (MCRegister Reg : SH::XFRegClass)
    markSuperRegs(Reserved, Reg);
  // Allocate primary FR storage and its DR/FV aliases when the subtarget has
  // an FPU. The alternate bank and matrix remain reserved above; a matrix
  // region owns them explicitly rather than through general allocation.
  if (!MF.getSubtarget<SHSubtarget>().hasSingleFPU())
    for (const TargetRegisterClass *RC :
         {&SH::FRRegClass, &SH::XFRegClass, &SH::DRRegClass, &SH::XDRegClass,
          &SH::FVRegClass, &SH::XMTRXRegRegClass})
      for (MCRegister Reg : *RC)
        Reserved.set(Reg);
  return Reserved;
}

bool SHRegisterInfo::getRegAllocationHints(Register VirtReg,
                                           ArrayRef<MCPhysReg> Order,
                                           SmallSetVector<MCPhysReg, 16> &Hints,
                                           const MachineFunction &MF,
                                           const VirtRegMap *VRM,
                                           const LiveRegMatrix *Matrix) const {
  bool Base = TargetRegisterInfo::getRegAllocationHints(VirtReg, Order, Hints,
                                                        MF, VRM, Matrix);
  if (!MF.getFunction().hasOptSize())
    return Base;
  const MachineRegisterInfo &MRI = MF.getRegInfo();
  if (!MRI.hasOneDef(VirtReg))
    return Base;
  MachineInstr *Def = MRI.getVRegDef(VirtReg);
  if (!Def || Def->getOpcode() != SH::MOVL_load_pc_island)
    return Base;
  MachineOperand *Use = MRI.getOneNonDBGUse(VirtReg);
  if (!Use || Use->getParent()->getOpcode() != SH::JSR ||
      Use->getOperandNo() != 0)
    return Base;
  // Keeping a direct call target out of R0 lets an R0-based argument load
  // move into the call's delay slot.
  for (MCPhysReg Reg : {SH::R1, SH::R2, SH::R3})
    if (llvm::is_contained(Order, Reg))
      Hints.insert(Reg);
  return Base;
}

bool SHRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MI,
                                         int SPAdj, unsigned FIOperandNum,
                                         RegScavenger *RS) const {
  MachineInstr &Instr = *MI;
  unsigned Opcode = Instr.getOpcode();
  bool IsFPULoad = Opcode == SH::FMOV_S_LOAD_FI ||
                   Opcode == SH::FMOV_D_LOAD_FI || Opcode == SH::FMOV_V_LOAD_FI;
  bool IsFPUStore = Opcode == SH::FMOV_S_STORE_FI ||
                    Opcode == SH::FMOV_D_STORE_FI ||
                    Opcode == SH::FMOV_V_STORE_FI;
  if (IsFPULoad || IsFPUStore) {
    MachineFunction &MF = *Instr.getParent()->getParent();
    const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
    bool IsVector =
        Opcode == SH::FMOV_V_LOAD_FI || Opcode == SH::FMOV_V_STORE_FI;
    if (!IsVector && emitSHAdjacentFPUFrameAccesses(MI, SPAdj, RS, *TII))
      return true;
    if (FIOperandNum != 1 || FIOperandNum + 1 >= Instr.getNumOperands() ||
        !Instr.getOperand(FIOperandNum + 1).isImm())
      report_fatal_error("SH FPU frame reference is malformed");
    const MachineFrameInfo &MFI = MF.getFrameInfo();
    int FrameIndex = Instr.getOperand(FIOperandNum).getIndex();
    int64_t BaseOffset;
    Register FrameReg =
        getSHFrameReferenceBase(MF, FrameIndex, SPAdj, BaseOffset);
    int64_t ByteOffset = MFI.getObjectOffset(FrameIndex) + BaseOffset +
                         Instr.getOperand(FIOperandNum + 1).getImm();
    if (ByteOffset < SHMinFrameAddressOffset ||
        ByteOffset > SHMaxFrameAddressOffset || ByteOffset % 4 != 0)
      report_fatal_error(
          "SH FPU frame reference must be four-byte aligned and representable");
    if (!RS)
      report_fatal_error("SH FPU frame reference requires register scavenging");
    MachineBasicBlock &MBB = *Instr.getParent();
    RS->setRegUsed(FrameReg);
    bool IsDouble =
        Opcode == SH::FMOV_D_LOAD_FI || Opcode == SH::FMOV_D_STORE_FI;
    if (!IsDouble && !IsVector && ByteOffset > 0 &&
        Instr.memoperands().size() == 1 && !RS->isRegUsed(SH::R0)) {
      RS->setRegUsed(SH::R0);
      if (ByteOffset <= 127) {
        BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(SH::MOVri), SH::R0)
            .addImm(ByteOffset);
      } else if (isInt<8>(ByteOffset / 4)) {
        BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(SH::MOVri), SH::R0)
            .addImm(ByteOffset / 4);
        BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(SH::SHLL2), SH::R0)
            .addReg(SH::R0);
      } else
        TII->materializeI32(MBB, MI, Instr.getDebugLoc(), SH::R0,
                            static_cast<uint32_t>(ByteOffset));
      unsigned IndexedOpcode = IsFPULoad ? SH::FMOV_S_LOAD_INDEXED_ALIAS
                                         : SH::FMOV_S_STORE_INDEXED_ALIAS;
      BuildMI(MBB, MI, MIMetadata(Instr), TII->get(IndexedOpcode))
          .add(Instr.getOperand(0))
          .addReg(FrameReg)
          .addMemOperand(Instr.memoperands().front())
          .setMIFlags(Instr.getFlags());
      MBB.erase(MI);
      return false;
    }
    Register Address = RS->FindUnusedReg(&SH::GPRRegClass);
    if (!Address)
      Address = RS->scavengeRegisterBackwards(SH::GPRRegClass, MI,
                                              /*RestoreAfter=*/false, SPAdj);
    if (!Address)
      report_fatal_error("SH could not scavenge an FPU frame address register");
    RS->setRegUsed(Address);
    unsigned NumWords = IsVector ? 4 : IsDouble ? 2 : 1;
    Register FPUReg = Instr.getOperand(0).getReg();
    bool IsBigEndian = MF.getDataLayout().isBigEndian();
    if (Instr.memoperands().size() != 1)
      report_fatal_error("SH FPU frame reference requires one memory operand");
    const MachineMemOperand *BaseMMO = Instr.memoperands().front();
    auto GetFPUWord = [&](unsigned MemoryWord) {
      if (IsVector)
        return Register(getSubReg(FPUReg, SH::sub_fpu32_0 + MemoryWord));
      unsigned PhysicalWord = IsBigEndian ? MemoryWord : 1 - MemoryWord;
      return Register(getSubReg(FPUReg, PhysicalWord == 0 ? SH::sub_fpu32_0
                                                          : SH::sub_fpu32_1));
    };
    auto GetWordMMO = [&](unsigned Word) {
      return MF.getMachineMemOperand(BaseMMO, 4 * Word, 4);
    };
    if (IsFPULoad) {
      emitSHFrameAddress(MBB, MI, Instr.getDebugLoc(), *TII, Address, FrameReg,
                         ByteOffset);
      for (unsigned Word = 0; Word != NumWords; ++Word) {
        MachineInstrBuilder Load =
            BuildMI(MBB, MI, Instr.getDebugLoc(),
                    TII->get(SH::FMOV_S_LOAD_POSTINC_ALIAS),
                    (IsDouble || IsVector) ? GetFPUWord(Word) : FPUReg)
                .addDef(Address)
                .addReg(Address, RegState::Kill)
                .addMemOperand(GetWordMMO(Word))
                .setMIFlags(Instr.getFlags());
        if (Word + 1 == NumWords)
          Load->getOperand(1).setIsDead();
      }
    } else {
      emitSHFrameAddress(MBB, MI, Instr.getDebugLoc(), *TII, Address, FrameReg,
                         ByteOffset + 4 * NumWords);
      for (unsigned I = 0; I != NumWords; ++I) {
        unsigned Word = NumWords - I - 1;
        Register Src = FPUReg;
        if (IsDouble || IsVector)
          Src = GetFPUWord(Word);
        MachineInstrBuilder Store =
            BuildMI(MBB, MI, Instr.getDebugLoc(),
                    TII->get(SH::FMOV_S_STORE_PREDEC_ALIAS), Address)
                .addReg(Src,
                        getKillRegState(Instr.getOperand(0).isKill() &&
                                        I + 1 == NumWords) |
                            getUndefRegState(Instr.getOperand(0).isUndef()))
                .addReg(Address, RegState::Kill)
                .addMemOperand(GetWordMMO(Word))
                .setMIFlags(Instr.getFlags());
        if (I + 1 == NumWords)
          Store->getOperand(0).setIsDead();
      }
    }
    MBB.erase(MI);
    return false;
  }
  if (Opcode == SH::LEA_FI) {
    if (FIOperandNum != 1 || Instr.getNumOperands() != 3 ||
        !Instr.getOperand(0).isReg() ||
        !Instr.getOperand(FIOperandNum + 1).isImm())
      report_fatal_error("SH frame address pseudo is malformed");
    MachineFunction &MF = *Instr.getParent()->getParent();
    const MachineFrameInfo &MFI = MF.getFrameInfo();
    int FrameIndex = Instr.getOperand(FIOperandNum).getIndex();
    int64_t BaseOffset;
    Register FrameReg =
        getSHFrameReferenceBase(MF, FrameIndex, SPAdj, BaseOffset);
    int64_t ByteOffset = MFI.getObjectOffset(FrameIndex) + BaseOffset +
                         Instr.getOperand(FIOperandNum + 1).getImm();
    if (ByteOffset < SHMinFrameAddressOffset ||
        ByteOffset > SHMaxFrameAddressOffset)
      report_fatal_error(Twine("SH finalized frame address offset ") +
                         Twine(ByteOffset) + " must be in [" +
                         Twine(SHMinFrameAddressOffset) + ", " +
                         Twine(SHMaxFrameAddressOffset) + "] from r15");
    const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
    MachineBasicBlock &MBB = *Instr.getParent();
    Register Destination = Instr.getOperand(0).getReg();
    emitSHFrameAddress(MBB, MI, Instr.getDebugLoc(), *TII, Destination,
                       FrameReg, ByteOffset);
    MBB.erase(MI);
    return false;
  }
  bool IsLong = Opcode == SH::MOVL_load_disp || Opcode == SH::MOVL_store_disp;
  bool IsByte = Opcode == SH::MOVB_load_frame || Opcode == SH::MOVB_store_frame;
  bool IsWord = Opcode == SH::MOVW_load_frame || Opcode == SH::MOVW_store_frame;
  if (!IsLong && !IsByte && !IsWord)
    reportFatalUsageError("SH frame index used by an unsupported instruction");
  if (FIOperandNum + 1 >= Instr.getNumOperands() ||
      !Instr.getOperand(FIOperandNum + 1).isImm())
    report_fatal_error("SH frame reference is missing its byte displacement");

  MachineFunction &MF = *Instr.getParent()->getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  int FrameIndex = Instr.getOperand(FIOperandNum).getIndex();
  int64_t BaseOffset;
  Register FrameReg =
      getSHFrameReferenceBase(MF, FrameIndex, SPAdj, BaseOffset);
  int64_t ByteOffset = MFI.getObjectOffset(FrameIndex) + BaseOffset +
                       Instr.getOperand(FIOperandNum + 1).getImm();

  bool IsLoad = Opcode == SH::MOVL_load_disp || Opcode == SH::MOVB_load_frame ||
                Opcode == SH::MOVW_load_frame;
  if (IsLong && ByteOffset % 4 != 0)
    report_fatal_error(Twine("SH finalized longword frame reference offset ") +
                       Twine(ByteOffset) + " must be four-byte aligned");
  if (IsWord && ByteOffset % 2 != 0)
    reportFatalUsageError(Twine("SH finalized word frame reference offset ") +
                          Twine(ByteOffset) +
                          " must be even and in [0, 30] from r15");
  bool NeedsAddressRegister = (IsLong && (ByteOffset < 0 || ByteOffset > 60)) ||
                              (IsByte && (ByteOffset < 0 || ByteOffset > 15)) ||
                              (IsWord && (ByteOffset < 0 || ByteOffset > 30));
  if (NeedsAddressRegister) {
    if (ByteOffset < SHMinFrameAddressOffset ||
        ByteOffset > SHMaxFrameAddressOffset)
      report_fatal_error(Twine("SH finalized materialized frame offset ") +
                         Twine(ByteOffset) + " must be in [" +
                         Twine(SHMinFrameAddressOffset) + ", " +
                         Twine(SHMaxFrameAddressOffset) + "] from r15");
    const SHInstrInfo *TII = MF.getSubtarget<SHSubtarget>().getInstrInfo();
    MachineBasicBlock &MBB = *Instr.getParent();
    if (emitSHAdjacentMixedFrameAccesses(MI, SPAdj, RS, *TII))
      return true;
    if (!IsLoad && emitSHAdjacentFrameStores(MI, SPAdj, RS, *TII))
      return true;
    if (IsLoad && emitSHAdjacentFrameLoads(MI, SPAdj, RS, *TII))
      return true;
    bool UseNarrowIndexed =
        FrameReg == SH::R15 &&
        ((IsByte && ByteOffset >= 16 && ByteOffset <= 127) ||
         (IsWord && ByteOffset >= 32 && ByteOffset <= 126)) &&
        (IsLoad || Instr.getOperand(0).getReg() != SH::R0);
    if (IsLong && !IsLoad && RS && RS->isScavengingFrameIndex(FrameIndex)) {
      int64_t Adjustment = ByteOffset - 60;
      bool AdjustCFA = FrameReg == SH::R14 || FrameReg == SH::R15;
      emitSHTemporaryFrameAdjustment(MF, MBB, MI, Instr.getDebugLoc(), *TII,
                                     FrameReg, Adjustment, AdjustCFA);
      BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(SH::MOVL_store_disp))
          .add(Instr.getOperand(0))
          .addReg(FrameReg)
          .addImm(60)
          .setMIFlags(Instr.getFlags())
          .cloneMemRefs(Instr);
      emitSHTemporaryFrameAdjustment(MF, MBB, MI, Instr.getDebugLoc(), *TII,
                                     FrameReg, -Adjustment, AdjustCFA);
      MBB.erase(MI);
      return false;
    }
    unsigned RegisterOpcode =
        IsLong   ? (IsLoad ? SH::MOVL_load_reg : SH::MOVL_store_reg)
        : IsByte ? (IsLoad ? SH::MOVB_load_reg : SH::MOVB_store_reg)
                 : (IsLoad ? SH::MOVW_load_reg : SH::MOVW_store_reg);
    Register Address = IsLoad ? Instr.getOperand(0).getReg() : Register();
    if (!IsLoad && !UseNarrowIndexed) {
      if (!RS)
        report_fatal_error("SH large frame store requires register scavenging");
      RS->setRegUsed(Instr.getOperand(0).getReg());
      Address = RS->FindUnusedReg(&SH::GPRRegClass);
      if (!Address) {
        Address = RS->scavengeRegisterBackwards(SH::GPRRegClass, MI,
                                                /*RestoreAfter=*/false, SPAdj);
        if (Address)
          RS->setRegUsed(Address);
      }
      if (!Address)
        report_fatal_error(
            "SH could not scavenge a large frame address register");
    }
    int64_t AddressOffset = ByteOffset;
    int64_t MemoryOffset = 0;
    bool UseMaterializedIndexedAddress =
        FrameReg == SH::R15 && ByteOffset > 127 &&
        (!IsLong || ByteOffset > 187) &&
        (Address == SH::R0 || (IsLoad && RS && !RS->isRegUsed(SH::R0)));
    bool UseIndexedFrameAddress =
        UseNarrowIndexed || UseMaterializedIndexedAddress ||
        (IsLong && FrameReg == SH::R15 &&
         (Address == SH::R0 || (IsLoad && RS && !RS->isRegUsed(SH::R0))) &&
         ByteOffset >= 64 && ByteOffset <= 127);
    if (IsLong && FrameReg == SH::R15 && ByteOffset >= 128 &&
        ByteOffset <= 187) {
      AddressOffset -= 60;
      MemoryOffset = 60;
    }
    if (UseIndexedFrameAddress) {
      if (ByteOffset <= 127)
        BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(SH::MOVri), SH::R0)
            .addImm(ByteOffset);
      else
        TII->materializeI32(MBB, MI, Instr.getDebugLoc(), SH::R0,
                            static_cast<uint32_t>(ByteOffset));
    } else
      emitSHFrameAddress(MBB, MI, Instr.getDebugLoc(), *TII, Address, FrameReg,
                         AddressOffset);
    unsigned MemoryOpcode = RegisterOpcode;
    if (UseIndexedFrameAddress) {
      if (IsLong)
        MemoryOpcode = IsLoad ? SH::MOVL_load_indexed : SH::MOVL_store_indexed;
      else if (IsByte)
        MemoryOpcode = IsLoad ? SH::MOVB_load_indexed : SH::MOVB_store_indexed;
      else
        MemoryOpcode = IsLoad ? SH::MOVW_load_indexed : SH::MOVW_store_indexed;
    } else if (MemoryOffset)
      MemoryOpcode = IsLoad ? SH::MOVL_load_disp : SH::MOVL_store_disp;
    if (IsLoad) {
      MachineInstrBuilder Load =
          BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(MemoryOpcode),
                  Instr.getOperand(0).getReg())
              .addReg(UseIndexedFrameAddress ? FrameReg : Address);
      if (MemoryOffset)
        Load.addImm(MemoryOffset);
      Load.setMIFlags(Instr.getFlags()).cloneMemRefs(Instr);
      if (UseIndexedFrameAddress)
        Load->findRegisterUseOperand(SH::R0, this)->setIsKill(true);
      Load->getOperand(0).setIsDead(Instr.getOperand(0).isDead());
    } else {
      MachineInstrBuilder Store =
          BuildMI(MBB, MI, Instr.getDebugLoc(), TII->get(MemoryOpcode))
              .add(Instr.getOperand(0))
              .addReg(UseIndexedFrameAddress ? FrameReg : Address,
                      getKillRegState(!UseIndexedFrameAddress));
      if (MemoryOffset)
        Store.addImm(MemoryOffset);
      Store.setMIFlags(Instr.getFlags()).cloneMemRefs(Instr);
      if (UseIndexedFrameAddress)
        Store->findRegisterUseOperand(SH::R0, this)->setIsKill(true);
    }
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

  Instr.getOperand(FIOperandNum).ChangeToRegister(FrameReg, false);
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
  const SHFrameLowering *TFI =
      MF.getSubtarget<SHSubtarget>().getFrameLowering();
  return TFI->hasFP(MF) ? SH::R14 : SH::R15;
}
