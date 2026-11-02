//===-- SHFixupKinds.h - SH-specific fixup kinds --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_MCTARGETDESC_SHFIXUPKINDS_H
#define LLVM_LIB_TARGET_SH_MCTARGETDESC_SHFIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace SH {

enum Fixups {
  fixup_SH_PCREL8_2 = FirstTargetFixupKind,
  fixup_SH_PCREL12_2,
  fixup_SH_BSR12_2,
  fixup_SH_PCREL8_4,
  fixup_SH_PCREL8_2_LITERAL,
  fixup_SH_LOOP_START,
  fixup_SH_LOOP_END,
  fixup_SH_MOVI20,

  LastTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};

} // namespace SH
} // namespace llvm

#endif
