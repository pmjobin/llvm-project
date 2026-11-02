//===-- SHInlineAsmSize.h - SH inline assembly sizing ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHINLINEASMSIZE_H
#define LLVM_LIB_TARGET_SH_SHINLINEASMSIZE_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include <cstdint>
#include <optional>
#include <string>

namespace llvm {

enum class SHInlineAsmSizeKind {
  Exact,
  Conservative,
  Indeterminate,
  Malformed,
};

struct SHInlineAsmSize {
  uint64_t Bytes = 0;
  SHInlineAsmSizeKind Kind = SHInlineAsmSizeKind::Exact;
  std::string Diagnostic;
};

ArrayRef<StringLiteral> getSHInlineAsmInstructionMnemonics();
std::optional<std::string>
selectSHInlineAsmAlternative(StringRef Asm, std::string &Diagnostic);
SHInlineAsmSize getSHInlineAsmSize(StringRef Asm);

} // namespace llvm

#endif
