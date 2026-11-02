//===-- SHCallFrameState.h - Custom inserter call frames ---------*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHCALLFRAMESTATE_H
#define LLVM_LIB_TARGET_SH_SHCALLFRAMESTATE_H

#include "llvm/ADT/ScopeExit.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

namespace llvm {

// Selects and shifts used as stack arguments can split a CALLSEQ. Every new
// block, including the continuation containing CALLSEQ_END, needs the active
// call-frame size so PEI can address spill slots correctly. SH custom inserters
// insert their blocks immediately after the original block.
inline auto preserveSHCallFrame(MachineInstr &MI, MachineBasicBlock &MBB) {
  const auto &TII = *MBB.getParent()->getSubtarget().getInstrInfo();
  int64_t Size = MBB.getCallFrameSize();
  for (const MachineInstr &Before : MBB) {
    if (&Before == &MI)
      break;
    Size += TII.getSPAdjust(Before);
  }
  auto OriginalNext = std::next(MBB.getIterator());
  return scope_exit([&MBB, OriginalNext, Size] {
    assert(Size >= 0 && "unbalanced SH call frame");
    for (auto I = std::next(MBB.getIterator()); I != OriginalNext; ++I)
      I->setCallFrameSize(Size);
  });
}

} // namespace llvm

#endif
