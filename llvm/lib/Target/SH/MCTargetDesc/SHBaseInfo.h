//===-- SHBaseInfo.h - SH architectural encoding facts --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_MCTARGETDESC_SHBASEINFO_H
#define LLVM_LIB_TARGET_SH_MCTARGETDESC_SHBASEINFO_H

#include "llvm/Support/MathExtras.h"
#include <cstdint>

namespace llvm::SH {

constexpr unsigned InstructionAlignment = 2;
constexpr unsigned NopSize = 2;
constexpr unsigned BranchInstructionSize = 2;
constexpr unsigned PCRelativeLiteralInstructionSize = 2;
constexpr int64_t PCRelativeBias = 4;
constexpr int64_t BranchDisplacementScale = 2;
constexpr unsigned ConditionalBranchDisplacementBits = 8;
constexpr unsigned UnconditionalBranchDisplacementBits = 12;
constexpr int64_t PCRelativeWordScale = 2;
constexpr int64_t PCRelativeWordMaxDisplacement = 255 * PCRelativeWordScale;
constexpr int64_t PCRelativeLongScale = 4;
constexpr int64_t PCRelativeLongMaxDisplacement = 255 * PCRelativeLongScale;

inline bool isBranchDisplacementInRange(int64_t ByteDisplacement,
                                        unsigned Width) {
  return ByteDisplacement % BranchDisplacementScale == 0 &&
         isIntN(Width, ByteDisplacement / BranchDisplacementScale);
}

inline bool isBranchOffsetInRange(int64_t ByteOffset, unsigned Width) {
  return isBranchDisplacementInRange(ByteOffset - PCRelativeBias, Width);
}

inline bool isPCRelativeWordDisplacementInRange(int64_t ByteDisplacement) {
  return ByteDisplacement >= 0 &&
         ByteDisplacement <= PCRelativeWordMaxDisplacement &&
         ByteDisplacement % PCRelativeWordScale == 0;
}

inline bool isPCRelativeLongDisplacementInRange(int64_t ByteDisplacement) {
  return ByteDisplacement >= 0 &&
         ByteDisplacement <= PCRelativeLongMaxDisplacement &&
         ByteDisplacement % PCRelativeLongScale == 0;
}

inline uint64_t getPCRelativeLongBase(uint64_t InstructionAddress) {
  return alignDown(InstructionAddress,
                   static_cast<uint64_t>(PCRelativeLongScale)) +
         PCRelativeBias;
}

} // namespace llvm::SH

#endif
