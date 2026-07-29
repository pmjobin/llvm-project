//===-- SHLiteralPool.cpp - SH trailing literal-pool layout ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHLiteralPool.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Support/ErrorHandling.h"
#include <limits>

using namespace llvm;

static uint64_t checkedAlign(uint64_t Offset, Align Alignment) {
  uint64_t Mask = Alignment.value() - 1;
  if (Offset > std::numeric_limits<uint64_t>::max() - Mask)
    report_fatal_error("SH literal pool layout overflow");
  return (Offset + Mask) & ~Mask;
}

SHLiteralPoolLayout
llvm::computeSHLiteralPoolLayout(const MachineFunction &MF) {
  SHLiteralPoolLayout Layout;
  const DataLayout &DL = MF.getDataLayout();
  const std::vector<MachineConstantPoolEntry> &Entries =
      MF.getConstantPool()->getConstants();
  uint64_t Offset = 0;

  Layout.Entries.reserve(Entries.size());
  for (const MachineConstantPoolEntry &Entry : Entries) {
    Align EntryAlignment = Entry.getAlign();
    if (EntryAlignment > Layout.Alignment)
      report_fatal_error(
          "SH literal pool entries cannot require alignment greater than four");
    unsigned Size = Entry.getSizeInBytes(DL);
    if (Size != 4)
      report_fatal_error("SH literal pool only supports four-byte entries");
    Offset = checkedAlign(Offset, EntryAlignment);
    Layout.Entries.push_back({Offset, Size, EntryAlignment});
    if (Offset > std::numeric_limits<uint64_t>::max() - Size)
      report_fatal_error("SH literal pool layout overflow");
    Offset += Size;
  }

  Layout.Size = Offset;
  return Layout;
}
