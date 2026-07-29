//===-- SHLiteralPool.h - SH trailing literal-pool layout -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHLITERALPOOL_H
#define LLVM_LIB_TARGET_SH_SHLITERALPOOL_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Alignment.h"
#include <cstdint>

namespace llvm {

class MachineFunction;

struct SHLiteralPoolEntryLayout {
  uint64_t Offset;
  unsigned Size;
  Align Alignment;
};

struct SHLiteralPoolLayout {
  Align Alignment = Align(4);
  SmallVector<SHLiteralPoolEntryLayout, 8> Entries;
  uint64_t Size = 0;
};

SHLiteralPoolLayout computeSHLiteralPoolLayout(const MachineFunction &MF);

} // namespace llvm

#endif
