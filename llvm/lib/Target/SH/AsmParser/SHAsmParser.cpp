//===-- SHAsmParser.cpp - Parse SH assembly instructions -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/SHMCTargetDesc.h"
#include "TargetInfo/SHTargetInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCParser/AsmLexer.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

namespace {

class SHOperand : public MCParsedAsmOperand {
  enum KindTy { Token, Register, Immediate } Kind;
  SMLoc StartLoc;
  SMLoc EndLoc;
  StringRef Tok;
  MCRegister Reg;
  const MCExpr *Expr = nullptr;

  explicit SHOperand(KindTy Kind) : Kind(Kind) {}

public:
  bool isToken() const override { return Kind == Token; }
  bool isReg() const override { return Kind == Register; }
  bool isImm() const override { return Kind == Immediate; }
  bool isMem() const override { return false; }

  SMLoc getStartLoc() const override { return StartLoc; }
  SMLoc getEndLoc() const override { return EndLoc; }

  StringRef getToken() const {
    assert(isToken());
    return Tok;
  }

  MCRegister getReg() const override {
    assert(isReg());
    return Reg;
  }

  const MCExpr *getImm() const {
    assert(isImm());
    return Expr;
  }

  bool isSImm8() const {
    const auto *CE = dyn_cast_if_present<MCConstantExpr>(Expr);
    return isImm() && CE && isInt<8>(CE->getValue());
  }

  void print(raw_ostream &OS, const MCAsmInfo &MAI) const override {
    if (isToken()) {
      OS << "token " << Tok;
      return;
    }
    if (isReg()) {
      OS << "register " << Reg;
      return;
    }
    OS << "immediate ";
    MAI.printExpr(OS, *Expr);
  }

  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1);
    Inst.addOperand(MCOperand::createReg(Reg));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1);
    const auto *CE = cast<MCConstantExpr>(Expr);
    Inst.addOperand(MCOperand::createImm(CE->getValue()));
  }

  static std::unique_ptr<SHOperand> createToken(StringRef Tok, SMLoc Loc) {
    auto Op = std::unique_ptr<SHOperand>(new SHOperand(Token));
    Op->Tok = Tok;
    Op->StartLoc = Loc;
    Op->EndLoc = Loc;
    return Op;
  }

  static std::unique_ptr<SHOperand> createReg(MCRegister Reg, SMLoc Start,
                                              SMLoc End) {
    auto Op = std::unique_ptr<SHOperand>(new SHOperand(Register));
    Op->Reg = Reg;
    Op->StartLoc = Start;
    Op->EndLoc = End;
    return Op;
  }

  static std::unique_ptr<SHOperand> createImm(const MCExpr *Expr, SMLoc Start,
                                              SMLoc End) {
    auto Op = std::unique_ptr<SHOperand>(new SHOperand(Immediate));
    Op->Expr = Expr;
    Op->StartLoc = Start;
    Op->EndLoc = End;
    return Op;
  }
};

class SHAsmParser : public MCTargetAsmParser {
  MCAsmParser &Parser;

#define GET_ASSEMBLER_HEADER
#include "SHGenAsmMatcher.inc"

  bool matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;
  bool parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;
  bool parseRegister(MCRegister &Reg, SMLoc &StartLoc, SMLoc &EndLoc) override;
  ParseStatus tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                               SMLoc &EndLoc) override;

  ParseStatus parseOperand(OperandVector &Operands, StringRef Mnemonic);
  ParseStatus parseSImm8(OperandVector &Operands);

public:
  enum SHMatchResultTy {
    Match_Dummy = FIRST_TARGET_MATCH_RESULT_TY,
#define GET_OPERAND_DIAGNOSTIC_TYPES
#include "SHGenAsmMatcher.inc"
#undef GET_OPERAND_DIAGNOSTIC_TYPES
  };

  SHAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
              const MCInstrInfo &MII)
      : MCTargetAsmParser(STI, MII), Parser(Parser) {
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }
};

} // namespace

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "SHGenAsmMatcher.inc"

ParseStatus SHAsmParser::tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                                          SMLoc &EndLoc) {
  if (Parser.getTok().isNot(AsmToken::Identifier))
    return ParseStatus::NoMatch;

  StartLoc = Parser.getTok().getLoc();
  EndLoc = Parser.getTok().getEndLoc();
  Reg = MatchRegisterName(Parser.getTok().getIdentifier());
  if (!Reg)
    return ParseStatus::NoMatch;

  Parser.Lex();
  return ParseStatus::Success;
}

bool SHAsmParser::parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                                SMLoc &EndLoc) {
  if (!tryParseRegister(Reg, StartLoc, EndLoc).isSuccess())
    return Error(Parser.getTok().getLoc(), "invalid register name");
  return false;
}

ParseStatus SHAsmParser::parseSImm8(OperandVector &Operands) {
  if (Parser.getTok().isNot(AsmToken::Hash))
    return ParseStatus::NoMatch;

  SMLoc Start = Parser.getTok().getLoc();
  Parser.Lex();

  const MCExpr *Expr;
  SMLoc End;
  if (Parser.parseExpression(Expr, End))
    return ParseStatus::Failure;
  if (!isa<MCConstantExpr>(Expr)) {
    Error(Start, "expected an integer immediate");
    return ParseStatus::Failure;
  }

  Operands.push_back(SHOperand::createImm(Expr, Start, End));
  return ParseStatus::Success;
}

ParseStatus SHAsmParser::parseOperand(OperandVector &Operands,
                                      StringRef Mnemonic) {
  ParseStatus Result = MatchOperandParserImpl(Operands, Mnemonic);
  if (!Result.isNoMatch())
    return Result;

  MCRegister Reg;
  SMLoc Start;
  SMLoc End;
  Result = tryParseRegister(Reg, Start, End);
  if (Result.isSuccess()) {
    Operands.push_back(SHOperand::createReg(Reg, Start, End));
    return Result;
  }

  if (Parser.getTok().is(AsmToken::Identifier)) {
    Error(Parser.getTok().getLoc(), "invalid register name");
    return ParseStatus::Failure;
  }
  Error(Parser.getTok().getLoc(), "unexpected operand");
  return ParseStatus::Failure;
}

bool SHAsmParser::parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                                   SMLoc NameLoc, OperandVector &Operands) {
  Operands.push_back(SHOperand::createToken(Name, NameLoc));

  if (Parser.getTok().is(AsmToken::EndOfStatement)) {
    Parser.Lex();
    return false;
  }

  if (!parseOperand(Operands, Name).isSuccess())
    return true;

  while (Parser.getTok().isNot(AsmToken::EndOfStatement)) {
    if (Parser.getTok().isNot(AsmToken::Comma))
      return Error(Parser.getTok().getLoc(), "expected comma");
    Parser.Lex();
    if (Parser.getTok().is(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(), "expected operand");
    if (!parseOperand(Operands, Name).isSuccess())
      return true;
  }

  Parser.Lex();
  return false;
}

bool SHAsmParser::matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                          OperandVector &Operands,
                                          MCStreamer &Out, uint64_t &ErrorInfo,
                                          bool MatchingInlineAsm) {
  MCInst Inst;
  switch (MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm)) {
  case Match_Success:
    Inst.setLoc(IDLoc);
    Out.emitInstruction(Inst, getSTI());
    return false;
  case Match_MnemonicFail:
    return Error(IDLoc, "unrecognized instruction mnemonic");
  case Match_MissingFeature:
    return Error(IDLoc, "instruction requires an unavailable feature");
  case Match_InvalidSImm8:
    return Error(Operands[ErrorInfo]->getStartLoc(),
                 "immediate must be an integer in the range [-128, 127]");
  case Match_InvalidTiedOperand:
    return Error(Operands[ErrorInfo]->getStartLoc(),
                 "destination register must match its input");
  case Match_InvalidOperand:
    if (ErrorInfo >= Operands.size())
      return Error(IDLoc, "too few operands for instruction");
    return Error(Operands[ErrorInfo]->getStartLoc(),
                 "invalid operand for instruction");
  default:
    llvm_unreachable("unexpected SH instruction match result");
  }
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeSHAsmParser() {
  RegisterMCAsmParser<SHAsmParser> X(getTheSHTarget());
  RegisterMCAsmParser<SHAsmParser> Y(getTheSHLETarget());
}
