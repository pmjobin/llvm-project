//===-- SHLiteralPool.h - SH inline literal-island helpers ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHLITERALPOOL_H
#define LLVM_LIB_TARGET_SH_SHLITERALPOOL_H

#include <cstdint>

namespace llvm {

class DataLayout;
class MachineBasicBlock;
class MCContext;
class MCSymbol;

constexpr int64_t SHUnassignedLiteralIsland = -1;
constexpr unsigned SHLiteralIslandAlignment = 4;
constexpr unsigned SHLiteralIslandEntrySize = 4;
constexpr unsigned SHLiteralIslandMaxPayload = 1020;
constexpr unsigned SHLiteralLoadMaxDistance = 1020;

bool isSHLiteralIslandBlock(const MachineBasicBlock &MBB);

MCSymbol *getSHLiteralIslandSymbol(MCContext &Ctx, const DataLayout &DL,
                                   unsigned FunctionNumber, unsigned CPI,
                                   unsigned Instance);

} // namespace llvm

#endif
