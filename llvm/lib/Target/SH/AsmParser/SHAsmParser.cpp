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
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/MathExtras.h"
#include <string>

using namespace llvm;

namespace {

static bool isSHGPR(MCAsmParser &Parser, MCRegister Reg) {
  return Parser.getContext()
      .getRegisterInfo()
      ->getRegClass(SH::GPRRegClassID)
      .contains(Reg);
}

class SHOperand : public MCParsedAsmOperand {
  enum KindTy {
    Token,
    Register,
    Immediate,
    LongMemReg,
    LongMemDisp,
    ByteMemDisp,
    WordMemDisp,
    PreDecGPR,
    PostIncGPR,
    PCLiteral
  } Kind;
  SMLoc StartLoc;
  SMLoc EndLoc;
  std::string Tok;
  MCRegister Reg;
  const MCExpr *Expr = nullptr;
  bool IsLiteralDisplacement = false;

  explicit SHOperand(KindTy Kind) : Kind(Kind) {}

public:
  bool isToken() const override { return Kind == Token; }
  bool isReg() const override { return Kind == Register; }
  bool isImm() const override { return Kind == Immediate; }
  bool isMem() const override {
    return Kind == LongMemReg || Kind == LongMemDisp || Kind == ByteMemDisp ||
           Kind == WordMemDisp || Kind == PreDecGPR || Kind == PostIncGPR;
  }
  bool isLongMemReg() const { return Kind == LongMemReg; }
  bool isTasMemReg() const {
    return Kind == LongMemReg && Reg >= SH::R0 && Reg <= SH::R15;
  }
  bool isLongMemDisp() const { return Kind == LongMemDisp; }
  bool isByteMemDisp() const { return Kind == ByteMemDisp; }
  bool isWordMemDisp() const { return Kind == WordMemDisp; }
  bool isPreDecGPR() const { return Kind == PreDecGPR; }
  bool isPostIncGPR() const { return Kind == PostIncGPR; }
  bool isPCLiteral() const { return Kind == PCLiteral; }
  bool isR0() const { return isReg() && Reg == SH::R0; }
  bool isBranchTarget() const { return isImm(); }

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
    if (isMem()) {
      OS << "memory base " << Reg;
      if (Kind == PreDecGPR)
        OS << " with pre-decrement";
      if (Kind == PostIncGPR)
        OS << " with post-increment";
      if (Kind == LongMemDisp || Kind == ByteMemDisp || Kind == WordMemDisp) {
        OS << " displacement ";
        MAI.printExpr(OS, *Expr);
      }
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

  void addBranchTargetOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1);
    Inst.addOperand(MCOperand::createExpr(Expr));
  }

  void addPCLiteralOperands(MCInst &Inst, unsigned N) const {
    assert(Kind == PCLiteral && N == 1);
    if (IsLiteralDisplacement) {
      const auto *CE = cast<MCConstantExpr>(Expr);
      Inst.addOperand(MCOperand::createImm(CE->getValue()));
      return;
    }
    Inst.addOperand(MCOperand::createExpr(Expr));
  }

  void addLongMemRegOperands(MCInst &Inst, unsigned N) const {
    assert(Kind == LongMemReg && N == 1);
    Inst.addOperand(MCOperand::createReg(Reg));
  }

  void addLongMemDispOperands(MCInst &Inst, unsigned N) const {
    assert(Kind == LongMemDisp && N == 2);
    Inst.addOperand(MCOperand::createReg(Reg));
    Inst.addOperand(
        MCOperand::createImm(cast<MCConstantExpr>(Expr)->getValue()));
  }

  void addNarrowMemDispOperands(MCInst &Inst, unsigned N) const {
    assert((Kind == ByteMemDisp || Kind == WordMemDisp) && N == 2);
    Inst.addOperand(MCOperand::createReg(Reg));
    Inst.addOperand(
        MCOperand::createImm(cast<MCConstantExpr>(Expr)->getValue()));
  }

  void addPreDecGPROperands(MCInst &Inst, unsigned N) const {
    assert(Kind == PreDecGPR && N == 1);
    Inst.addOperand(MCOperand::createReg(Reg));
  }

  void addPostIncGPROperands(MCInst &Inst, unsigned N) const {
    assert(Kind == PostIncGPR && N == 1);
    Inst.addOperand(MCOperand::createReg(Reg));
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

  static std::unique_ptr<SHOperand> createLongMemReg(MCRegister Base,
                                                     SMLoc Start, SMLoc End) {
    auto Op = std::unique_ptr<SHOperand>(new SHOperand(LongMemReg));
    Op->Reg = Base;
    Op->StartLoc = Start;
    Op->EndLoc = End;
    return Op;
  }

  static std::unique_ptr<SHOperand> createLongMemDisp(MCRegister Base,
                                                      const MCExpr *Disp,
                                                      SMLoc Start, SMLoc End) {
    auto Op = std::unique_ptr<SHOperand>(new SHOperand(LongMemDisp));
    Op->Reg = Base;
    Op->Expr = Disp;
    Op->StartLoc = Start;
    Op->EndLoc = End;
    return Op;
  }

  static std::unique_ptr<SHOperand>
  createNarrowMemDisp(KindTy Kind, MCRegister Base, const MCExpr *Disp,
                      SMLoc Start, SMLoc End) {
    assert((Kind == ByteMemDisp || Kind == WordMemDisp) &&
           "invalid narrow memory operand kind");
    auto Op = std::unique_ptr<SHOperand>(new SHOperand(Kind));
    Op->Reg = Base;
    Op->Expr = Disp;
    Op->StartLoc = Start;
    Op->EndLoc = End;
    return Op;
  }

  static std::unique_ptr<SHOperand> createByteMemDisp(MCRegister Base,
                                                      const MCExpr *Disp,
                                                      SMLoc Start, SMLoc End) {
    return createNarrowMemDisp(ByteMemDisp, Base, Disp, Start, End);
  }

  static std::unique_ptr<SHOperand> createWordMemDisp(MCRegister Base,
                                                      const MCExpr *Disp,
                                                      SMLoc Start, SMLoc End) {
    return createNarrowMemDisp(WordMemDisp, Base, Disp, Start, End);
  }

  static std::unique_ptr<SHOperand> createPreDecGPR(MCRegister Base,
                                                    SMLoc Start, SMLoc End) {
    auto Op = std::unique_ptr<SHOperand>(new SHOperand(PreDecGPR));
    Op->Reg = Base;
    Op->StartLoc = Start;
    Op->EndLoc = End;
    return Op;
  }

  static std::unique_ptr<SHOperand> createPostIncGPR(MCRegister Base,
                                                     SMLoc Start, SMLoc End) {
    auto Op = std::unique_ptr<SHOperand>(new SHOperand(PostIncGPR));
    Op->Reg = Base;
    Op->StartLoc = Start;
    Op->EndLoc = End;
    return Op;
  }

  static std::unique_ptr<SHOperand> createPCLiteral(const MCExpr *Expr,
                                                    bool IsDisplacement,
                                                    SMLoc Start, SMLoc End) {
    auto Op = std::unique_ptr<SHOperand>(new SHOperand(PCLiteral));
    Op->Expr = Expr;
    Op->IsLiteralDisplacement = IsDisplacement;
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
  ParseStatus parseBranchTarget(OperandVector &Operands);
  ParseStatus parsePCLiteral(OperandVector &Operands);
  ParseStatus parseSImm8(OperandVector &Operands);
  ParseStatus parseMemory(OperandVector &Operands, StringRef Mnemonic);
  ParseStatus parsePreDecGPR(OperandVector &Operands);
  ParseStatus parsePostIncGPR(OperandVector &Operands);

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

ParseStatus SHAsmParser::parseBranchTarget(OperandVector &Operands) {
  SMLoc Start = Parser.getTok().getLoc();
  const MCExpr *Expr;
  SMLoc End;
  if (Parser.parseExpression(Expr, End))
    return ParseStatus::Failure;
  Operands.push_back(SHOperand::createImm(Expr, Start, End));
  return ParseStatus::Success;
}

ParseStatus SHAsmParser::parsePCLiteral(OperandVector &Operands) {
  if (Parser.getTok().is(AsmToken::At))
    return ParseStatus::NoMatch;
  if (Parser.getTok().is(AsmToken::Identifier) &&
      MatchRegisterName(Parser.getTok().getIdentifier()))
    return ParseStatus::NoMatch;

  SMLoc Start = Parser.getTok().getLoc();
  const MCExpr *Expr;
  SMLoc End;
  if (Parser.parseExpression(Expr, End))
    return ParseStatus::Failure;
  Operands.push_back(SHOperand::createPCLiteral(Expr, false, Start, End));
  return ParseStatus::Success;
}

ParseStatus SHAsmParser::parseMemory(OperandVector &Operands,
                                     StringRef Mnemonic) {
  if (Parser.getTok().isNot(AsmToken::At))
    return ParseStatus::NoMatch;

  SMLoc Start = Parser.getTok().getLoc();
  Parser.Lex();

  if (Parser.getTok().is(AsmToken::LParen)) {
    Parser.Lex();

    const MCExpr *Disp;
    SMLoc End;
    if (Parser.parseExpression(Disp, End))
      return ParseStatus::Failure;
    if (!isa<MCConstantExpr>(Disp)) {
      Error(Start, "expected an integer memory displacement");
      return ParseStatus::Failure;
    }
    if (Parser.getTok().isNot(AsmToken::Comma)) {
      Error(Parser.getTok().getLoc(),
            Mnemonic == "mov.b" || Mnemonic == "mov.w"
                ? "expected comma in byte/word memory operand"
                : "expected comma in longword memory operand");
      return ParseStatus::Failure;
    }
    Parser.Lex();

    MCRegister Base;
    SMLoc RegStart;
    SMLoc RegEnd;
    if (!tryParseRegister(Base, RegStart, RegEnd).isSuccess()) {
      Error(Parser.getTok().getLoc(), "invalid register name");
      return ParseStatus::Failure;
    }
    if (Parser.getTok().isNot(AsmToken::RParen)) {
      Error(Parser.getTok().getLoc(),
            Mnemonic == "mov.b" || Mnemonic == "mov.w"
                ? "expected ')' in byte/word memory operand"
                : "expected ')' in longword memory operand");
      return ParseStatus::Failure;
    }
    End = Parser.getTok().getEndLoc();
    Parser.Lex();
    int64_t ByteDisp = cast<MCConstantExpr>(Disp)->getValue();
    if ((Mnemonic == "mov.l" || Mnemonic == "mova") && Base == SH::PC) {
      if (ByteDisp < 0 || ByteDisp > 1020 || ByteDisp % 4 != 0) {
        Error(Start, "SH PC-relative literal displacement must be a multiple "
                     "of 4 in the range [0, 1020]");
        return ParseStatus::Failure;
      }
      Operands.push_back(SHOperand::createPCLiteral(Disp, true, Start, End));
    } else if (Base == SH::PC) {
      Error(Start, "pc is only valid for mov.l and mova PC-relative operands");
      return ParseStatus::Failure;
    } else if (Mnemonic == "mov.b") {
      if (ByteDisp < 0 || ByteDisp > 15) {
        Error(Start, "byte displacement must be in the range [0, 15]");
        return ParseStatus::Failure;
      }
      Operands.push_back(SHOperand::createByteMemDisp(Base, Disp, Start, End));
    } else if (Mnemonic == "mov.w") {
      if (ByteDisp < 0 || ByteDisp > 30 || ByteDisp % 2 != 0) {
        Error(Start, "word displacement must be an even byte offset in the "
                     "range [0, 30]");
        return ParseStatus::Failure;
      }
      Operands.push_back(SHOperand::createWordMemDisp(Base, Disp, Start, End));
    } else {
      if (ByteDisp < 0 || ByteDisp > 60 || ByteDisp % 4 != 0) {
        Error(Start, "longword displacement must be a multiple of 4 in the "
                     "range [0, 60]");
        return ParseStatus::Failure;
      }
      Operands.push_back(SHOperand::createLongMemDisp(Base, Disp, Start, End));
    }
    return ParseStatus::Success;
  }

  MCRegister Base;
  SMLoc RegStart;
  SMLoc RegEnd;
  if (!tryParseRegister(Base, RegStart, RegEnd).isSuccess()) {
    Error(Parser.getTok().getLoc(), "expected register or '(' after '@'");
    return ParseStatus::Failure;
  }
  Operands.push_back(SHOperand::createLongMemReg(Base, Start, RegEnd));
  return ParseStatus::Success;
}

ParseStatus SHAsmParser::parsePreDecGPR(OperandVector &Operands) {
  if (Parser.getTok().isNot(AsmToken::At))
    return ParseStatus::NoMatch;

  SMLoc Start = Parser.getTok().getLoc();
  Parser.Lex();
  if (Parser.getTok().isNot(AsmToken::Minus)) {
    Error(Parser.getTok().getLoc(), "expected '-' after '@'");
    return ParseStatus::Failure;
  }
  Parser.Lex();

  MCRegister Base;
  SMLoc RegStart;
  SMLoc RegEnd;
  if (!tryParseRegister(Base, RegStart, RegEnd).isSuccess()) {
    Error(Parser.getTok().getLoc(), "expected GPR after '@-'");
    return ParseStatus::Failure;
  }
  if (!isSHGPR(Parser, Base)) {
    Error(RegStart, "expected GPR after '@-'");
    return ParseStatus::Failure;
  }
  Operands.push_back(SHOperand::createPreDecGPR(Base, Start, RegEnd));
  return ParseStatus::Success;
}

ParseStatus SHAsmParser::parsePostIncGPR(OperandVector &Operands) {
  if (Parser.getTok().isNot(AsmToken::At))
    return ParseStatus::NoMatch;

  SMLoc Start = Parser.getTok().getLoc();
  Parser.Lex();

  MCRegister Base;
  SMLoc RegStart;
  SMLoc RegEnd;
  if (!tryParseRegister(Base, RegStart, RegEnd).isSuccess()) {
    Error(Parser.getTok().getLoc(), "expected GPR after '@'");
    return ParseStatus::Failure;
  }
  if (!isSHGPR(Parser, Base)) {
    Error(RegStart, "expected GPR after '@'");
    return ParseStatus::Failure;
  }
  if (Parser.getTok().isNot(AsmToken::Plus)) {
    Error(Parser.getTok().getLoc(), "expected '+' after post-increment GPR");
    return ParseStatus::Failure;
  }
  SMLoc End = Parser.getTok().getEndLoc();
  Parser.Lex();
  Operands.push_back(SHOperand::createPostIncGPR(Base, Start, End));
  return ParseStatus::Success;
}

ParseStatus SHAsmParser::parseOperand(OperandVector &Operands,
                                      StringRef Mnemonic) {
  if ((Mnemonic == "jsr" || Mnemonic == "jmp") &&
      Parser.getTok().is(AsmToken::At)) {
    SMLoc AtLoc = Parser.getTok().getLoc();
    Operands.push_back(SHOperand::createToken("@", AtLoc));
    Parser.Lex();
    MCRegister Reg;
    SMLoc Start;
    SMLoc End;
    if (!tryParseRegister(Reg, Start, End).isSuccess() ||
        !isSHGPR(Parser, Reg)) {
      Error(Parser.getTok().getLoc(), "expected GPR after '@'");
      return ParseStatus::Failure;
    }
    Operands.push_back(SHOperand::createReg(Reg, Start, End));
    return ParseStatus::Success;
  }

  if (Mnemonic == "sts.l") {
    if (Parser.getTok().is(AsmToken::Identifier) &&
        Parser.getTok().getIdentifier() == "pr") {
      SMLoc Start = Parser.getTok().getLoc();
      SMLoc End = Parser.getTok().getEndLoc();
      Operands.push_back(SHOperand::createReg(SH::PR, Start, End));
      Parser.Lex();
      return ParseStatus::Success;
    }
    ParseStatus Result = parsePreDecGPR(Operands);
    if (!Result.isNoMatch())
      return Result;
  }

  if (Mnemonic == "lds.l") {
    if (Parser.getTok().is(AsmToken::Identifier) &&
        Parser.getTok().getIdentifier() == "pr") {
      SMLoc Start = Parser.getTok().getLoc();
      SMLoc End = Parser.getTok().getEndLoc();
      Operands.push_back(SHOperand::createReg(SH::PR, Start, End));
      Parser.Lex();
      return ParseStatus::Success;
    }
    ParseStatus Result = parsePostIncGPR(Operands);
    if (!Result.isNoMatch())
      return Result;
  }

  ParseStatus Result = parseMemory(Operands, Mnemonic);
  if (!Result.isNoMatch())
    return Result;

  Result = MatchOperandParserImpl(Operands, Mnemonic);
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
  std::string FullName;
  if (Parser.getTok().is(AsmToken::Slash)) {
    if (Name != "cmp")
      return Error(NameLoc, "unrecognized instruction mnemonic");
    FullName = Name.str();
    FullName += '/';
    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::Identifier))
      return Error(Parser.getTok().getLoc(),
                   "expected comparison mnemonic suffix");
    FullName += Parser.getTok().getIdentifier();
    Parser.Lex();
    Name = FullName;
  }
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

  if ((Name == "mov.b" || Name == "mov.w") && Operands.size() == 3) {
    const auto *First = static_cast<const SHOperand *>(Operands[1].get());
    const auto *Second = static_cast<const SHOperand *>(Operands[2].get());
    if ((Second->isByteMemDisp() || Second->isWordMemDisp()) && !First->isR0())
      return Error(First->getStartLoc(), "operand must be r0");
    if ((First->isByteMemDisp() || First->isWordMemDisp()) && !Second->isR0())
      return Error(Second->getStartLoc(), "operand must be r0");
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
  case Match_InvalidPCLiteral:
    return Error(Operands[ErrorInfo]->getStartLoc(),
                 "invalid SH PC-relative literal operand");
  case Match_InvalidR0:
    return Error(Operands[ErrorInfo]->getStartLoc(), "operand must be r0");
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
