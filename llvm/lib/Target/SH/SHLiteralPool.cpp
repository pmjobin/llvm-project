//===-- SHLiteralPool.cpp - SH inline literal-island helpers --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHLiteralPool.h"
#include "SHConstantPoolValue.h"
#include "SHInstrInfo.h"
#include "SHSubtarget.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Alignment.h"

using namespace llvm;

bool llvm::isSHLiteralIslandBlock(const MachineBasicBlock &MBB) {
  bool HasEntry = false;
  for (const MachineInstr &MI : MBB) {
    if (MI.isMetaInstruction())
      continue;
    if (MI.getOpcode() != SH::SH_CONSTPOOL_ENTRY)
      return false;
    HasEntry = true;
  }
  return HasEntry;
}

bool llvm::isSHJumpTableBlock(const MachineBasicBlock &MBB) {
  bool HasEntry = false;
  for (const MachineInstr &MI : MBB) {
    if (MI.isMetaInstruction())
      continue;
    if (MI.getOpcode() != SH::SH_JUMPTABLE)
      return false;
    HasEntry = true;
  }
  return HasEntry;
}

bool llvm::isSHDataIslandBlock(const MachineBasicBlock &MBB) {
  return isSHLiteralIslandBlock(MBB) || isSHJumpTableBlock(MBB);
}

bool llvm::coalesceSHLongLiteralEntries(MachineFunction &MF) {
  if (!MF.getFunction().hasOptSize() && !MF.getFunction().hasMinSize())
    return false;
  const auto &TII = *MF.getSubtarget<SHSubtarget>().getInstrInfo();
  const auto &Constants = MF.getConstantPool()->getConstants();
  DenseMap<const MachineInstr *, uint64_t> Offsets;
  DenseMap<uint64_t, SmallVector<MachineInstr *, 4>> Uses;
  DenseMap<unsigned, SmallVector<MachineInstr *, 4>> Retained;
  DenseSet<const MCSymbol *> SymbolUses;
  SmallVector<MachineInstr *, 16> Entries;
  auto Key = [](unsigned CPI, unsigned Instance) {
    return uint64_t(CPI) << 32 | Instance;
  };
  uint64_t Offset = 0;
  for (MachineBasicBlock &MBB : MF) {
    // Four-byte deletions preserve every padding decision and PC residue.
    // Larger alignments and inexact instruction sizes need a different
    // layout proof. Leave those functions to the ordinary range validator.
    if (MBB.getAlignment() > Align(4) ||
        (MBB.isBeginSection() && !MBB.isEntryBlock()))
      return false;
    uint64_t Aligned = alignTo(Offset, MBB.getAlignment());
    unsigned MaxPadding = MBB.getMaxBytesForAlignment();
    if (!MaxPadding || Aligned - Offset <= MaxPadding)
      Offset = Aligned;
    for (MachineInstr &MI : MBB) {
      if (MI.isInlineAsm() || MI.getOpcode() == SH::SH_LONG_BRANCH ||
          MI.getOpcode() == TargetOpcode::BUNDLE)
        return false;
      for (const MachineOperand &MO : MI.operands())
        if (MO.isMCSymbol())
          SymbolUses.insert(MO.getMCSymbol());
      if (MI.isBundledWithPred())
        continue;
      Offsets[&MI] = Offset;
      if (MI.getOpcode() == SH::MOVL_load_pc_island)
        Uses[Key(MI.getOperand(1).getIndex(), MI.getOperand(2).getImm())]
            .push_back(&MI);
      if (MI.getOpcode() == SH::SH_CONSTPOOL_ENTRY &&
          MI.getOperand(2).getImm() == 4)
        Entries.push_back(&MI);
      unsigned Size = TII.getInstSizeInBytes(MI);
      if (!Size && !MI.isMetaInstruction())
        return false;
      Offset += Size;
    }
  }

  bool Changed = false;
  for (MachineInstr *Entry : llvm::reverse(Entries)) {
    unsigned CPI = Entry->getOperand(0).getIndex();
    unsigned Instance = Entry->getOperand(1).getImm();
    if (Constants[CPI].isMachineConstantPoolEntry() &&
        static_cast<const SHConstantPoolValue *>(
            Constants[CPI].Val.MachineCPVal)
                ->getModifier() != SHConstantPoolValue::Modifier::None)
      continue;
    const MCSymbol *Symbol =
        getSHLiteralIslandSymbol(MF.getContext(), MF.getDataLayout(),
                                 MF.getFunctionNumber(), CPI, Instance);
    if (SymbolUses.contains(Symbol))
      continue;
    auto UseIt = Uses.find(Key(CPI, Instance));
    if (UseIt == Uses.end())
      continue;
    auto &Candidates = Retained[CPI];
    auto Reusable = llvm::find_if(Candidates, [&](MachineInstr *Other) {
      uint64_t EntryOffset = Offsets.lookup(Other);
      return llvm::all_of(UseIt->second, [&](MachineInstr *Use) {
        uint64_t Base = SH::getPCRelativeLongBase(Offsets.lookup(Use));
        return EntryOffset >= Base &&
               EntryOffset - Base <= SHLongLiteralLoadMaxDistance &&
               (EntryOffset - Base) % SH::PCRelativeLongScale == 0;
      });
    });
    if (Reusable == Candidates.end()) {
      Candidates.push_back(Entry);
      continue;
    }
    // Keep instruction order, registers, pool identities and address-taken
    // entries intact. Every removal is four bytes, so all existing branch,
    // literal and jump-table displacements contract with unchanged padding.
    // Checking the original layout also bounds all simultaneous removals.
    for (MachineInstr *Use : UseIt->second)
      Use->getOperand(2).setImm((*Reusable)->getOperand(1).getImm());
    Entry->eraseFromParent();
    Changed = true;
  }
  return Changed;
}

MCSymbol *llvm::getSHLiteralIslandSymbol(MCContext &Ctx, const DataLayout &DL,
                                         unsigned FunctionNumber, unsigned CPI,
                                         unsigned Instance) {
  return Ctx.getOrCreateSymbol(Twine(DL.getInternalSymbolPrefix()) + "CPI" +
                               Twine(FunctionNumber) + "_" + Twine(CPI) + "_" +
                               Twine(Instance));
}
