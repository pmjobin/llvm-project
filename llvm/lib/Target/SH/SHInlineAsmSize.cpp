//===-- SHInlineAsmSize.cpp - SH inline assembly sizing ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHInlineAsmSize.h"
#include "MCTargetDesc/SHDSPInfo.h"
#include "MCTargetDesc/SHMCAsmInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include <cctype>
#include <limits>

using namespace llvm;

namespace {

// Bound both directive nesting and recursive constant-expression evaluation
// with one parser-wide policy. This keeps the maximum native call depth
// independent of the input size and gives every recursive entry point the same
// exact boundary.
constexpr unsigned MaxParserNestingDepth = 64;

// This list mirrors every instruction mnemonic in the TableGen SH assembly
// matcher. SHInlineAsmSizeTest mechanically compares it with the MC instruction
// and assembly-writer inventory so additions cannot be missed. Width is
// classified independently below because SH-2A has overloaded 16/32-bit forms.
constexpr StringLiteral SHInstructionMnemonics[] = {
    "add",       "addc",     "addv",    "and",      "and.b",   "band.b",
    "bandnot.b", "bclr",     "bclr.b",  "bf",       "bf/s",    "bld",
    "bld.b",     "bldnot.b", "bor.b",   "bornot.b", "bra",     "braf",
    "bset",      "bset.b",   "bsr",     "bsrf",     "bst",     "bst.b",
    "bt",        "bt/s",     "bxor.b",  "clips.b",  "clips.w", "clipu.b",
    "clipu.w",   "clrdmxy",  "clrmac",  "clrs",     "clrt",    "cmp/eq",
    "cmp/ge",    "cmp/gt",   "cmp/hi",  "cmp/hs",   "cmp/pl",  "cmp/pz",
    "cmp/str",   "div0s",    "div0u",   "div1",     "divs",    "divu",
    "dmuls.l",   "dmulu.l",  "dsp.ddt", "dsp.ppi",  "dsp.sdt", "dt",
    "exts.b",    "exts.w",   "extu.b",  "extu.w",   "fabs",    "fadd",
    "fcmp/eq",   "fcmp/gt",  "fcnvds",  "fcnvsd",   "fdiv",    "fipr",
    "fldi0",     "fldi1",    "flds",    "float",    "fmac",    "fmov",
    "fmov.d",    "fmov.s",   "fmul",    "fneg",     "fpchg",   "frchg",
    "fsca",      "fschg",    "fsqrt",   "fsrra",    "fsts",    "fsub",
    "ftrc",      "ftrv",     "icbi",    "jmp",      "jsr",     "jsr/n",
    "ldbank",    "ldc",      "ldc.l",   "ldrc",     "ldre",    "ldrs",
    "ldtlb",     "lds",      "lds.l",   "mac.l",    "mac.w",   "mov",
    "mov.b",     "mov.l",    "mov.w",   "mova",     "movca.l", "movco.l",
    "movi20",    "movi20s",  "movli.l", "movml.l",  "movmu.l", "movrt",
    "movt",      "movu.b",   "movu.w",  "movua.l",  "mul.l",   "mulr",
    "muls.w",    "mulu.w",   "neg",     "negc",     "nop",     "not",
    "nott",      "ocbi",     "ocbp",    "ocbwb",    "or",      "or.b",
    "pref",      "prefi",    "resbank", "rotcl",    "rotcr",   "rotl",
    "rotr",      "rte",      "rts",     "rts/n",    "rtv/n",   "setdmx",
    "setdmy",    "setrc",    "sets",    "sett",     "shad",    "shal",
    "shar",      "shld",     "shll",    "shll16",   "shll2",   "shll8",
    "shlr",      "shlr16",   "shlr2",   "shlr8",    "sleep",   "stbank",
    "stc",       "stc.l",    "sts",     "sts.l",    "sub",     "subc",
    "subv",      "swap.b",   "swap.w",  "synco",    "tas.b",   "trapa",
    "tst",       "tst.b",    "xor",     "xor.b",    "xtrct",
};

constexpr StringLiteral SHFourByteInstructionMnemonics[] = {
    "band.b", "bandnot.b", "bclr.b", "bld.b",  "bldnot.b",
    "bor.b",  "bornot.b",  "bset.b", "bst.b",  "bxor.b",
    "movi20", "movi20s",   "movu.b", "movu.w", "dsp.ppi",
};

enum class ConstantKind {
  Value,
  Indeterminate,
  Malformed,
};

struct ConstantResult {
  ConstantKind Kind = ConstantKind::Value;
  int64_t Value = 0;
  std::string Diagnostic;
};

// Parse with MC's GNU grammar. Keep only the size estimator's restrictions:
// unknown symbols/operands are indeterminate, nesting is bounded, and unsafe
// arithmetic is rejected before MC evaluation. In particular, MC comparisons
// produce -1, and shifts have the same precedence as multiplication.
class ConstantParser {
  std::string Input;
  ConstantResult Result;

  bool fail(ConstantKind Kind, const Twine &Message) {
    Result.Kind = Kind;
    Result.Diagnostic = Message.str();
    return false;
  }

  bool checkTokens(const MCAsmInfo &MAI) {
    AsmLexer Lexer(MAI);
    Lexer.setBuffer(Input);
    SmallVector<unsigned> Levels;
    unsigned Depth = 0;
    bool Operand = true;
    for (AsmToken Token = Lexer.Lex(); !Token.is(AsmToken::Eof);
         Token = Lexer.Lex()) {
      auto Kind = Token.getKind();
      if (Kind == AsmToken::Identifier || Kind == AsmToken::Dot ||
          Kind == AsmToken::Dollar)
        return fail(ConstantKind::Indeterminate,
                    Token.getString().contains('$')
                        ? "operand-dependent expression"
                        : "symbol-dependent expression");
      if (Kind == AsmToken::Error || Kind == AsmToken::Real ||
          Kind == AsmToken::String || Kind == AsmToken::BigNum)
        return fail(ConstantKind::Malformed,
                    "invalid constant expression token");
      bool Unary =
          Operand && (Kind == AsmToken::Plus || Kind == AsmToken::Minus ||
                      Kind == AsmToken::Tilde || Kind == AsmToken::Exclaim);
      // ELF MC expressions also admit brackets. Both grouping forms recurse
      // during parsing but disappear from the AST, so evaluation cannot bound
      // them afterward. Include unary prefixes in the same depth budget.
      bool Opening = Kind == AsmToken::LParen || Kind == AsmToken::LBrac;
      bool Closing = Kind == AsmToken::RParen || Kind == AsmToken::RBrac;
      if (Unary || Opening) {
        if (++Depth > MaxParserNestingDepth)
          return fail(ConstantKind::Malformed,
                      "constant expression nesting is too deep");
        if (Opening)
          Levels.push_back(Depth);
        Operand = true;
      } else if (Kind == AsmToken::Integer || Closing) {
        if (Kind == AsmToken::Integer && Token.getIntVal() < 0)
          return fail(ConstantKind::Malformed,
                      "integer is outside the signed 64-bit range");
        if (Closing) {
          if (Levels.empty())
            return fail(ConstantKind::Malformed,
                        "unmatched '" + Token.getString() + "' in expression");
          Levels.pop_back();
        }
        Depth = Levels.empty() ? 0 : Levels.back();
        Operand = false;
      } else {
        Operand = true;
      }
    }
    return true;
  }

  bool evaluate(const MCExpr *Expr, MCContext &Ctx, int64_t &Value,
                unsigned Depth = 0) {
    if (Depth > MaxParserNestingDepth)
      return fail(ConstantKind::Malformed,
                  "constant expression nesting is too deep");
    if (const auto *Constant = dyn_cast<MCConstantExpr>(Expr)) {
      Value = Constant->getValue();
      return true;
    }
    const MCExpr *Folded;
    if (const auto *Unary = dyn_cast<MCUnaryExpr>(Expr)) {
      int64_t Operand;
      if (!evaluate(Unary->getSubExpr(), Ctx, Operand, Depth + 1))
        return false;
      if (Unary->getOpcode() == MCUnaryExpr::Minus &&
          Operand == std::numeric_limits<int64_t>::min())
        return fail(ConstantKind::Malformed,
                    "constant expression subtraction overflows");
      Folded = MCUnaryExpr::create(Unary->getOpcode(),
                                   MCConstantExpr::create(Operand, Ctx), Ctx);
    } else if (const auto *Binary = dyn_cast<MCBinaryExpr>(Expr)) {
      int64_t Left, Right, Ignored;
      if (!evaluate(Binary->getLHS(), Ctx, Left, Depth + 1) ||
          !evaluate(Binary->getRHS(), Ctx, Right, Depth + 1))
        return false;
      auto Op = Binary->getOpcode();
      StringRef Error;
      if (Op == MCBinaryExpr::Add && AddOverflow(Left, Right, Ignored))
        Error = "addition overflows";
      if (Op == MCBinaryExpr::Sub && SubOverflow(Left, Right, Ignored))
        Error = "subtraction overflows";
      if (Op == MCBinaryExpr::Mul && MulOverflow(Left, Right, Ignored))
        Error = "multiplication overflows";
      if (Op == MCBinaryExpr::Div || Op == MCBinaryExpr::Mod) {
        if (!Right)
          Error = "division or remainder by zero";
        if (Left == std::numeric_limits<int64_t>::min() && Right == -1) {
          if (Op == MCBinaryExpr::Mod) {
            Value = 0;
            return true;
          }
          Error = "division overflows";
        }
      }
      if (Op == MCBinaryExpr::Shl || Op == MCBinaryExpr::LShr ||
          Op == MCBinaryExpr::AShr) {
        if (Right < 0 || Right >= 64) {
          Error = "invalid shift count";
        } else if (Op == MCBinaryExpr::Shl) {
          bool Overflow;
          (void)APInt(64, Left).sshl_ov(APInt(64, Right), Overflow);
          if (Overflow)
            Error = "left shift overflows";
        }
      }
      if (!Error.empty())
        return fail(ConstantKind::Malformed, "constant expression " + Error);
      Folded = MCBinaryExpr::create(Op, MCConstantExpr::create(Left, Ctx),
                                    MCConstantExpr::create(Right, Ctx), Ctx);
    } else {
      return fail(ConstantKind::Indeterminate, "symbol-dependent expression");
    }
    return Folded->evaluateAsAbsolute(Value);
  }

public:
  explicit ConstantParser(StringRef Input) : Input(Input.str()) {}

  ConstantResult parse() {
    Triple TT("sh-unknown-none-elf");
    MCTargetOptions Options;
    SHMCAsmInfo MAI(TT, Options);
    if (!checkTokens(MAI))
      return Result;
    MCRegisterInfo MRI;
    MCSubtargetInfo STI(TT, "", "", "", "", {}, {}, {}, nullptr, nullptr,
                        nullptr, nullptr, nullptr, nullptr, nullptr);
    SourceMgr SM;
    SM.AddNewSourceBuffer(MemoryBuffer::getMemBufferCopy(Input), SMLoc());
    SM.setDiagHandler(
        [](const SMDiagnostic &D, void *Context) {
          auto &Self = *static_cast<ConstantParser *>(Context);
          Self.fail(ConstantKind::Malformed, D.getMessage());
        },
        this);
    MCContext Ctx(TT, MAI, MRI, STI, &SM);
    std::unique_ptr<MCStreamer> Streamer(createNullStreamer(Ctx));
    std::unique_ptr<MCAsmParser> Parser(
        createMCAsmParser(SM, Ctx, *Streamer, MAI));
    Parser->setFoldExpressions(false);
    Parser->Lex();
    const MCExpr *Expr;
    if (Parser->parseExpression(Expr)) {
      Parser->printPendingErrors();
      if (Result.Kind == ConstantKind::Value)
        fail(ConstantKind::Malformed, "invalid constant expression");
    } else if (!Parser->getTok().is(AsmToken::Eof) &&
               !Parser->getTok().is(AsmToken::EndOfStatement)) {
      fail(ConstantKind::Malformed, "unexpected token in constant expression");
    } else {
      evaluate(Expr, Ctx, Result.Value);
    }
    return Result;
  }
};

enum class StopDirective {
  None,
  EndRepeat,
  Else,
  EndIf,
};

class InlineAsmSizeParser {
  SmallVector<StringRef, 32> Statements;
  SHInlineAsmSize Result;

  bool fail(SHInlineAsmSizeKind Kind, const Twine &Message) {
    if (Result.Kind == SHInlineAsmSizeKind::Exact ||
        Result.Kind == SHInlineAsmSizeKind::Conservative) {
      Result.Bytes = 0;
      Result.Kind = Kind;
      Result.Diagnostic = Message.str();
    }
    return false;
  }

  bool add(uint64_t &Size, uint64_t Addition) {
    if (Addition > std::numeric_limits<uint64_t>::max() - Size)
      return fail(SHInlineAsmSizeKind::Malformed,
                  "inline assembly size addition overflows");
    Size += Addition;
    return true;
  }

  bool multiply(uint64_t Left, uint64_t Right, uint64_t &Product) {
    if (Left != 0 && Right > std::numeric_limits<uint64_t>::max() / Left)
      return fail(SHInlineAsmSizeKind::Malformed,
                  "inline assembly size multiplication overflows");
    Product = Left * Right;
    return true;
  }

  bool tokenize(StringRef Asm) {
    size_t Start = 0;
    char Quote = 0;
    bool Escaped = false;
    bool Comment = false;
    for (size_t I = 0; I != Asm.size(); ++I) {
      char Character = Asm[I];
      if (Comment) {
        if (Character != '\n' && Character != '\r')
          continue;
        Comment = false;
        Start = I + 1;
        if (Character == '\r' && I + 1 != Asm.size() && Asm[I + 1] == '\n') {
          ++I;
          ++Start;
        }
        continue;
      }
      if (Quote) {
        if (Escaped) {
          Escaped = false;
          continue;
        }
        if (Character == '\\') {
          Escaped = true;
          continue;
        }
        if (Character == Quote)
          Quote = 0;
        continue;
      }
      if (Character == '\'' || Character == '"') {
        Quote = Character;
        continue;
      }
      // Match AsmLexer's additional '#' line comments at the start of a
      // statement, including after labels. Elsewhere '#' introduces an SH
      // immediate operand and must remain part of the instruction.
      if (Character == '!' ||
          (Character == '#' && stripLabels(Asm.slice(Start, I)).empty())) {
        Statements.push_back(Asm.slice(Start, I));
        Comment = true;
        continue;
      }
      if (Character != ';' && Character != '\n' && Character != '\r')
        continue;
      Statements.push_back(Asm.slice(Start, I));
      Start = I + 1;
      if (Character == '\r' && I + 1 != Asm.size() && Asm[I + 1] == '\n') {
        ++I;
        ++Start;
      }
    }
    if (Quote)
      return fail(SHInlineAsmSizeKind::Malformed,
                  "unterminated quoted string in inline assembly");
    if (!Comment)
      Statements.push_back(Asm.substr(Start));
    return true;
  }

  static bool isLabelName(StringRef Name) {
    Name = Name.trim();
    if (Name.empty())
      return false;
    if (llvm::all_of(Name, [](char Character) {
          return isDigit(static_cast<unsigned char>(Character));
        }))
      return true;
    if (!isAlpha(static_cast<unsigned char>(Name.front())) &&
        Name.front() != '_' && Name.front() != '.' && Name.front() != '$')
      return false;
    return llvm::all_of(Name.drop_front(), [](char Character) {
      return isAlnum(static_cast<unsigned char>(Character)) ||
             Character == '_' || Character == '.' || Character == '$';
    });
  }

  static StringRef stripLabels(StringRef Statement) {
    Statement = Statement.trim();
    while (!Statement.empty()) {
      size_t Colon = Statement.find(':');
      if (Colon == StringRef::npos || !isLabelName(Statement.take_front(Colon)))
        break;
      Statement = Statement.drop_front(Colon + 1).trim();
    }
    return Statement;
  }

  static std::pair<StringRef, StringRef> splitHead(StringRef Statement) {
    Statement = stripLabels(Statement);
    size_t End = 0;
    while (End != Statement.size() &&
           !isSpace(static_cast<unsigned char>(Statement[End])))
      ++End;
    return {Statement.take_front(End), Statement.drop_front(End).trim()};
  }

  static StopDirective getStopDirective(StringRef Statement) {
    auto [Head, Arguments] = splitHead(Statement);
    if (!Arguments.empty())
      return StopDirective::None;
    if (Head.equals_insensitive(".endr"))
      return StopDirective::EndRepeat;
    if (Head.equals_insensitive(".else"))
      return StopDirective::Else;
    if (Head.equals_insensitive(".endif"))
      return StopDirective::EndIf;
    return StopDirective::None;
  }

  ConstantResult parseConstant(StringRef Expression) {
    return ConstantParser(Expression).parse();
  }

  bool useConstant(StringRef Expression, int64_t &Value, StringRef Directive) {
    ConstantResult Constant = parseConstant(Expression);
    if (Constant.Kind == ConstantKind::Indeterminate)
      return fail(SHInlineAsmSizeKind::Indeterminate,
                  Directive + " has an " + Constant.Diagnostic);
    if (Constant.Kind == ConstantKind::Malformed)
      return fail(SHInlineAsmSizeKind::Malformed,
                  Directive +
                      " has a malformed expression: " + Constant.Diagnostic);
    Value = Constant.Value;
    return true;
  }

  bool splitArguments(StringRef Input, SmallVectorImpl<StringRef> &Arguments) {
    size_t Start = 0;
    unsigned Parentheses = 0;
    char Quote = 0;
    bool Escaped = false;
    for (size_t I = 0; I != Input.size(); ++I) {
      char Character = Input[I];
      if (Quote) {
        if (Escaped)
          Escaped = false;
        else if (Character == '\\')
          Escaped = true;
        else if (Character == Quote)
          Quote = 0;
        continue;
      }
      if (Character == '\'' || Character == '"') {
        Quote = Character;
        continue;
      }
      if (Character == '(') {
        ++Parentheses;
        continue;
      }
      if (Character == ')') {
        if (Parentheses == 0)
          return fail(SHInlineAsmSizeKind::Malformed,
                      "unmatched ')' in directive arguments");
        --Parentheses;
        continue;
      }
      if (Character == ',' && Parentheses == 0) {
        Arguments.push_back(Input.slice(Start, I).trim());
        Start = I + 1;
      }
    }
    if (Quote || Parentheses != 0)
      return fail(SHInlineAsmSizeKind::Malformed,
                  "unterminated directive argument");
    Arguments.push_back(Input.substr(Start).trim());
    return true;
  }

  bool requireArgumentCount(ArrayRef<StringRef> Arguments, unsigned Minimum,
                            unsigned Maximum, StringRef Directive) {
    if (Arguments.size() < Minimum || Arguments.size() > Maximum)
      return fail(SHInlineAsmSizeKind::Malformed,
                  Directive + " has the wrong number of arguments");
    if (llvm::any_of(Arguments,
                     [](StringRef Argument) { return Argument.empty(); }))
      return fail(SHInlineAsmSizeKind::Malformed,
                  Directive + " has an empty argument");
    return true;
  }

  bool measureValues(StringRef ArgumentsText, uint64_t ValueSize,
                     StringRef Directive, uint64_t &Size) {
    SmallVector<StringRef, 8> Arguments;
    if (!splitArguments(ArgumentsText, Arguments) ||
        !requireArgumentCount(Arguments, 1,
                              std::numeric_limits<unsigned>::max(), Directive))
      return false;
    uint64_t Addition;
    if (!multiply(Arguments.size(), ValueSize, Addition))
      return false;
    return add(Size, Addition);
  }

  bool measureFill(StringRef ArgumentsText, uint64_t &Size) {
    SmallVector<StringRef, 3> Arguments;
    if (!splitArguments(ArgumentsText, Arguments) ||
        !requireArgumentCount(Arguments, 1, 3, ".fill"))
      return false;
    int64_t Count;
    if (!useConstant(Arguments[0], Count, ".fill"))
      return false;
    if (Count < 0)
      return fail(SHInlineAsmSizeKind::Malformed,
                  ".fill has a negative repeat count");
    int64_t ElementSize = 1;
    if (Arguments.size() >= 2 &&
        !useConstant(Arguments[1], ElementSize, ".fill"))
      return false;
    if (ElementSize < 0)
      ElementSize = 0;
    if (ElementSize > 8)
      ElementSize = 8;
    uint64_t Addition;
    if (!multiply(static_cast<uint64_t>(Count),
                  static_cast<uint64_t>(ElementSize), Addition))
      return false;
    return add(Size, Addition);
  }

  bool measureSpace(StringRef ArgumentsText, StringRef Directive,
                    uint64_t &Size) {
    SmallVector<StringRef, 2> Arguments;
    if (!splitArguments(ArgumentsText, Arguments) ||
        !requireArgumentCount(Arguments, 1, 2, Directive))
      return false;
    int64_t Count;
    if (!useConstant(Arguments[0], Count, Directive))
      return false;
    if (Count < 0)
      return fail(SHInlineAsmSizeKind::Malformed,
                  Directive + " has a negative byte count");
    return add(Size, static_cast<uint64_t>(Count));
  }

  bool measureAlignment(StringRef ArgumentsText, StringRef Directive,
                        bool PowerOfTwoArgument, uint64_t &Size,
                        bool &Conservative) {
    if (ArgumentsText.empty() && Directive.starts_with(".p2align"))
      return true;
    SmallVector<StringRef, 3> Arguments;
    if (!splitArguments(ArgumentsText, Arguments) || Arguments.empty() ||
        Arguments.size() > 3 || Arguments[0].empty())
      return fail(SHInlineAsmSizeKind::Malformed,
                  Directive + " has malformed arguments");
    int64_t Argument;
    if (!useConstant(Arguments[0], Argument, Directive))
      return false;
    uint64_t Alignment;
    if (PowerOfTwoArgument) {
      if (Argument < 0 || Argument >= 32)
        return fail(SHInlineAsmSizeKind::Malformed,
                    Directive + " has an invalid alignment exponent");
      Alignment = uint64_t(1) << Argument;
    } else {
      if (Argument < 0)
        return fail(SHInlineAsmSizeKind::Malformed,
                    Directive + " has a negative alignment");
      Alignment = Argument == 0 ? 1 : static_cast<uint64_t>(Argument);
      if (!isPowerOf2_64(Alignment) || Alignment >= (uint64_t(1) << 32))
        return fail(SHInlineAsmSizeKind::Malformed,
                    Directive + " alignment is not a power of two below 2**32");
    }
    uint64_t MaximumPadding = Alignment - 1;
    if (Arguments.size() == 3) {
      if (Arguments[2].empty())
        return fail(SHInlineAsmSizeKind::Malformed,
                    Directive + " has an empty maximum-skip argument");
      int64_t MaximumSkip;
      if (!useConstant(Arguments[2], MaximumSkip, Directive))
        return false;
      if (MaximumSkip < 1)
        return fail(SHInlineAsmSizeKind::Malformed,
                    Directive + " has an invalid maximum-skip argument");
      if (static_cast<uint64_t>(MaximumSkip) < Alignment)
        MaximumPadding = static_cast<uint64_t>(MaximumSkip);
    }
    if (MaximumPadding != 0)
      Conservative = true;
    return add(Size, MaximumPadding);
  }

  bool measureString(StringRef String, uint64_t &Length) {
    String = String.trim();
    if (String.size() < 2 || String.front() != '"' || String.back() != '"')
      return fail(SHInlineAsmSizeKind::Malformed,
                  "string data directive expects a quoted string");
    String = String.drop_front().drop_back();
    for (size_t I = 0; I != String.size(); ++I) {
      if (String[I] == '$' && I + 1 != String.size() &&
          (isDigit(static_cast<unsigned char>(String[I + 1])) ||
           String[I + 1] == '{'))
        return fail(SHInlineAsmSizeKind::Indeterminate,
                    "string data directive has operand-dependent contents");
      if (String[I] != '\\') {
        if (!add(Length, 1))
          return false;
        continue;
      }
      if (++I == String.size())
        return fail(SHInlineAsmSizeKind::Malformed,
                    "unexpected backslash at end of string");
      if (String[I] == 'x' || String[I] == 'X') {
        if (I + 1 == String.size() || !isHexDigit(String[I + 1]))
          return fail(SHInlineAsmSizeKind::Malformed,
                      "invalid hexadecimal string escape");
        while (I + 1 != String.size() && isHexDigit(String[I + 1]))
          ++I;
      } else if (String[I] >= '0' && String[I] <= '7') {
        unsigned Digits = 1;
        while (Digits != 3 && I + 1 != String.size() && String[I + 1] >= '0' &&
               String[I + 1] <= '7') {
          ++I;
          ++Digits;
        }
      } else if (!StringRef("bfnrt\"\\").contains(String[I])) {
        return fail(SHInlineAsmSizeKind::Malformed,
                    "unrecognized string escape");
      }
      if (!add(Length, 1))
        return false;
    }
    return true;
  }

  bool measureAscii(StringRef ArgumentsText, bool ZeroTerminated,
                    uint64_t &Size) {
    SmallVector<StringRef, 8> Arguments;
    if (!splitArguments(ArgumentsText, Arguments) ||
        !requireArgumentCount(Arguments, 1,
                              std::numeric_limits<unsigned>::max(),
                              ZeroTerminated ? ".asciz/.string" : ".ascii"))
      return false;
    for (StringRef Argument : Arguments) {
      uint64_t Length = 0;
      if (!measureString(Argument, Length) || !add(Size, Length) ||
          (ZeroTerminated && !add(Size, 1)))
        return false;
    }
    return true;
  }

  static bool isInstruction(StringRef Mnemonic) {
    SmallString<16> Lower(Mnemonic);
    for (char &Character : Lower)
      Character = llvm::toLower(Character);
    return llvm::any_of(SHInstructionMnemonics, [&](StringRef Candidate) {
      return Candidate == Lower;
    });
  }

  static bool hasDSPParallelOperation(StringRef Arguments) {
    while (!Arguments.empty()) {
      Arguments = Arguments.ltrim();
      size_t End = Arguments.find_if([](char Character) {
        return isSpace(static_cast<unsigned char>(Character));
      });
      StringRef Word = Arguments.take_front(End);
      SmallString<16> Lower(Word);
      for (char &Character : Lower)
        Character = llvm::toLower(Character);
      if (SH::isDSPParallelOperationMnemonic(Lower))
        return true;
      if (End == StringRef::npos)
        break;
      Arguments = Arguments.drop_front(End);
    }
    return false;
  }

  bool measureDSPInstruction(StringRef Mnemonic, StringRef Arguments,
                             uint64_t &Size) {
    if (Mnemonic == "repeat")
      return add(Size, 6);
    if (SH::isDSPParallelOperationMnemonic(Mnemonic) ||
        hasDSPParallelOperation(Arguments))
      return add(Size, 4);
    return add(Size, 2);
  }

  bool measureInstruction(StringRef Mnemonic, StringRef Arguments,
                          uint64_t &Size) {
    if (llvm::is_contained(SHFourByteInstructionMnemonics, Mnemonic))
      return add(Size, 4);
    if (Mnemonic != "mov.b" && Mnemonic != "mov.w" && Mnemonic != "mov.l" &&
        Mnemonic != "fmov.s" && Mnemonic != "fmov.d")
      return add(Size, 2);

    SmallVector<StringRef, 2> Operands;
    if (!splitArguments(Arguments, Operands) || Operands.size() != 2)
      return fail(SHInlineAsmSizeKind::Indeterminate,
                  "cannot classify the operands of mixed-width instruction '" +
                      Mnemonic + "'");
    unsigned MemoryIndex = Operands[0].contains("@(")   ? 0
                           : Operands[1].contains("@(") ? 1
                                                        : 2;
    if (MemoryIndex == 2)
      return add(Size, 2);
    StringRef Memory = Operands[MemoryIndex].trim();
    size_t Open = Memory.find("@(");
    size_t Close = Memory.rfind(')');
    if (Open == StringRef::npos || Close == StringRef::npos ||
        Close <= Open + 2)
      return fail(SHInlineAsmSizeKind::Malformed,
                  "malformed memory operand in mixed-width instruction '" +
                      Mnemonic + "'");
    SmallVector<StringRef, 2> Address;
    if (!splitArguments(Memory.slice(Open + 2, Close), Address) ||
        Address.size() != 2)
      return fail(SHInlineAsmSizeKind::Indeterminate,
                  "cannot classify the address of mixed-width instruction '" +
                      Mnemonic + "'");
    SmallString<16> First(Address[0].trim());
    SmallString<16> Base(Address[1].trim());
    for (char &Character : First)
      Character = llvm::toLower(Character);
    for (char &Character : Base)
      Character = llvm::toLower(Character);
    if (First == "r0" || Base == "gbr" || Base == "pc")
      return add(Size, 2);
    if (Mnemonic == "fmov.s" || Mnemonic == "fmov.d") {
      if (StringRef(First).contains('$') || StringRef(Base).contains('$'))
        return fail(SHInlineAsmSizeKind::Indeterminate,
                    "operand-dependent mixed-width instruction '" + Mnemonic +
                        "'");
      return add(Size, 4);
    }

    ConstantResult Displacement = parseConstant(First);
    if (Displacement.Kind == ConstantKind::Indeterminate)
      return fail(SHInlineAsmSizeKind::Indeterminate,
                  "operand-dependent mixed-width instruction '" + Mnemonic +
                      "'");
    if (Displacement.Kind == ConstantKind::Malformed)
      return fail(SHInlineAsmSizeKind::Malformed,
                  "malformed displacement in mixed-width instruction '" +
                      Mnemonic + "'");
    if (Mnemonic == "mov.l") {
      bool Short = Displacement.Value >= 0 && Displacement.Value <= 60 &&
                   Displacement.Value % 4 == 0;
      return add(Size, Short ? 2 : 4);
    }
    int64_t ShortMaximum = Mnemonic == "mov.b" ? 15 : 30;
    int64_t Scale = Mnemonic == "mov.b" ? 1 : 2;
    bool ShortDisplacement = Displacement.Value >= 0 &&
                             Displacement.Value <= ShortMaximum &&
                             Displacement.Value % Scale == 0;
    SmallString<16> Other(Operands[1 - MemoryIndex].trim());
    for (char &Character : Other)
      Character = llvm::toLower(Character);
    if (Other == "r0" && ShortDisplacement)
      return add(Size, 2);
    if (StringRef(Other).contains('$') && ShortDisplacement)
      return fail(SHInlineAsmSizeKind::Indeterminate,
                  "operand-dependent mixed-width instruction '" + Mnemonic +
                      "'");
    return add(Size, 4);
  }

  static bool isNonEmittingDirective(StringRef Directive) {
    SmallString<32> Lower(Directive);
    for (char &Character : Lower)
      Character = llvm::toLower(Character);
    if (StringRef(Lower).starts_with(".cfi_"))
      return true;
    return StringSwitch<bool>(Lower)
        .Cases({".file", ".loc", ".line", ".loc_label"}, true)
        .Cases({".type", ".size", ".ident", ".version"}, true)
        .Cases({".globl", ".global", ".local", ".extern"}, true)
        .Cases({".weak", ".hidden", ".protected", ".internal"}, true)
        .Cases({".symver", ".comm", ".common", ".lcomm"}, true)
        .Cases({".syntax", ".arch", ".cpu", ".module"}, true)
        .Default(false);
  }

  bool measureStatement(StringRef Statement, uint64_t &Size,
                        bool &Conservative) {
    auto [Head, Arguments] = splitHead(Statement);
    if (Head.empty())
      return true;
    SmallString<32> Directive(Head);
    for (char &Character : Directive)
      Character = llvm::toLower(Character);
    StringRef Name(Directive);
    if (!Name.starts_with(".")) {
      if (Name == "bt.s")
        Name = "bt/s";
      else if (Name == "bf.s")
        Name = "bf/s";
      else if (Name == "muls")
        Name = "muls.w";
      else if (Name == "mulu")
        Name = "mulu.w";
      if (Name == "repeat" || SH::isDSPCompoundMnemonic(Name))
        return measureDSPInstruction(Name, Arguments, Size);
      if (!isInstruction(Name))
        return fail(SHInlineAsmSizeKind::Indeterminate,
                    "unknown instruction or macro invocation '" + Head + "'");
      return measureInstruction(Name, Arguments, Size);
    }
    if (Name == ".byte")
      return measureValues(Arguments, 1, Name, Size);
    if (Name == ".short" || Name == ".value" || Name == ".2byte")
      return measureValues(Arguments, 2, Name, Size);
    if (Name == ".long" || Name == ".int" || Name == ".4byte")
      return measureValues(Arguments, 4, Name, Size);
    if (Name == ".quad" || Name == ".8byte")
      return measureValues(Arguments, 8, Name, Size);
    if (Name == ".fill")
      return measureFill(Arguments, Size);
    if (Name == ".space" || Name == ".skip" || Name == ".zero")
      return measureSpace(Arguments, Name, Size);
    if (Name == ".align" || Name == ".align32" || Name.starts_with(".p2align"))
      return measureAlignment(Arguments, Name, true, Size, Conservative);
    if (Name.starts_with(".balign"))
      return measureAlignment(Arguments, Name, false, Size, Conservative);
    if (Name == ".ascii")
      return measureAscii(Arguments, false, Size);
    if (Name == ".asciz" || Name == ".string")
      return measureAscii(Arguments, true, Size);
    if (isNonEmittingDirective(Name))
      return true;
    return fail(SHInlineAsmSizeKind::Indeterminate,
                "unsupported size-affecting directive '" + Head + "'");
  }

  bool parseSequence(size_t &Index, bool Active, unsigned Depth,
                     bool AllowEndRepeat, bool AllowElse, bool AllowEndIf,
                     uint64_t &Size, bool &Conservative, StopDirective &Stop) {
    while (Index != Statements.size()) {
      StringRef Statement = stripLabels(Statements[Index]);
      if (Statement.empty()) {
        ++Index;
        continue;
      }
      StopDirective CurrentStop = getStopDirective(Statement);
      if (CurrentStop != StopDirective::None) {
        bool Allowed =
            (CurrentStop == StopDirective::EndRepeat && AllowEndRepeat) ||
            (CurrentStop == StopDirective::Else && AllowElse) ||
            (CurrentStop == StopDirective::EndIf && AllowEndIf);
        if (!Allowed)
          return fail(SHInlineAsmSizeKind::Malformed,
                      "unmatched inline assembly control directive");
        Stop = CurrentStop;
        return true;
      }

      auto [Head, Arguments] = splitHead(Statement);
      if (Head.equals_insensitive(".rept") || Head.equals_insensitive(".rep")) {
        if (Depth == MaxParserNestingDepth)
          return fail(SHInlineAsmSizeKind::Malformed,
                      "inline assembly repetition nesting is too deep");
        int64_t Count = 0;
        if (Active && !useConstant(Arguments, Count, Head))
          return false;
        if (Active && Count < 0)
          return fail(SHInlineAsmSizeKind::Malformed,
                      "inline assembly repetition count is negative");
        ++Index;
        uint64_t BodySize = 0;
        bool BodyConservative = false;
        StopDirective BodyStop = StopDirective::None;
        if (!parseSequence(Index, Active && Count != 0, Depth + 1, true, false,
                           false, BodySize, BodyConservative, BodyStop))
          return false;
        if (BodyStop != StopDirective::EndRepeat)
          return fail(SHInlineAsmSizeKind::Malformed,
                      "unterminated inline assembly repetition");
        ++Index;
        if (Active) {
          uint64_t RepeatedSize;
          if (!multiply(BodySize, static_cast<uint64_t>(Count), RepeatedSize) ||
              !add(Size, RepeatedSize))
            return false;
          Conservative |= BodyConservative && Count != 0;
        }
        continue;
      }

      if (Head.equals_insensitive(".if")) {
        if (Depth == MaxParserNestingDepth)
          return fail(SHInlineAsmSizeKind::Malformed,
                      "inline assembly conditional nesting is too deep");
        int64_t Condition = 0;
        if (Active && !useConstant(Arguments, Condition, Head))
          return false;
        ++Index;
        uint64_t TrueSize = 0;
        bool TrueConservative = false;
        StopDirective ConditionalStop = StopDirective::None;
        if (!parseSequence(Index, Active && Condition != 0, Depth + 1, false,
                           true, true, TrueSize, TrueConservative,
                           ConditionalStop))
          return false;
        uint64_t FalseSize = 0;
        bool FalseConservative = false;
        if (ConditionalStop == StopDirective::Else) {
          ++Index;
          if (!parseSequence(Index, Active && Condition == 0, Depth + 1, false,
                             false, true, FalseSize, FalseConservative,
                             ConditionalStop))
            return false;
        }
        if (ConditionalStop != StopDirective::EndIf)
          return fail(SHInlineAsmSizeKind::Malformed,
                      "unterminated inline assembly conditional");
        ++Index;
        if (Active && !add(Size, Condition != 0 ? TrueSize : FalseSize))
          return false;
        if (Active)
          Conservative |= Condition != 0 ? TrueConservative : FalseConservative;
        continue;
      }

      if (Active && !measureStatement(Statement, Size, Conservative))
        return false;
      ++Index;
    }
    Stop = StopDirective::None;
    return true;
  }

public:
  explicit InlineAsmSizeParser(StringRef Asm) { tokenize(Asm); }

  SHInlineAsmSize parse() {
    if (Result.Kind == SHInlineAsmSizeKind::Malformed)
      return Result;
    size_t Index = 0;
    bool Conservative = false;
    StopDirective Stop = StopDirective::None;
    uint64_t Size = 0;
    if (!parseSequence(Index, true, 0, false, false, false, Size, Conservative,
                       Stop))
      return Result;
    Result.Bytes = Size;
    Result.Kind = Conservative ? SHInlineAsmSizeKind::Conservative
                               : SHInlineAsmSizeKind::Exact;
    return Result;
  }
};

bool selectInlineAsmVariant(StringRef Asm, SmallVectorImpl<char> &Selected,
                            std::string &Diagnostic) {
  int CurrentVariant = -1;
  for (size_t I = 0; I != Asm.size(); ++I) {
    char Character = Asm[I];
    // Generic AsmPrinter preserves line breaks in every alternative so
    // source locations and statement separation remain stable.
    if (Character == '\n') {
      Selected.push_back(Character);
      continue;
    }
    if (Character != '$' || I + 1 == Asm.size()) {
      if (CurrentVariant == -1 || CurrentVariant == 0)
        Selected.push_back(Character);
      continue;
    }
    char Next = Asm[I + 1];
    if (Next == '$') {
      if (CurrentVariant == -1 || CurrentVariant == 0)
        Selected.push_back('$');
      ++I;
      continue;
    }
    if (Next == '(') {
      if (CurrentVariant != -1) {
        Diagnostic = "nested inline assembly alternatives";
        return false;
      }
      CurrentVariant = 0;
      ++I;
      continue;
    }
    if (Next == '|') {
      if (CurrentVariant == -1)
        Selected.push_back('|');
      else
        ++CurrentVariant;
      ++I;
      continue;
    }
    if (Next == ')') {
      if (CurrentVariant == -1) {
        Selected.push_back('}');
      } else {
        CurrentVariant = -1;
      }
      ++I;
      continue;
    }
    if (CurrentVariant == -1 || CurrentVariant == 0)
      Selected.push_back(Character);
  }
  // Generic AsmPrinter accepts an unterminated final alternative and emits
  // the selected text accumulated through the end of the template.
  return true;
}

} // namespace

ArrayRef<StringLiteral> llvm::getSHInlineAsmInstructionMnemonics() {
  return SHInstructionMnemonics;
}

std::optional<std::string>
llvm::selectSHInlineAsmAlternative(StringRef Asm, std::string &Diagnostic) {
  SmallString<256> Selected;
  if (!selectInlineAsmVariant(Asm, Selected, Diagnostic))
    return std::nullopt;
  return std::string(Selected);
}

SHInlineAsmSize llvm::getSHInlineAsmSize(StringRef Asm) {
  std::string Diagnostic;
  std::optional<std::string> Selected =
      selectSHInlineAsmAlternative(Asm, Diagnostic);
  if (!Selected)
    return {0, SHInlineAsmSizeKind::Malformed, std::move(Diagnostic)};
  return InlineAsmSizeParser(*Selected).parse();
}
