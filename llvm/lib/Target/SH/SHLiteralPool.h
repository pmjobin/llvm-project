//===-- SHLiteralPool.h - SH inline literal-island helpers ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHLITERALPOOL_H
#define LLVM_LIB_TARGET_SH_SHLITERALPOOL_H

#include "MCTargetDesc/SHBaseInfo.h"
#include <cstdint>

namespace llvm {

class DataLayout;
class MachineBasicBlock;
class MachineFunction;
class MCContext;
class MCSymbol;

constexpr int64_t SHUnassignedLiteralIsland = -1;
constexpr unsigned SHLiteralIslandMinAlignment = 2;
constexpr unsigned SHLiteralIslandAlignment = 4;
constexpr unsigned SHWordLiteralIslandEntrySize = 2;
constexpr unsigned SHLongLiteralIslandEntrySize = 4;
constexpr unsigned SHDoubleLiteralIslandEntrySize = 8;
constexpr unsigned SHLiteralIslandMaxPayload =
    SH::PCRelativeLongMaxDisplacement;
constexpr unsigned SHWordLiteralLoadMaxDistance =
    SH::PCRelativeWordMaxDisplacement;
constexpr unsigned SHLongLiteralLoadMaxDistance =
    SH::PCRelativeLongMaxDisplacement;

bool isSHLiteralIslandBlock(const MachineBasicBlock &MBB);
bool isSHJumpTableBlock(const MachineBasicBlock &MBB);
bool isSHDataIslandBlock(const MachineBasicBlock &MBB);
bool coalesceSHLongLiteralEntries(MachineFunction &MF);

MCSymbol *getSHLiteralIslandSymbol(MCContext &Ctx, const DataLayout &DL,
                                   unsigned FunctionNumber, unsigned CPI,
                                   unsigned Instance);

} // namespace llvm

#endif
