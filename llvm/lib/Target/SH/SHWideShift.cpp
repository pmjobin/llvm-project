//===-- SHWideShift.cpp - Exact two-word SH shifts ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SH.h"
#include "SHInstrInfo.h"
#include "SHSubtarget.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

namespace {
static Register createGPR(MachineRegisterInfo &MRI) {
  return MRI.createVirtualRegister(&SH::GPRRegClass);
}

// Count is masked exactly as in the original bit loop. For right-small at
// n=0, (H << 1) << (31-n) is zero modulo32 bits; no SHLD-by-positive32 trap.
static MachineBasicBlock *emitNativeWideShift(MachineInstr &MI,
                                              MachineBasicBlock *Entry) {
  MachineFunction &MF = *Entry->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHSubtarget &ST = MF.getSubtarget<SHSubtarget>();
  const SHInstrInfo &TII = *ST.getInstrInfo();
  const bool SH3Boundary = ST.getCPU() == "sh3";
  const DebugLoc DL = MI.getDebugLoc();
  const bool Left = MI.getOpcode() == SH::SHL64_PSEUDO;
  const bool Arithmetic = MI.getOpcode() == SH::SRA64_PSEUDO;
  Register DestLo = MI.getOperand(0).getReg();
  Register DestHi = MI.getOperand(1).getReg();
  Register SourceLo = MI.getOperand(2).getReg();
  Register SourceHi = MI.getOperand(3).getReg();
  Register Count = MI.getOperand(4).getReg();
  MachineBasicBlock *Small = MF.CreateMachineBasicBlock(Entry->getBasicBlock());
  MachineBasicBlock *Large = MF.CreateMachineBasicBlock(Entry->getBasicBlock());
  MachineBasicBlock *Done = MF.CreateMachineBasicBlock(Entry->getBasicBlock());
  auto Insert = std::next(Entry->getIterator());
  MF.insert(Insert, Small);
  MF.insert(Insert, Large);
  MF.insert(Insert, Done);
  Done->splice(Done->begin(), Entry, std::next(MI.getIterator()), Entry->end());
  Done->transferSuccessorsAndUpdatePHIs(Entry);
  Entry->addSuccessor(Small);
  Entry->addSuccessor(Large);
  Small->addSuccessor(Done);
  Large->addSuccessor(Done);
  Register Mask = createGPR(MRI), N = createGPR(MRI), Bit5 = createGPR(MRI);
  BuildMI(*Entry, MI, DL, TII.get(SH::MOVri), Mask).addImm(63);
  BuildMI(*Entry, MI, DL, TII.get(SH::ANDrr), N).addReg(Mask).addReg(Count);
  BuildMI(*Entry, MI, DL, TII.get(SH::MOVri), Bit5).addImm(32);
  BuildMI(*Entry, MI, DL, TII.get(SH::TST)).addReg(N).addReg(Bit5);
  BuildMI(*Entry, MI, DL, TII.get(SH::BF)).addMBB(Large);
  auto Unary = [&](MachineBasicBlock *B, unsigned Op, Register Input) {
    Register Out = createGPR(MRI);
    BuildMI(*B, B->end(), DL, TII.get(Op), Out).addReg(Input);
    return Out;
  };
  auto Binary = [&](MachineBasicBlock *B, unsigned Op, Register A, Register C) {
    Register Out = createGPR(MRI);
    BuildMI(*B, B->end(), DL, TII.get(Op), Out).addReg(A).addReg(C);
    return Out;
  };
  auto Add = [&](MachineBasicBlock *B, Register Input, int Amount) {
    Register Out = createGPR(MRI);
    BuildMI(*B, B->end(), DL, TII.get(SH::ADDri), Out)
        .addReg(Input)
        .addImm(Amount);
    return Out;
  };
  auto Constant = [&](MachineBasicBlock *B, int Value) {
    Register Out = createGPR(MRI);
    BuildMI(*B, B->end(), DL, TII.get(SH::MOVri), Out).addImm(Value);
    return Out;
  };
  Register SmallLo, SmallHi, LargeLo, LargeHi;
  if (Left) {
    SmallLo = Binary(Small, SH::SHLD, SourceLo, N);
    Register High = Binary(Small, SH::SHLD, SourceHi, N);
    // SH3's operation pseudocode and prose disagree at a negative count whose
    // low five bits are zero. (L >> 1) >> (31-n) avoids that boundary,
    // including n=0 where the zero top bit makes the cross word zero. Keep
    // SH4's documented negative-32 sequence, which needs one fewer instruction.
    Register CrossSource =
        SH3Boundary ? Unary(Small, SH::SHLR, SourceLo) : SourceLo;
    Register CrossCount = Add(Small, N, SH3Boundary ? -31 : -32);
    Register Cross = Binary(Small, SH::SHLD, CrossSource, CrossCount);
    SmallHi = Binary(Small, SH::ORrr, High, Cross);
    Register LargeCount = Add(Large, N, -32);
    LargeLo = Constant(Large, 0);
    LargeHi = Binary(Large, SH::SHLD, SourceLo, LargeCount);
  } else {
    unsigned RightOpcode = Arithmetic ? SH::SHAD : SH::SHLD;
    Register NegativeN = Unary(Small, SH::NEG, N);
    SmallHi = Binary(Small, RightOpcode, SourceHi, NegativeN);
    Register Low = Binary(Small, SH::SHLD, SourceLo, NegativeN);
    Register TwiceHigh = Unary(Small, SH::SHLL, SourceHi);
    Register CrossCount = Add(Small, NegativeN, 31);
    Register Cross = Binary(Small, SH::SHLD, TwiceHigh, CrossCount);
    SmallLo = Binary(Small, SH::ORrr, Low, Cross);
    Register LargeCount = Add(Large, N, -32);
    Register NegativeLargeCount = Unary(Large, SH::NEG, LargeCount);
    LargeLo = Binary(Large, RightOpcode, SourceHi, NegativeLargeCount);
    if (Arithmetic) {
      // A signed32-bit value shifted right31 already is its full sign mask.
      // This also avoids relying on the negative-32 boundary on SH3.
      Register SignCount = Constant(Large, -31);
      LargeHi = Binary(Large, SH::SHAD, SourceHi, SignCount);
    } else {
      LargeHi = Constant(Large, 0);
    }
  }
  BuildMI(*Small, Small->end(), DL, TII.get(SH::BRA)).addMBB(Done);
  BuildMI(*Done, Done->begin(), DL, TII.get(TargetOpcode::PHI), DestLo)
      .addReg(SmallLo)
      .addMBB(Small)
      .addReg(LargeLo)
      .addMBB(Large);
  BuildMI(*Done, Done->begin(), DL, TII.get(TargetOpcode::PHI), DestHi)
      .addReg(SmallHi)
      .addMBB(Small)
      .addReg(LargeHi)
      .addMBB(Large);
  MI.eraseFromParent();
  return Done;
}
} // namespace

MachineBasicBlock *llvm::emitSHVariableI64Shift(MachineInstr &MI,
                                                MachineBasicBlock *EntryMBB) {
  MachineFunction &MF = *EntryMBB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHSubtarget &STI = MF.getSubtarget<SHSubtarget>();
  const SHInstrInfo &TII = *STI.getInstrInfo();
  const Function &F = MF.getFunction();
  if (STI.hasDynamicShift() &&
      (STI.getCPU() == "sh3" || STI.getCPU() == "sh4") &&
      STI.getTuneCPU() == STI.getCPU() &&
      MF.getTarget().getOptLevel() != CodeGenOptLevel::None &&
      !F.hasOptNone() && !F.hasOptSize() && !F.hasMinSize())
    return emitNativeWideShift(MI, EntryMBB);
  const DebugLoc &DL = MI.getDebugLoc();
  const BasicBlock *IRBB = EntryMBB->getBasicBlock();
  MachineFunction::iterator Insert = std::next(EntryMBB->getIterator());

  MachineBasicBlock *LoopMBB = MF.CreateMachineBasicBlock(IRBB);
  MachineBasicBlock *DoneMBB = MF.CreateMachineBasicBlock(IRBB);
  MF.insert(Insert, LoopMBB);
  MF.insert(Insert, DoneMBB);

  DoneMBB->splice(DoneMBB->begin(), EntryMBB,
                  std::next(MachineBasicBlock::iterator(MI)), EntryMBB->end());
  DoneMBB->transferSuccessorsAndUpdatePHIs(EntryMBB);

  EntryMBB->addSuccessor(LoopMBB);
  EntryMBB->addSuccessor(DoneMBB);
  LoopMBB->addSuccessor(LoopMBB);
  LoopMBB->addSuccessor(DoneMBB);

  Register DestLo = MI.getOperand(0).getReg();
  Register DestHi = MI.getOperand(1).getReg();
  Register SourceLo = MI.getOperand(2).getReg();
  Register SourceHi = MI.getOperand(3).getReg();
  Register Count = MI.getOperand(4).getReg();
  Register Mask = createGPR(MRI);
  Register MaskedCount = createGPR(MRI);
  Register LoPhi = createGPR(MRI);
  Register HiPhi = createGPR(MRI);
  Register CountPhi = createGPR(MRI);
  Register NextLo = createGPR(MRI);
  Register NextHi = createGPR(MRI);
  Register NextCount = createGPR(MRI);

  BuildMI(*EntryMBB, MI, DL, TII.get(SH::MOVri), Mask).addImm(63);
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::ANDrr), MaskedCount)
      .addReg(Mask)
      .addReg(Count);
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::TST))
      .addReg(MaskedCount)
      .addReg(MaskedCount);
  BuildMI(*EntryMBB, MI, DL, TII.get(SH::BT)).addMBB(DoneMBB);

  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TargetOpcode::PHI), LoPhi)
      .addReg(SourceLo)
      .addMBB(EntryMBB)
      .addReg(NextLo)
      .addMBB(LoopMBB);
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TargetOpcode::PHI), HiPhi)
      .addReg(SourceHi)
      .addMBB(EntryMBB)
      .addReg(NextHi)
      .addMBB(LoopMBB);
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(TargetOpcode::PHI), CountPhi)
      .addReg(MaskedCount)
      .addMBB(EntryMBB)
      .addReg(NextCount)
      .addMBB(LoopMBB);

  switch (MI.getOpcode()) {
  default:
    llvm_unreachable("unexpected SH i64 variable shift pseudo");
  case SH::SHL64_PSEUDO:
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::SHLL), NextLo)
        .addReg(LoPhi);
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::ROTCL), NextHi)
        .addReg(HiPhi);
    break;
  case SH::SRL64_PSEUDO:
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::SHLR), NextHi)
        .addReg(HiPhi);
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::ROTCR), NextLo)
        .addReg(LoPhi);
    break;
  case SH::SRA64_PSEUDO:
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::SHAR), NextHi)
        .addReg(HiPhi);
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::ROTCR), NextLo)
        .addReg(LoPhi);
    break;
  }
  if (STI.hasSH2())
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::DT), NextCount)
        .addReg(CountPhi);
  else {
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::ADDri), NextCount)
        .addReg(CountPhi)
        .addImm(-1);
    BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::TST))
        .addReg(NextCount)
        .addReg(NextCount);
  }
  BuildMI(*LoopMBB, LoopMBB->end(), DL, TII.get(SH::BF)).addMBB(LoopMBB);

  BuildMI(*DoneMBB, DoneMBB->begin(), DL, TII.get(TargetOpcode::PHI), DestLo)
      .addReg(SourceLo)
      .addMBB(EntryMBB)
      .addReg(NextLo)
      .addMBB(LoopMBB);
  BuildMI(*DoneMBB, DoneMBB->begin(), DL, TII.get(TargetOpcode::PHI), DestHi)
      .addReg(SourceHi)
      .addMBB(EntryMBB)
      .addReg(NextHi)
      .addMBB(LoopMBB);

  MI.eraseFromParent();
  return DoneMBB;
}
