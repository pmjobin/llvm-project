//===-- SHDSPInfo.h - SuperH DSP MC representation -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_MCTARGETDESC_SHDSPINFO_H
#define LLVM_LIB_TARGET_SH_MCTARGETDESC_SHDSPINFO_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include <cstdint>
#include <string>

namespace llvm {

class raw_ostream;

namespace SH {

enum class DSPArgKind : uint8_t { Register, Immediate, Memory };
enum class DSPUpdate : uint8_t { None, PreDecrement, PostIncrement, Modulo };

enum DSPRegister : unsigned {
  DSP_R0,
  DSP_R1,
  DSP_R2,
  DSP_R3,
  DSP_R4,
  DSP_R5,
  DSP_R6,
  DSP_R7,
  DSP_R8,
  DSP_R9,
  DSP_R10,
  DSP_R11,
  DSP_R12,
  DSP_R13,
  DSP_R14,
  DSP_R15,
  DSP_A1 = 0x105,
  DSP_A0 = 0x107,
  DSP_X0 = 0x108,
  DSP_X1 = 0x109,
  DSP_Y0 = 0x10a,
  DSP_Y1 = 0x10b,
  DSP_M0 = 0x10c,
  DSP_A1G = 0x10d,
  DSP_M1 = 0x10e,
  DSP_A0G = 0x10f,
  DSP_MACH = 0x200,
  DSP_MACL = 0x201,
};

struct DSPArg {
  DSPArgKind Kind = DSPArgKind::Register;
  unsigned Reg = 0;
  unsigned IndexReg = 0;
  int64_t Immediate = 0;
  DSPUpdate Update = DSPUpdate::None;
};

struct DSPSubInstruction {
  std::string Mnemonic;
  SmallVector<DSPArg, 3> Args;
};

bool isDSPCompoundMnemonic(StringRef Mnemonic);
bool isDSPParallelOperationMnemonic(StringRef Mnemonic);
bool getDSPRegister(StringRef Name, unsigned &Reg);
StringRef getDSPRegisterName(unsigned Reg);
bool encodeDSPInstruction(ArrayRef<DSPSubInstruction> Instructions,
                          uint32_t &Encoding, unsigned &Size,
                          std::string &Error);
bool isValidDSPInstruction(uint32_t Encoding, unsigned Size);
bool printDSPInstruction(uint32_t Encoding, unsigned Size, raw_ostream &OS);

} // namespace SH
} // namespace llvm

#endif
