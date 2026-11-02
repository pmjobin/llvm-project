//===-- SHDSPInfo.cpp - SuperH DSP MC representation --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHDSPInfo.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/raw_ostream.h"
#include <optional>

using namespace llvm;

namespace llvm::SH {
namespace {

enum class OperationFormat : uint8_t { Fixed, Conditional };
enum class OperationArg : uint8_t { None, X, Y, N, MACH, MACL };

struct OperationInfo {
  const char *Name;
  OperationFormat Format;
  uint8_t Nibble2;
  uint8_t Nibble3;
  uint8_t Nibble4Mask;
  uint8_t Nibble4Value;
  OperationArg Args[3];
};

constexpr OperationInfo Operations[] = {
    {"psubc",
     OperationFormat::Fixed,
     0xa,
     0x0,
     0x0,
     0x0,
     {OperationArg::X, OperationArg::Y, OperationArg::N}},
    {"paddc",
     OperationFormat::Fixed,
     0xb,
     0x0,
     0x0,
     0x0,
     {OperationArg::X, OperationArg::Y, OperationArg::N}},
    {"pcmp",
     OperationFormat::Fixed,
     0x8,
     0x4,
     0x0,
     0x0,
     {OperationArg::X, OperationArg::Y, OperationArg::None}},
    {"pwsb",
     OperationFormat::Fixed,
     0xa,
     0x4,
     0x0,
     0x0,
     {OperationArg::X, OperationArg::Y, OperationArg::N}},
    {"pwad",
     OperationFormat::Fixed,
     0xb,
     0x4,
     0x0,
     0x0,
     {OperationArg::X, OperationArg::Y, OperationArg::N}},
    {"pabs",
     OperationFormat::Fixed,
     0x8,
     0x8,
     0x3,
     0x0,
     {OperationArg::X, OperationArg::N, OperationArg::None}},
    {"pabs",
     OperationFormat::Conditional,
     0x8,
     0x9,
     0x3,
     0x1,
     {OperationArg::X, OperationArg::N, OperationArg::None}},
    {"pabs",
     OperationFormat::Fixed,
     0xa,
     0x8,
     0xc,
     0x0,
     {OperationArg::Y, OperationArg::N, OperationArg::None}},
    {"pabs",
     OperationFormat::Conditional,
     0xa,
     0x9,
     0xc,
     0x4,
     {OperationArg::Y, OperationArg::N, OperationArg::None}},
    {"prnd",
     OperationFormat::Fixed,
     0x9,
     0x8,
     0x3,
     0x0,
     {OperationArg::X, OperationArg::N, OperationArg::None}},
    {"prnd",
     OperationFormat::Conditional,
     0x9,
     0x9,
     0x3,
     0x1,
     {OperationArg::X, OperationArg::N, OperationArg::None}},
    {"prnd",
     OperationFormat::Fixed,
     0xb,
     0x8,
     0xc,
     0x0,
     {OperationArg::Y, OperationArg::N, OperationArg::None}},
    {"prnd",
     OperationFormat::Conditional,
     0xb,
     0x9,
     0xc,
     0x4,
     {OperationArg::Y, OperationArg::N, OperationArg::None}},
    {"pshl",
     OperationFormat::Conditional,
     0x8,
     0x1,
     0x0,
     0x0,
     {OperationArg::X, OperationArg::Y, OperationArg::N}},
    {"psha",
     OperationFormat::Conditional,
     0x9,
     0x1,
     0x0,
     0x0,
     {OperationArg::X, OperationArg::Y, OperationArg::N}},
    {"psub",
     OperationFormat::Conditional,
     0xa,
     0x1,
     0x0,
     0x0,
     {OperationArg::X, OperationArg::Y, OperationArg::N}},
    {"psub",
     OperationFormat::Conditional,
     0x8,
     0x5,
     0x0,
     0x0,
     {OperationArg::Y, OperationArg::X, OperationArg::N}},
    {"padd",
     OperationFormat::Conditional,
     0xb,
     0x1,
     0x0,
     0x0,
     {OperationArg::X, OperationArg::Y, OperationArg::N}},
    {"pand",
     OperationFormat::Conditional,
     0x9,
     0x5,
     0x0,
     0x0,
     {OperationArg::X, OperationArg::Y, OperationArg::N}},
    {"pxor",
     OperationFormat::Conditional,
     0xa,
     0x5,
     0x0,
     0x0,
     {OperationArg::X, OperationArg::Y, OperationArg::N}},
    {"por",
     OperationFormat::Conditional,
     0xb,
     0x5,
     0x0,
     0x0,
     {OperationArg::X, OperationArg::Y, OperationArg::N}},
    {"pdec",
     OperationFormat::Conditional,
     0x8,
     0x9,
     0x3,
     0x0,
     {OperationArg::X, OperationArg::N, OperationArg::None}},
    {"pdec",
     OperationFormat::Conditional,
     0xa,
     0x9,
     0xc,
     0x0,
     {OperationArg::Y, OperationArg::N, OperationArg::None}},
    {"pinc",
     OperationFormat::Conditional,
     0x9,
     0x9,
     0x3,
     0x0,
     {OperationArg::X, OperationArg::N, OperationArg::None}},
    {"pinc",
     OperationFormat::Conditional,
     0xb,
     0x9,
     0xc,
     0x0,
     {OperationArg::Y, OperationArg::N, OperationArg::None}},
    {"pclr",
     OperationFormat::Conditional,
     0x8,
     0xd,
     0xf,
     0x0,
     {OperationArg::N, OperationArg::None, OperationArg::None}},
    {"pdmsb",
     OperationFormat::Conditional,
     0x9,
     0xd,
     0x3,
     0x0,
     {OperationArg::X, OperationArg::N, OperationArg::None}},
    {"pdmsb",
     OperationFormat::Conditional,
     0xb,
     0xd,
     0xc,
     0x0,
     {OperationArg::Y, OperationArg::N, OperationArg::None}},
    {"pneg",
     OperationFormat::Conditional,
     0xc,
     0x9,
     0x3,
     0x0,
     {OperationArg::X, OperationArg::N, OperationArg::None}},
    {"pneg",
     OperationFormat::Conditional,
     0xe,
     0x9,
     0xc,
     0x0,
     {OperationArg::Y, OperationArg::N, OperationArg::None}},
    {"pcopy",
     OperationFormat::Conditional,
     0xd,
     0x9,
     0x3,
     0x0,
     {OperationArg::X, OperationArg::N, OperationArg::None}},
    {"pcopy",
     OperationFormat::Conditional,
     0xf,
     0x9,
     0xc,
     0x0,
     {OperationArg::Y, OperationArg::N, OperationArg::None}},
    {"psts",
     OperationFormat::Conditional,
     0xc,
     0xd,
     0xf,
     0x0,
     {OperationArg::MACH, OperationArg::N, OperationArg::None}},
    {"psts",
     OperationFormat::Conditional,
     0xd,
     0xd,
     0xf,
     0x0,
     {OperationArg::MACL, OperationArg::N, OperationArg::None}},
    {"plds",
     OperationFormat::Conditional,
     0xe,
     0xd,
     0xf,
     0x0,
     {OperationArg::N, OperationArg::MACH, OperationArg::None}},
    {"plds",
     OperationFormat::Conditional,
     0xf,
     0xd,
     0xf,
     0x0,
     {OperationArg::N, OperationArg::MACL, OperationArg::None}},
    {"pswap",
     OperationFormat::Conditional,
     0x9,
     0xd,
     0x3,
     0x1,
     {OperationArg::X, OperationArg::N, OperationArg::None}},
    {"pswap",
     OperationFormat::Conditional,
     0xb,
     0xd,
     0xc,
     0x4,
     {OperationArg::Y, OperationArg::N, OperationArg::None}},
};

unsigned getDSPNibble(unsigned Reg) {
  return Reg >= 0x100 && Reg <= 0x10f ? Reg & 0xf : ~0U;
}

std::optional<unsigned> getXEncoding(unsigned Reg) {
  switch (Reg) {
  case DSP_X0:
    return 0;
  case DSP_X1:
    return 1;
  case DSP_A0:
    return 2;
  case DSP_A1:
    return 3;
  default:
    return std::nullopt;
  }
}

std::optional<unsigned> getYEncoding(unsigned Reg) {
  switch (Reg) {
  case DSP_Y0:
    return 0;
  case DSP_Y1:
    return 1;
  case DSP_M0:
    return 2;
  case DSP_M1:
    return 3;
  default:
    return std::nullopt;
  }
}

StringRef getXName(unsigned Encoding) {
  static constexpr StringLiteral Names[] = {"x0", "x1", "a0", "a1"};
  if (Encoding < 4)
    return Names[Encoding];
  return StringRef();
}

StringRef getYName(unsigned Encoding) {
  static constexpr StringLiteral Names[] = {"y0", "y1", "m0", "m1"};
  if (Encoding < 4)
    return Names[Encoding];
  return StringRef();
}

bool hasArg(const OperationInfo &Info, OperationArg Type) {
  for (OperationArg Arg : Info.Args)
    if (Arg == Type)
      return true;
  return false;
}

unsigned numArgs(const OperationInfo &Info) {
  unsigned Count = 0;
  while (Count != 3 && Info.Args[Count] != OperationArg::None)
    ++Count;
  return Count;
}

bool encodeOperationArg(OperationArg Type, const DSPArg &Arg, unsigned &X,
                        unsigned &Y, unsigned &N) {
  if (Arg.Kind != DSPArgKind::Register)
    return false;
  switch (Type) {
  case OperationArg::X:
    if (auto Encoding = getXEncoding(Arg.Reg)) {
      X = *Encoding;
      return true;
    }
    return false;
  case OperationArg::Y:
    if (auto Encoding = getYEncoding(Arg.Reg)) {
      Y = *Encoding;
      return true;
    }
    return false;
  case OperationArg::N:
    N = getDSPNibble(Arg.Reg);
    return N != ~0U && N != 6;
  case OperationArg::MACH:
    return Arg.Reg == DSP_MACH;
  case OperationArg::MACL:
    return Arg.Reg == DSP_MACL;
  case OperationArg::None:
    return false;
  }
  return false;
}

bool encodeOperation(const DSPSubInstruction &Instruction, unsigned Condition,
                     unsigned &FieldB) {
  for (const OperationInfo &Info : Operations) {
    if (Instruction.Mnemonic != Info.Name ||
        Instruction.Args.size() != numArgs(Info))
      continue;
    if (Condition && Info.Format != OperationFormat::Conditional)
      continue;
    if (!Condition && Info.Format == OperationFormat::Fixed &&
        (Info.Nibble3 & 3) != 0)
      continue;

    unsigned X = 0;
    unsigned Y = 0;
    unsigned N = 0;
    bool Matches = true;
    for (unsigned I = 0; I != Instruction.Args.size(); ++I)
      Matches &= encodeOperationArg(Info.Args[I], Instruction.Args[I], X, Y, N);
    if (!Matches)
      continue;

    unsigned Nibble4 = (X << 2) | Y;
    if ((Nibble4 & Info.Nibble4Mask) != 0)
      continue;
    Nibble4 |= Info.Nibble4Value;
    FieldB = (Info.Nibble2 << 12) + (Info.Nibble3 << 8) + Condition +
             (Nibble4 << 4) + N;
    return true;
  }
  return false;
}

bool encodeShift(const DSPSubInstruction &Instruction, unsigned Condition,
                 unsigned &FieldB) {
  if ((Instruction.Mnemonic != "pshl" && Instruction.Mnemonic != "psha") ||
      Instruction.Args.size() != 2 || Condition ||
      Instruction.Args[0].Kind != DSPArgKind::Immediate ||
      Instruction.Args[1].Kind != DSPArgKind::Register)
    return false;
  int64_t Shift = Instruction.Args[0].Immediate;
  unsigned N = getDSPNibble(Instruction.Args[1].Reg);
  if (Shift < -64 || Shift > 63 || N == ~0U || N == 6)
    return false;
  FieldB = (Instruction.Mnemonic == "psha" ? 0x1000 : 0) |
           ((static_cast<unsigned>(Shift) & 0x7f) << 4) | N;
  return true;
}

bool getEFGEncoding(const DSPSubInstruction &Instruction, unsigned &EFG) {
  if (Instruction.Mnemonic != "pmuls" || Instruction.Args.size() != 3)
    return false;
  for (const DSPArg &Arg : Instruction.Args)
    if (Arg.Kind != DSPArgKind::Register)
      return false;
  auto E = StringSwitch<std::optional<unsigned>>(
               getDSPRegisterName(Instruction.Args[0].Reg))
               .Case("x0", 0)
               .Case("x1", 1)
               .Case("y0", 2)
               .Case("a1", 3)
               .Default(std::nullopt);
  auto F = StringSwitch<std::optional<unsigned>>(
               getDSPRegisterName(Instruction.Args[1].Reg))
               .Case("y0", 0)
               .Case("y1", 1)
               .Case("x0", 2)
               .Case("a1", 3)
               .Default(std::nullopt);
  auto G = StringSwitch<std::optional<unsigned>>(
               getDSPRegisterName(Instruction.Args[2].Reg))
               .Case("m0", 0)
               .Case("m1", 1)
               .Case("a0", 2)
               .Case("a1", 3)
               .Default(std::nullopt);
  if (!E || !F || !G)
    return false;
  EFG = (*E << 10) | (*F << 8) | (*G << 2);
  return true;
}

bool encodePMUL(const DSPSubInstruction &Instruction, unsigned Condition,
                unsigned &FieldB) {
  unsigned EFG;
  if (Condition || !getEFGEncoding(Instruction, EFG))
    return false;
  if (FieldB) {
    if ((FieldB & 0xef00) == 0xa100)
      FieldB -= 0x8100;
    else if ((FieldB & 0xff00) == 0x8d00)
      FieldB -= 0x8cf0;
    else
      return false;
    switch (FieldB & 0xf) {
    case 8:
      FieldB -= 8;
      break;
    case 10:
      FieldB -= 9;
      break;
    case 7:
      FieldB -= 5;
      break;
    case 5:
      FieldB -= 2;
      break;
    default:
      return false;
    }
  }
  FieldB += 0x4000 + EFG;
  return true;
}

bool encodeSingleTransfer(const DSPSubInstruction &Instruction,
                          uint32_t &Encoding, std::string &Error) {
  if ((Instruction.Mnemonic != "movs.w" && Instruction.Mnemonic != "movs.l") ||
      Instruction.Args.size() != 2) {
    Error = "invalid DSP single-data-transfer instruction";
    return false;
  }
  bool IsLong = Instruction.Mnemonic == "movs.l";
  bool IsLoad = Instruction.Args[0].Kind == DSPArgKind::Memory;
  const DSPArg &Memory = Instruction.Args[IsLoad ? 0 : 1];
  const DSPArg &Data = Instruction.Args[IsLoad ? 1 : 0];
  if (Memory.Kind != DSPArgKind::Memory || Data.Kind != DSPArgKind::Register ||
      Memory.Reg < DSP_R2 || Memory.Reg > DSP_R5) {
    Error = "invalid operands for DSP single-data transfer";
    return false;
  }
  if (Memory.Update == DSPUpdate::Modulo && Memory.IndexReg != DSP_R8) {
    Error = "invalid modulo index register for DSP single-data transfer";
    return false;
  }
  unsigned DataReg = getDSPNibble(Data.Reg);
  if (DataReg == ~0U || DataReg == 6) {
    Error = "invalid DSP data register";
    return false;
  }
  unsigned Mode;
  switch (Memory.Update) {
  case DSPUpdate::PreDecrement:
    Mode = 0;
    break;
  case DSPUpdate::None:
    Mode = 4;
    break;
  case DSPUpdate::PostIncrement:
    Mode = 8;
    break;
  case DSPUpdate::Modulo:
    Mode = 12;
    break;
  }
  unsigned Base = (Memory.Reg & 3) | 4;
  unsigned Suffix = Mode | (!IsLoad ? 1 : 0) | (IsLong ? 2 : 0);
  Encoding = 0xf000 | (Base << 8) | (DataReg << 4) | Suffix;
  return true;
}

std::optional<unsigned> getAXEncoding(unsigned Reg) {
  switch (Reg) {
  case DSP_A0:
    return 0;
  case DSP_X0:
    return 1;
  case DSP_A1:
    return 2;
  case DSP_X1:
    return 3;
  default:
    return std::nullopt;
  }
}

std::optional<unsigned> getXYEncoding(unsigned Reg) {
  switch (Reg) {
  case DSP_X0:
    return 0;
  case DSP_Y0:
    return 1;
  case DSP_X1:
    return 2;
  case DSP_Y1:
    return 3;
  default:
    return std::nullopt;
  }
}

std::optional<unsigned> getAYEncoding(unsigned Reg) {
  switch (Reg) {
  case DSP_A0:
    return 0;
  case DSP_A1:
    return 1;
  case DSP_Y0:
    return 2;
  case DSP_Y1:
    return 3;
  default:
    return std::nullopt;
  }
}

std::optional<unsigned> getYXEncoding(unsigned Reg) {
  switch (Reg) {
  case DSP_Y0:
    return 0;
  case DSP_Y1:
    return 1;
  case DSP_X0:
    return 2;
  case DSP_X1:
    return 3;
  default:
    return std::nullopt;
  }
}

bool encodeMove(const DSPSubInstruction &Instruction, unsigned &MovX,
                unsigned &MovY, std::string &Error) {
  bool IsX =
      Instruction.Mnemonic == "movx.w" || Instruction.Mnemonic == "movx.l";
  bool IsY =
      Instruction.Mnemonic == "movy.w" || Instruction.Mnemonic == "movy.l";
  if ((!IsX && !IsY) || Instruction.Args.size() != 2)
    return false;
  unsigned &Move = IsX ? MovX : MovY;
  if (Move) {
    Error =
        IsX ? "multiple movx specifications" : "multiple movy specifications";
    return false;
  }
  bool IsLong = StringRef(Instruction.Mnemonic).ends_with(".l");
  bool IsLoad = Instruction.Args[0].Kind == DSPArgKind::Memory;
  const DSPArg &Memory = Instruction.Args[IsLoad ? 0 : 1];
  const DSPArg &Data = Instruction.Args[IsLoad ? 1 : 0];
  if (Memory.Kind != DSPArgKind::Memory || Data.Kind != DSPArgKind::Register ||
      Memory.Update == DSPUpdate::PreDecrement) {
    Error = "invalid operands for DSP double-data transfer";
    return false;
  }
  unsigned RequiredIndex = IsX ? DSP_R8 : DSP_R9;
  if (Memory.Update == DSPUpdate::Modulo && Memory.IndexReg != RequiredIndex) {
    Error = "invalid modulo index register for DSP double-data transfer";
    return false;
  }
  unsigned Mode;
  switch (Memory.Update) {
  case DSPUpdate::None:
    Mode = 1;
    break;
  case DSPUpdate::PostIncrement:
    Mode = 2;
    break;
  case DSPUpdate::Modulo:
    Mode = 3;
    break;
  case DSPUpdate::PreDecrement:
    return false;
  }

  bool UseOld = !IsLong;
  if (IsX)
    UseOld &= Memory.Reg >= DSP_R4 && Memory.Reg <= DSP_R5 &&
              (IsLoad ? (Data.Reg == DSP_X0 || Data.Reg == DSP_X1)
                      : (Data.Reg == DSP_A0 || Data.Reg == DSP_A1));
  else
    UseOld &= Memory.Reg >= DSP_R6 && Memory.Reg <= DSP_R7 && IsLoad &&
              (Data.Reg == DSP_Y0 || Data.Reg == DSP_Y1);

  if (UseOld) {
    unsigned DataBit = Data.Reg == (IsLoad ? (IsX ? DSP_X1 : DSP_Y1) : DSP_A1);
    unsigned Opcode = Mode | (!IsLoad ? 8 : 0);
    if (IsX) {
      if (MovY & 0x2ac) {
        Error = "movx requires nopy";
        return false;
      }
      Move = 0xf000 | ((Memory.Reg - DSP_R4) << 9) | (DataBit << 7) |
             (Opcode << 2);
    } else {
      if (MovX & 0x153) {
        Error = "movy requires nopx";
        return false;
      }
      Move = 0xf000 | ((Memory.Reg - DSP_R6) << 8) | (DataBit << 6) | Opcode;
    }
    return true;
  }

  if (IsX) {
    if (!((Memory.Reg <= DSP_R1) ||
          (Memory.Reg >= DSP_R4 && Memory.Reg <= DSP_R5))) {
      Error = "invalid movx address register";
      return false;
    }
    auto DataEncoding =
        IsLoad ? getXYEncoding(Data.Reg) : getAXEncoding(Data.Reg);
    if (!DataEncoding) {
      Error = "invalid movx DSP register";
      return false;
    }
    if (MovY & 0x2ac) {
      Error = "movx cannot be combined with non-nopy";
      return false;
    }
    unsigned Form = (IsLong ? 1 : 0) | (!IsLoad ? 2 : 0);
    Move = 0xf000 | ((Memory.Reg & 1) << 9) | ((Memory.Reg < DSP_R4) << 8) |
           (*DataEncoding << 6) | (Form << 4) | (Mode << 2);
  } else {
    if (!((Memory.Reg >= DSP_R2 && Memory.Reg <= DSP_R3) ||
          (Memory.Reg >= DSP_R6 && Memory.Reg <= DSP_R7))) {
      Error = "invalid movy address register";
      return false;
    }
    auto DataEncoding =
        IsLoad ? getYXEncoding(Data.Reg) : getAYEncoding(Data.Reg);
    if (!DataEncoding) {
      Error = "invalid movy DSP register";
      return false;
    }
    if (MovX & 0x153) {
      Error = "movy cannot be combined with non-nopx";
      return false;
    }
    unsigned Form = (IsLong ? 2 : 0) | (!IsLoad ? 1 : 0);
    Move = 0xf000 | ((Memory.Reg & 1) << 8) | ((Memory.Reg < DSP_R6) << 9) |
           (*DataEncoding << 6) | (Form << 4) | Mode;
  }
  return true;
}

void printMemory(unsigned Reg, DSPUpdate Update, unsigned ModuloReg,
                 raw_ostream &OS) {
  OS << '@';
  if (Update == DSPUpdate::PreDecrement)
    OS << '-';
  OS << 'r' << Reg;
  if (Update == DSPUpdate::PostIncrement)
    OS << '+';
  else if (Update == DSPUpdate::Modulo)
    OS << "+r" << ModuloReg;
}

bool printSingleTransfer(uint16_t Insn, raw_ostream &OS) {
  unsigned BaseEncoding = (Insn >> 8) & 0xf;
  unsigned Data = (Insn >> 4) & 0xf;
  unsigned Suffix = Insn & 0xf;
  if (BaseEncoding < 4 || BaseEncoding > 7 ||
      getDSPRegisterName(0x100 | Data).empty())
    return false;
  static constexpr unsigned Bases[] = {4, 5, 2, 3};
  unsigned Base = Bases[BaseEncoding - 4];
  bool IsStore = Suffix & 1;
  bool IsLong = Suffix & 2;
  DSPUpdate Update;
  switch (Suffix & 0xc) {
  case 0x0:
    Update = DSPUpdate::PreDecrement;
    break;
  case 0x4:
    Update = DSPUpdate::None;
    break;
  case 0x8:
    Update = DSPUpdate::PostIncrement;
    break;
  case 0xc:
    Update = DSPUpdate::Modulo;
    break;
  default:
    return false;
  }
  OS << "movs." << (IsLong ? 'l' : 'w') << '\t';
  if (IsStore)
    OS << getDSPRegisterName(0x100 | Data) << ',';
  printMemory(Base, Update, DSP_R8, OS);
  if (!IsStore)
    OS << ',' << getDSPRegisterName(0x100 | Data);
  return true;
}

void printNewMove(bool IsY, unsigned Bits, raw_ostream &OS) {
  unsigned Form = (Bits >> 4) & 3;
  unsigned Mode = IsY ? Bits & 3 : (Bits >> 2) & 3;
  unsigned EncodedReg = (Bits >> 6) & 3;
  bool IsStore = IsY ? (Form & 1) : (Form & 2);
  bool IsLong = IsY ? (Form & 2) : (Form & 1);
  unsigned Base = 4 * ((Bits & (IsY ? 0x200 : 0x100)) == 0) + 2 * IsY +
                  ((Bits & (IsY ? 0x100 : 0x200)) != 0);
  StringRef Data;
  if (IsY) {
    static constexpr StringLiteral Load[] = {"y0", "y1", "x0", "x1"};
    static constexpr StringLiteral Store[] = {"a0", "a1", "y0", "y1"};
    Data = IsStore ? Store[EncodedReg] : Load[EncodedReg];
  } else {
    static constexpr StringLiteral Load[] = {"x0", "y0", "x1", "y1"};
    static constexpr StringLiteral Store[] = {"a0", "x0", "a1", "x1"};
    Data = IsStore ? Store[EncodedReg] : Load[EncodedReg];
  }
  DSPUpdate Update = Mode == 1   ? DSPUpdate::None
                     : Mode == 2 ? DSPUpdate::PostIncrement
                                 : DSPUpdate::Modulo;
  OS << (IsY ? "movy." : "movx.") << (IsLong ? 'l' : 'w') << '\t';
  if (IsStore)
    OS << Data << ',';
  printMemory(Base, Update, IsY ? DSP_R9 : DSP_R8, OS);
  if (!IsStore)
    OS << ',' << Data;
}

bool printDoubleTransfer(unsigned Bits, raw_ostream &OS) {
  if (Bits == 0) {
    OS << "nopx\tnopy";
    return true;
  }
  if (Bits == 0x800)
    return true;
  if ((Bits & 0x800) && (Bits & 0x3ff))
    OS << '\t';
  bool Printed = false;
  bool NewY = (Bits & 3) != 0 && (Bits & 0xc) == 0 && (Bits & 0x2a0);
  bool NewX = (Bits & 3) == 0 && (Bits & 0xc) != 0 && (Bits & 0x150);
  if (NewX || NewY) {
    unsigned Form = (Bits >> 4) & 3;
    unsigned Mode = NewY ? Bits & 3 : (Bits >> 2) & 3;
    if (!Mode || Mode > 3 || Form > 3)
      return false;
    printNewMove(NewY, Bits, OS);
    return true;
  }
  unsigned XOpcode = (Bits >> 2) & 0xb;
  if (XOpcode) {
    if (XOpcode != 1 && XOpcode != 2 && XOpcode != 3 && XOpcode != 9 &&
        XOpcode != 10 && XOpcode != 11)
      return false;
    bool Store = XOpcode & 8;
    unsigned Mode = XOpcode & 3;
    unsigned Base = ((Bits >> 9) & 1) + 4;
    unsigned Data = (Bits >> 7) & 1;
    OS << "movx.w\t";
    if (Store)
      OS << 'a' << Data << ',';
    printMemory(Base,
                Mode == 1   ? DSPUpdate::None
                : Mode == 2 ? DSPUpdate::PostIncrement
                            : DSPUpdate::Modulo,
                DSP_R8, OS);
    if (!Store)
      OS << ",x" << Data;
    Printed = true;
  }
  unsigned YOpcode = (Bits & 3) | ((Bits >> 1) & 8);
  if (YOpcode) {
    if (YOpcode != 1 && YOpcode != 2 && YOpcode != 3 && YOpcode != 9 &&
        YOpcode != 10 && YOpcode != 11)
      return false;
    if (Printed)
      OS << '\t';
    bool Store = YOpcode & 8;
    unsigned Mode = YOpcode & 3;
    unsigned Base = ((Bits >> 8) & 1) + 6;
    unsigned Data = (Bits >> 6) & 1;
    OS << "movy.w\t";
    if (Store)
      OS << 'a' << Data << ',';
    printMemory(Base,
                Mode == 1   ? DSPUpdate::None
                : Mode == 2 ? DSPUpdate::PostIncrement
                            : DSPUpdate::Modulo,
                DSP_R9, OS);
    if (!Store)
      OS << ",y" << Data;
    Printed = true;
  }
  return Printed;
}

void printOperationArg(OperationArg Type, unsigned FieldB, raw_ostream &OS) {
  switch (Type) {
  case OperationArg::X:
    OS << getXName((FieldB >> 6) & 3);
    break;
  case OperationArg::Y:
    OS << getYName((FieldB >> 4) & 3);
    break;
  case OperationArg::N:
    OS << getDSPRegisterName(0x100 | (FieldB & 0xf));
    break;
  case OperationArg::MACH:
    OS << "mach";
    break;
  case OperationArg::MACL:
    OS << "macl";
    break;
  case OperationArg::None:
    break;
  }
}

bool printPMUL(unsigned FieldB, raw_ostream &OS) {
  if ((FieldB & 0xc000) != 0x4000 || (FieldB & 0x3000) == 0x1000)
    return false;
  static constexpr StringLiteral DU[] = {"x0", "y0", "a0", "a1"};
  static constexpr StringLiteral E[] = {"x0", "x1", "y0", "a1"};
  static constexpr StringLiteral F[] = {"y0", "y1", "x0", "a1"};
  static constexpr StringLiteral G[] = {"m0", "m1", "a0", "a1"};
  if (FieldB & 0x2000)
    OS << ((FieldB & 0x1000) ? "padd\t" : "psub\t")
       << getXName((FieldB >> 6) & 3) << ',' << getYName((FieldB >> 4) & 3)
       << ',' << DU[FieldB & 3] << '\t';
  else if ((FieldB & 0xf0) == 0x10)
    OS << "pclr\t" << DU[FieldB & 3] << '\t';
  else if ((FieldB & 0xf3) != 0)
    return false;
  OS << "pmuls\t" << E[(FieldB >> 10) & 3] << ',' << F[(FieldB >> 8) & 3] << ','
     << G[(FieldB >> 2) & 3];
  return true;
}

bool printFieldB(unsigned FieldB, raw_ostream &OS) {
  if ((FieldB & 0xe800) == 0) {
    StringRef Dest = getDSPRegisterName(0x100 | (FieldB & 0xf));
    if (Dest.empty() || (FieldB & 0xf) == 6)
      return false;
    int Shift = (FieldB >> 4) & 0x7f;
    if (Shift & 0x40)
      Shift -= 0x80;
    OS << ((FieldB & 0x1000) ? "psha\t#" : "pshl\t#") << Shift << ',' << Dest;
    return true;
  }
  if ((FieldB & 0xc000) == 0x4000)
    return printPMUL(FieldB, OS);

  unsigned Nibble2 = (FieldB >> 12) & 0xf;
  unsigned Nibble3 = (FieldB >> 8) & 0xf;
  unsigned Nibble4 = (FieldB >> 4) & 0xf;
  unsigned Condition = Nibble3 & 3;
  OperationFormat Format;
  StringRef Prefix;
  if (Condition == 0) {
    Format = OperationFormat::Fixed;
  } else {
    Format = OperationFormat::Conditional;
    if (Condition == 2) {
      Prefix = "dct ";
      --Nibble3;
    } else if (Condition == 3) {
      Prefix = "dcf ";
      Nibble3 -= 2;
    }
  }
  for (const OperationInfo &Info : Operations) {
    if (Info.Format != Format || Info.Nibble2 != Nibble2 ||
        Info.Nibble3 != Nibble3 ||
        (Nibble4 & Info.Nibble4Mask) != Info.Nibble4Value)
      continue;
    unsigned RequiredMask = Info.Nibble4Mask;
    if (!hasArg(Info, OperationArg::X))
      RequiredMask |= 0xc;
    if (!hasArg(Info, OperationArg::Y))
      RequiredMask |= 0x3;
    if ((Nibble4 & RequiredMask) != Info.Nibble4Value)
      continue;
    if (!hasArg(Info, OperationArg::N) && (FieldB & 0xf))
      continue;
    if (hasArg(Info, OperationArg::N) &&
        getDSPRegisterName(0x100 | (FieldB & 0xf)).empty())
      continue;
    OS << Prefix << Info.Name << '\t';
    for (unsigned I = 0; I != numArgs(Info); ++I) {
      if (I)
        OS << ',';
      printOperationArg(Info.Args[I], FieldB, OS);
    }
    return true;
  }
  return false;
}

bool renderDSPInstruction(uint32_t Encoding, unsigned Size, raw_ostream &OS) {
  if (Size == 2) {
    uint16_t Insn = Encoding;
    if ((Insn & 0xfc00) == 0xf400)
      return printSingleTransfer(Insn, OS);
    if ((Insn & 0xfc00) == 0xf000)
      return printDoubleTransfer(Insn & 0xfff, OS);
    return false;
  }
  if (Size != 4 || (Encoding & 0xfc000000) != 0xf8000000)
    return false;
  SmallString<128> FieldText;
  raw_svector_ostream FieldOS(FieldText);
  if (!printFieldB(Encoding & 0xffff, FieldOS))
    return false;
  SmallString<128> TransferText;
  raw_svector_ostream TransferOS(TransferText);
  if (!printDoubleTransfer((Encoding >> 16) & 0xfff, TransferOS))
    return false;
  OS << FieldText << TransferText;
  return true;
}

} // namespace

bool isDSPCompoundMnemonic(StringRef Mnemonic) {
  if (Mnemonic == "movs.w" || Mnemonic == "movs.l" || Mnemonic == "movx.w" ||
      Mnemonic == "movx.l" || Mnemonic == "movy.w" || Mnemonic == "movy.l" ||
      Mnemonic == "nopx" || Mnemonic == "nopy" || Mnemonic == "dct" ||
      Mnemonic == "dcf" || Mnemonic == "pmuls")
    return true;
  for (const OperationInfo &Info : Operations)
    if (Mnemonic == Info.Name)
      return true;
  return false;
}

bool isDSPParallelOperationMnemonic(StringRef Mnemonic) {
  if (Mnemonic == "dct" || Mnemonic == "dcf" || Mnemonic == "pmuls")
    return true;
  for (const OperationInfo &Info : Operations)
    if (Mnemonic == Info.Name)
      return true;
  return false;
}

bool getDSPRegister(StringRef Name, unsigned &Reg) {
  std::string Lower = Name.lower();
  auto Value = StringSwitch<std::optional<unsigned>>(Lower)
                   .Case("a0", DSP_A0)
                   .Case("a1", DSP_A1)
                   .Case("a0g", DSP_A0G)
                   .Case("a1g", DSP_A1G)
                   .Case("x0", DSP_X0)
                   .Case("x1", DSP_X1)
                   .Case("y0", DSP_Y0)
                   .Case("y1", DSP_Y1)
                   .Case("m0", DSP_M0)
                   .Case("m1", DSP_M1)
                   .Case("mach", DSP_MACH)
                   .Case("macl", DSP_MACL)
                   .Default(std::nullopt);
  if (Value) {
    Reg = *Value;
    return true;
  }
  StringRef LowerRef(Lower);
  if (LowerRef.size() < 2 || LowerRef[0] != 'r')
    return false;
  unsigned Number;
  if (LowerRef.drop_front().getAsInteger(10, Number) || Number > 15)
    return false;
  Reg = Number;
  return true;
}

StringRef getDSPRegisterName(unsigned Reg) {
  switch (Reg) {
  case DSP_A1:
    return "a1";
  case DSP_A0:
    return "a0";
  case DSP_X0:
    return "x0";
  case DSP_X1:
    return "x1";
  case DSP_Y0:
    return "y0";
  case DSP_Y1:
    return "y1";
  case DSP_M0:
    return "m0";
  case DSP_A1G:
    return "a1g";
  case DSP_M1:
    return "m1";
  case DSP_A0G:
    return "a0g";
  case DSP_MACH:
    return "mach";
  case DSP_MACL:
    return "macl";
  default:
    return StringRef();
  }
}

bool encodeDSPInstruction(ArrayRef<DSPSubInstruction> Instructions,
                          uint32_t &Encoding, unsigned &Size,
                          std::string &Error) {
  if (Instructions.empty()) {
    Error = "empty DSP instruction";
    return false;
  }
  if (Instructions.front().Mnemonic == "movs.w" ||
      Instructions.front().Mnemonic == "movs.l") {
    if (Instructions.size() != 1) {
      Error = "movs cannot be combined with a parallel instruction";
      return false;
    }
    Size = 2;
    return encodeSingleTransfer(Instructions.front(), Encoding, Error);
  }

  unsigned MovX = 0;
  unsigned MovY = 0;
  unsigned Condition = 0;
  unsigned FieldB = 0;
  for (const DSPSubInstruction &Instruction : Instructions) {
    if (Instruction.Mnemonic == "nopx" || Instruction.Mnemonic == "nopy") {
      unsigned &Move = Instruction.Mnemonic == "nopx" ? MovX : MovY;
      if (!Instruction.Args.empty() || Move) {
        Error = Instruction.Mnemonic == "nopx" ? "multiple movx specifications"
                                               : "multiple movy specifications";
        return false;
      }
      Move = 0xf000;
      continue;
    }
    if (StringRef(Instruction.Mnemonic).starts_with("movx.") ||
        StringRef(Instruction.Mnemonic).starts_with("movy.")) {
      if (!encodeMove(Instruction, MovX, MovY, Error))
        return false;
      continue;
    }
    if (Instruction.Mnemonic == "dct" || Instruction.Mnemonic == "dcf") {
      if (!Instruction.Args.empty() || Condition) {
        Error = "multiple DSP condition specifications";
        return false;
      }
      Condition = Instruction.Mnemonic == "dct" ? 0x100 : 0x200;
      continue;
    }
    if (Instruction.Mnemonic == "pmuls") {
      if (!encodePMUL(Instruction, Condition, FieldB)) {
        Error = "pmuls cannot be combined with this DSP operation";
        return false;
      }
      Condition = 0;
      continue;
    }
    if (FieldB) {
      Error = "multiple parallel-processing operations";
      return false;
    }
    if (!encodeShift(Instruction, Condition, FieldB) &&
        !encodeOperation(Instruction, Condition, FieldB)) {
      Error = "invalid operands for DSP operation";
      return false;
    }
    Condition = 0;
  }
  if (Condition) {
    Error = "condition is not followed by a conditional DSP operation";
    return false;
  }
  if (!FieldB) {
    if (!MovX && !MovY) {
      Error = "DSP instruction has no operation";
      return false;
    }
    Encoding = MovX | MovY;
    Size = 2;
    return true;
  }
  Encoding = ((MovX | MovY | 0xf800) << 16) | FieldB;
  Size = 4;
  return true;
}

bool isValidDSPInstruction(uint32_t Encoding, unsigned Size) {
  raw_null_ostream OS;
  return renderDSPInstruction(Encoding, Size, OS);
}

bool printDSPInstruction(uint32_t Encoding, unsigned Size, raw_ostream &OS) {
  SmallString<256> Buffer;
  raw_svector_ostream BufferOS(Buffer);
  if (!renderDSPInstruction(Encoding, Size, BufferOS))
    return false;
  OS << Buffer;
  return true;
}

} // namespace llvm::SH
