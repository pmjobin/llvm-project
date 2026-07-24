//===-- SHTargetInfo.cpp - SH Target Implementation ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

Target &llvm::getTheSHTarget() {
  static Target TheSHTarget;
  return TheSHTarget;
}

Target &llvm::getTheSHLETarget() {
  static Target TheSHLETarget;
  return TheSHLETarget;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeSHTargetInfo() {
  RegisterTarget<Triple::sh, /*HasJIT=*/false> X(
      getTheSHTarget(), "sh", "SuperH (32-bit big endian)", "SH");
  RegisterTarget<Triple::shle, /*HasJIT=*/false> Y(
      getTheSHLETarget(), "shle", "SuperH (32-bit little endian)", "SH");
}

// Targets.def requires these entry points for every configured target family.
// SH intentionally has no code generation to initialize yet.
extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeSHTarget() {}
