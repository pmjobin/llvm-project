//===-- SHBranchRange.cpp - Preserve ranges during late layout changes
//------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHBranchRange.h"
#include "MCTargetDesc/SHBaseInfo.h"
#include "SHInlineAsmSize.h"
#include "SHInstrInfo.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/CodeGen/MachineFunction.h"

using namespace llvm;

bool llvm::preservesSHBranchRanges(
    const MachineFunction &MF, const SHInstrInfo &TII,
    function_ref<unsigned(const MachineInstr &, unsigned)> GetSizeAfter) {
  DenseMap<const MachineBasicBlock *, std::pair<uint64_t, uint64_t>> Blocks;
  DenseMap<const MachineInstr *, std::pair<uint64_t, uint64_t>> Branches;
  uint64_t Before = 0;
  uint64_t After = 0;
  for (const MachineBasicBlock &MBB : MF) {
    // A stronger alignment or a separate section leaves the starting
    // residue unknown. Keep the incoming layout in that case.
    if (MBB.getAlignment() > MF.getAlignment() ||
        (MBB.isBeginSection() && !MBB.isEntryBlock()))
      return false;
    auto AlignOffset = [&](uint64_t Offset) {
      uint64_t Aligned = alignTo(Offset, MBB.getAlignment());
      unsigned MaxPadding = MBB.getMaxBytesForAlignment();
      return MaxPadding && Aligned - Offset > MaxPadding ? Offset : Aligned;
    };
    Before = AlignOffset(Before);
    After = AlignOffset(After);
    Blocks[&MBB] = {Before, After};
    for (const MachineInstr &MI : MBB) {
      // Generic bundles can hide branches. Long branches have internal
      // alignment, and their reported size is only an upper bound.
      if (MI.getOpcode() == TargetOpcode::BUNDLE ||
          MI.getOpcode() == SH::SH_LONG_BRANCH)
        return false;
      if (MI.isBundledWithPred())
        continue;
      if (MI.isInlineAsm() &&
          getSHInlineAsmSize(MI.getOperand(0).getSymbolName()).Kind !=
              SHInlineAsmSizeKind::Exact)
        return false;
      unsigned Opcode = MI.getOpcode();
      if (Opcode == SH::BT || Opcode == SH::BF || Opcode == SH::BT_S ||
          Opcode == SH::BF_S || Opcode == SH::BRA)
        Branches[&MI] = {Before, After};
      unsigned Size = TII.getInstSizeInBytes(MI);
      if (!Size && !MI.isMetaInstruction() && !MI.isInlineAsm())
        return false;
      Before += Size;
      After += GetSizeAfter(MI, Size);
    }
  }
  for (const auto &[MI, Offsets] : Branches) {
    if (!MI->getOperand(0).isMBB())
      continue;
    const MachineBasicBlock *Target = MI->getOperand(0).getMBB();
    if (Target->getSectionID() != MI->getParent()->getSectionID())
      return false;
    auto TargetOffsets = Blocks.lookup(Target);
    unsigned Bits = MI->getOpcode() == SH::BRA
                        ? SH::UnconditionalBranchDisplacementBits
                        : SH::ConditionalBranchDisplacementBits;
    int64_t OldOffset = int64_t(TargetOffsets.first) - int64_t(Offsets.first);
    int64_t NewOffset = int64_t(TargetOffsets.second) - int64_t(Offsets.second);
    // Shrinking before a branch can increase its distance across aligned
    // blocks. Check all branches, including ones the caller does not move.
    if (SH::isBranchOffsetInRange(OldOffset, Bits) &&
        !SH::isBranchOffsetInRange(NewOffset, Bits))
      return false;
  }
  return true;
}
