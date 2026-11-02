//===-- SHGraphics.cpp - Bounded graphics instruction expansion -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHISelLowering.h"
#include "SHInstrInfo.h"
#include "SHSubtarget.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

// A region is expanded before RA so the normal allocator owns every vector,
// loop-carried pointer and spill. XMTRX is a fixed resource only between BEGIN
// and END. The loop never speculatively reads a vector past its declared count.
MachineBasicBlock *
SHTargetLowering::emitGraphicsRegion(MachineInstr &MI,
                                     MachineBasicBlock *Entry) const {
  MachineFunction &MF = *Entry->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const SHInstrInfo &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const DebugLoc DL = MI.getDebugLoc();
  bool Fixed = MI.getOpcode() == SH::FTRV4_MEM;
  bool Compose = MI.getOpcode() == SH::FTRV_COMPOSE;
  auto MMOs = MI.memoperands();
  MachineMemOperand *Private = MMOs.back();
  Register Dst = MI.getOperand(0).getReg();
  Register Src = MI.getOperand(1).getReg();
  Register Count = Fixed ? Register() : MI.getOperand(2).getReg();
  unsigned FixedCount = Fixed ? 4 : 0;
  if (!Fixed)
    if (MachineInstr *Def = MRI.getVRegDef(Count);
        Def && Def->getOpcode() == SH::MOVri &&
        Def->getOperand(1).getImm() > 0 && Def->getOperand(1).getImm() <= 4)
      FixedCount = Def->getOperand(1).getImm();
  Register Matrix = MI.getOperand(Fixed ? 2 : 3).getReg();
  Register Save = MI.getOperand(Fixed ? 3 : Compose ? 5 : 4).getReg();
  bool Paired =
      MMOs[0]->getAlign() >= Align(8) && MMOs[1]->getAlign() >= Align(8);
  auto GPR = [&]() { return MRI.createVirtualRegister(&SH::GPRRegClass); };
  auto FV = [&]() { return MRI.createVirtualRegister(&SH::FVRegClass); };
  MachineBasicBlock *Current = Entry;
  MachineBasicBlock::iterator Pos = MI.getIterator();
  auto At = [&](MachineBasicBlock *BB) {
    Current = BB;
    Pos = BB->end();
  };
  auto Emit = [&](unsigned Opcode) {
    return BuildMI(*Current, Pos, DL, TII.get(Opcode));
  };
  auto Access = [&](unsigned Arg, unsigned Offset, bool Dynamic) {
    if (Dynamic)
      // The loop advances its address. Do not falsely describe every
      // iteration as a slice at offset zero of the original argument.
      return MF.getMachineMemOperand(
          MachinePointerInfo(),
          Arg == 0 ? MachineMemOperand::MOStore : MachineMemOperand::MOLoad, 16,
          std::min(MMOs[Arg]->getAlign(), Align(16)));
    return MF.getMachineMemOperand(MMOs[Arg], Offset, 16);
  };
  auto Load = [&](Register &Ptr, unsigned Arg, unsigned Offset, bool Pair,
                  bool Dynamic) {
    Register V = FV(), Next = GPR();
    Emit(Pair ? SH::FMOV_V_LOAD_PAIR : SH::FMOV_V_LOAD)
        .addDef(V)
        .addDef(Next)
        .addReg(Ptr)
        .addMemOperand(Access(Arg, Offset, Dynamic));
    Ptr = Next;
    return V;
  };
  auto Transform = [&](Register V) {
    Register Result = FV();
    Emit(SH::FTRV).addDef(Result).addReg(V);
    return Result;
  };
  auto Store = [&](Register V, Register &Ptr, unsigned Offset, bool Dynamic) {
    Register Next = GPR();
    Emit(Paired ? SH::FMOV_V_STORE_PAIR : SH::FMOV_V_STORE)
        .addDef(Next)
        .addReg(V)
        .addReg(Ptr)
        .addMemOperand(Access(0, Offset, Dynamic));
    Ptr = Next;
  };
  auto Stream = [&](unsigned N, Register &D, Register &S, bool Dynamic) {
    SmallVector<Register, 4> Values;
    for (unsigned I = 0; I != std::min(N, 2U); ++I)
      Values.push_back(Load(S, 1, 16 * I, Paired, Dynamic));
    for (unsigned I = 0; I != N; ++I) {
      Values[I] = Transform(Values[I]);
      if (I)
        Store(Values[I - 1], D, 16 * (I - 1), Dynamic);
      if (I + 2 < N)
        Values.push_back(Load(S, 1, 16 * (I + 2), Paired, Dynamic));
    }
    Store(Values.back(), D, 16 * (N - 1), Dynamic);
  };
  auto Begin = [&]() {
    Emit(SH::GRAPHICS_BEGIN)
        .addDef(GPR())
        .addDef(GPR())
        .addDef(GPR())
        .addReg(Matrix)
        .addReg(Save)
        .addMemOperand(MMOs[2])
        .addMemOperand(Private);
    if (Compose) {
      Register Right = MI.getOperand(4).getReg();
      SmallVector<Register, 4> Columns;
      for (unsigned I = 0; I != 4; ++I)
        Columns.push_back(Transform(
            Load(Right, 3, 16 * I, MMOs[3]->getAlign() >= Align(8), false)));
      auto Install = Emit(SH::XMTRX_FROM_FVS);
      for (Register Column : Columns)
        Install.addReg(Column);
    }
    if (!Paired)
      Emit(SH::GRAPHICS_SCALAR);
  };
  auto End = [&]() {
    Emit(SH::GRAPHICS_END)
        .addDef(GPR())
        .addReg(Save)
        .addMemOperand(MF.getMachineMemOperand(Private, 0, 16));
  };

  if (FixedCount) {
    Begin();
    Stream(FixedCount, Dst, Src, false);
    End();
    MI.eraseFromParent();
    return Entry;
  }

  auto Insert = std::next(Entry->getIterator());
  auto Block = [&]() {
    auto *BB = MF.CreateMachineBasicBlock(Entry->getBasicBlock());
    MF.insert(Insert, BB);
    return BB;
  };
  MachineBasicBlock *Setup = Block(), *Test = Block(), *Body = Block();
  MachineBasicBlock *TailTest = Block(), *Tail = Block(), *Cleanup = Block();
  MachineBasicBlock *Done = Block();
  Done->splice(Done->begin(), Entry, std::next(MI.getIterator()), Entry->end());
  Done->transferSuccessorsAndUpdatePHIs(Entry);
  At(Entry);
  Emit(SH::TST).addReg(Count).addReg(Count);
  Emit(SH::BT).addMBB(Done);
  Entry->addSuccessor(Setup);
  Entry->addSuccessor(Done);

  At(Setup);
  Begin();
  Register Three = GPR();
  Emit(SH::MOVri).addDef(Three).addImm(3);
  Setup->addSuccessor(Test);

  Register D = GPR(), S = GPR(), N = GPR();
  Register NextD, NextS, NextN = GPR();
  At(Body);
  NextD = D;
  NextS = S;
  Stream(3, NextD, NextS, true);
  Emit(SH::ADDri).addDef(NextN).addReg(N).addImm(-3);
  Emit(SH::BRA).addMBB(Test);
  Body->addSuccessor(Test);

  auto Phi = [&](Register Out, Register Initial, MachineBasicBlock *From,
                 Register Back, MachineBasicBlock *BackBB) {
    Emit(TargetOpcode::PHI)
        .addDef(Out)
        .addReg(Initial)
        .addMBB(From)
        .addReg(Back)
        .addMBB(BackBB);
  };
  At(Test);
  Phi(D, Dst, Setup, NextD, Body);
  Phi(S, Src, Setup, NextS, Body);
  Phi(N, Count, Setup, NextN, Body);
  Emit(SH::CMP_HS).addReg(Three).addReg(N);
  Emit(SH::BF).addMBB(TailTest);
  Test->addSuccessor(Body);
  Test->addSuccessor(TailTest);

  Register TD = GPR(), TS = GPR(), TN = GPR(), TailN = GPR();
  Register TailD = TD, TailS = TS;
  At(Tail);
  Stream(1, TailD, TailS, true);
  Emit(SH::ADDri).addDef(TailN).addReg(TN).addImm(-1);
  Emit(SH::BRA).addMBB(TailTest);
  Tail->addSuccessor(TailTest);

  At(TailTest);
  Phi(TD, D, Test, TailD, Tail);
  Phi(TS, S, Test, TailS, Tail);
  Phi(TN, N, Test, TailN, Tail);
  Emit(SH::TST).addReg(TN).addReg(TN);
  Emit(SH::BT).addMBB(Cleanup);
  TailTest->addSuccessor(Tail);
  TailTest->addSuccessor(Cleanup);

  At(Cleanup);
  End();
  Cleanup->addSuccessor(Done);
  MF.getProperties().resetNoPHIs();
  MI.eraseFromParent();
  return Done;
}

bool SHInstrInfo::expandGraphicsPseudo(MachineInstr &MI) const {
  unsigned Op = MI.getOpcode();
  bool Load = Op == SH::FMOV_V_LOAD || Op == SH::FMOV_V_LOAD_PAIR;
  bool Store = Op == SH::FMOV_V_STORE || Op == SH::FMOV_V_STORE_PAIR;
  bool Begin = Op == SH::GRAPHICS_BEGIN;
  bool End = Op == SH::GRAPHICS_END;
  bool Matrix = Begin || Op == SH::FTRV_MEM;
  bool MatrixLoad = Op == SH::XMTRX_LOAD;
  bool MatrixStore = Op == SH::XMTRX_STORE;
  if (!Load && !Store && !Matrix && !MatrixLoad && !MatrixStore && !End &&
      Op != SH::FSCA_VALUE && Op != SH::XMTRX_FROM_FVS &&
      Op != SH::GRAPHICS_SCALAR)
    return false;
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  const DebugLoc &DL = MI.getDebugLoc();
  const TargetRegisterInfo &TRI = getRegisterInfo();
  auto Emit = [&](unsigned Opcode) {
    return BuildMI(MBB, MI, DL, get(Opcode)).setMIFlags(MI.getFlags());
  };
  if (Op == SH::GRAPHICS_SCALAR) {
    MI.eraseFromParent();
    return true;
  }
  if (Op == SH::XMTRX_FROM_FVS) {
    const MCPhysReg Pairs[] = {SH::XD0, SH::XD2,  SH::XD4,  SH::XD6,
                               SH::XD8, SH::XD10, SH::XD12, SH::XD14};
    for (unsigned I = 0; I != 4; ++I)
      for (unsigned J = 0; J != 2; ++J) {
        Register Lane =
            TRI.getSubReg(MI.getOperand(I).getReg(), SH::sub_fpu32_0 + 2 * J);
        Register Pair =
            TRI.getMatchingSuperReg(Lane, SH::sub_fpu32_0, &SH::DRRegClass);
        Emit(SH::FMOV_P_COPY_XD).addDef(Pairs[2 * I + J]).addReg(Pair);
      }
    MI.eraseFromParent();
    return true;
  }
  if (Op == SH::FSCA_VALUE) {
    Emit(SH::LDS_FPUL).add(MI.getOperand(1));
    Emit(SH::FSCA).add(MI.getOperand(0));
    MI.eraseFromParent();
    return true;
  }
  auto Copy = [&](Register To, Register From) {
    if (To != From)
      Emit(SH::MOVrr).addDef(To).addReg(From);
  };
  auto Add = [&](Register R, int N) {
    Emit(SH::ADDri).addDef(R).addReg(R).addImm(N);
  };
  auto Slice = [&](const MachineMemOperand *MMO, unsigned Offset,
                   unsigned Size) {
    return MF.getMachineMemOperand(MMO, Offset, Size);
  };
  auto PairLoad = [&](Register R, Register P, bool Alternate,
                      MachineMemOperand *MMO) {
    MMO = MF.getMachineMemOperand(MMO, MMO->getFlags() &
                                           ~MachineMemOperand::MOStore);
    Emit(Alternate ? SH::FMOV_P_LOAD_XD : SH::FMOV_P_LOAD_DR)
        .addDef(R)
        .addDef(P)
        .addReg(P)
        .addMemOperand(MMO);
  };
  auto PairStore = [&](Register R, Register P, bool Alternate,
                       MachineMemOperand *MMO) {
    MMO = MF.getMachineMemOperand(MMO,
                                  MMO->getFlags() & ~MachineMemOperand::MOLoad);
    Emit(Alternate ? SH::FMOV_P_STORE_XD : SH::FMOV_P_STORE_DR)
        .addDef(P)
        .addReg(R)
        .addReg(P)
        .addMemOperand(MMO);
  };
  const MCPhysReg Pairs[] = {SH::XD0, SH::XD2,  SH::XD4,  SH::XD6,
                             SH::XD8, SH::XD10, SH::XD12, SH::XD14};
  if (MatrixLoad || MatrixStore) {
    Register P = MI.getOperand(0).getReg();
    unsigned First = MI.getOperand(2).getImm();
    unsigned Count = MI.getOperand(3).getImm();
    assert(Count && First + Count <= std::size(Pairs));
    const MachineMemOperand *MMO = *MI.memoperands_begin();
    if (MatrixStore)
      Add(P, 8 * Count);
    for (unsigned I = 0; I != Count; ++I) {
      unsigned Index = MatrixLoad ? I : Count - I - 1;
      auto *Part = Slice(MMO, 8 * Index, 8);
      if (MatrixLoad)
        PairLoad(Pairs[First + Index], P, true, Part);
      else
        PairStore(Pairs[First + Index], P, true, Part);
    }
    MI.eraseFromParent();
    return true;
  }
  auto MatrixTransfer = [&](Register P, bool Read, unsigned First,
                            unsigned Count, MachineMemOperand *MMO) {
    MachineInstr *Transfer = Emit(Read ? SH::XMTRX_LOAD : SH::XMTRX_STORE)
                                 .addDef(P)
                                 .addReg(P)
                                 .addImm(First)
                                 .addImm(Count)
                                 .addMemOperand(MMO);
    expandGraphicsPseudo(*Transfer);
  };
  if (End) {
    Register P = MI.getOperand(0).getReg();
    Copy(P, MI.getOperand(1).getReg());
    MatrixTransfer(P, true, 6, 2, Slice(*MI.memoperands_begin(), 0, 16));
    MI.eraseFromParent();
    return true;
  }
  auto Vector = [&](Register V, Register P, bool Read, bool Paired,
                    const MachineMemOperand *MMO, unsigned Offset) {
    // Stores walk backwards so both forms can use the native predecrement.
    if (!Read)
      Add(P, 16);
    unsigned Step = Paired ? 2 : 1;
    for (unsigned I = 0; I != 4; I += Step) {
      unsigned Lane = Read ? I : 4 - Step - I;
      Register R = TRI.getSubReg(V, SH::sub_fpu32_0 + Lane);
      auto *Part = Slice(MMO, Offset + 4 * Lane, 4 * Step);
      if (Paired) {
        R = TRI.getMatchingSuperReg(R, SH::sub_fpu32_0, &SH::DRRegClass);
        if (Read)
          PairLoad(R, P, false, Part);
        else
          PairStore(R, P, false, Part);
      } else if (Read) {
        Emit(SH::FMOV_S_LOAD_POSTINC_ALIAS)
            .addDef(R)
            .addDef(P)
            .addReg(P)
            .addMemOperand(Part);
      } else {
        Emit(SH::FMOV_S_STORE_PREDEC_ALIAS)
            .addDef(P)
            .addReg(R)
            .addReg(P)
            .addMemOperand(Part);
      }
    }
    if (!Read && !MI.getOperand(0).isDead())
      Add(P, 16);
  };
  if (Load || Store) {
    Register V = MI.getOperand(Load ? 0 : 1).getReg();
    Register P = MI.getOperand(Load ? 1 : 0).getReg();
    Copy(P, MI.getOperand(2).getReg());
    Vector(V, P, Load,
           Op == SH::FMOV_V_LOAD_PAIR || Op == SH::FMOV_V_STORE_PAIR,
           *MI.memoperands_begin(), 0);
    MI.eraseFromParent();
    return true;
  }

  Register Scratch = MI.getOperand(Begin ? 0 : 1).getReg();
  Register Temp = MI.getOperand(Begin ? 1 : 2).getReg();
  Register Word = MI.getOperand(Begin ? 2 : 3).getReg();
  Register MatrixPtr = MI.getOperand(Begin ? 3 : 5).getReg();
  Register Save = MI.getOperand(Begin ? 4 : 6).getReg();
  auto MMOs = MI.memoperands();
  const MachineMemOperand *MatrixMMO = MMOs[0];
  const MachineMemOperand *PrivateMMO = MMOs.back();

  // Save the alternate bank's ABI-preserved tail before taking ownership.
  Copy(Scratch, Save);
  MatrixTransfer(Scratch, false, 6, 2, Slice(PrivateMMO, 0, 16));

  const MachineMemOperand *LoadMMO = MatrixMMO;
  unsigned MatrixOffset = 0;
  if (MatrixMMO->getAlign() < Align(8)) {
    // Quadword accesses at 8n+4 fault on SH4. Copy an align4 source using
    // integer words into the aligned private frame buffer; no FRCHG needed.
    Copy(Scratch, MatrixPtr);
    Copy(Temp, Save);
    Add(Temp, 16);
    for (unsigned I = 0; I != 16; ++I) {
      Emit(SH::MOVL_load_reg)
          .addDef(Word)
          .addReg(Scratch)
          .addMemOperand(Slice(MatrixMMO, 4 * I, 4));
      Emit(SH::MOVL_store_reg)
          .addReg(Word)
          .addReg(Temp)
          .addMemOperand(MF.getMachineMemOperand(
              Slice(PrivateMMO, 16 + 4 * I, 4), MachineMemOperand::MOStore));
      Add(Scratch, 4);
      Add(Temp, 4);
    }
    Copy(Scratch, Save);
    Add(Scratch, 16);
    LoadMMO = PrivateMMO;
    MatrixOffset = 16;
  } else {
    Copy(Scratch, MatrixPtr);
  }
  MatrixTransfer(Scratch, true, 0, 8, Slice(LoadMMO, MatrixOffset, 64));

  if (!Begin) {
    Register V = MI.getOperand(0).getReg();
    Emit(SH::FTRV).addDef(V).addReg(V);
    Copy(Scratch, Save);
    MatrixTransfer(Scratch, true, 6, 2, Slice(PrivateMMO, 0, 16));
  }
  MI.eraseFromParent();
  return true;
}
