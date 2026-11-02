//===-- SHBranchRange.h - Preserve ranges during late layout changes
//-------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHBRANCHRANGE_H
#define LLVM_LIB_TARGET_SH_SHBRANCHRANGE_H

#include "llvm/ADT/STLFunctionalExtras.h"

namespace llvm {

class MachineFunction;
class MachineInstr;
class SHInstrInfo;

// Check a size change that preserves block order and direct branch identities.
// GetSizeAfter receives each instruction header and its current size, including
// any reserved delay slot. Reject changes when the layout cannot be modeled.
bool preservesSHBranchRanges(
    const MachineFunction &MF, const SHInstrInfo &TII,
    function_ref<unsigned(const MachineInstr &, unsigned)> GetSizeAfter);

} // namespace llvm

#endif
