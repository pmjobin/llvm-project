//===-- SHAsmParser.cpp - Parse SH assembly instructions -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/SHBaseInfo.h"
#include "MCTargetDesc/SHDSPInfo.h"
#include "MCTargetDesc/SHMCTargetDesc.h"
#include "MCTargetDesc/SHTargetStreamer.h"
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
public:
  enum KindTy {
    Token,
    Register,
    Immediate,
    LongMemReg,
    IndexedMem,
    LongMemDisp,
    ByteMemDisp,
    WordMemDisp,
    ExtendedByteMemDisp,
    ExtendedWordMemDisp,
    ExtendedLongMemDisp,
    ExtendedDoubleMemDisp,
    GBRByteDisp,
    GBRWordDisp,
    GBRLongDisp,
    PreDecGPR,
    PostIncGPR,
    PCLiteral,
    TBRDisp
  };

private:
  KindTy Kind;
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
    return Kind == LongMemReg || Kind == IndexedMem || Kind == LongMemDisp ||
           Kind == ByteMemDisp || Kind == WordMemDisp ||
           Kind == ExtendedByteMemDisp || Kind == ExtendedWordMemDisp ||
           Kind == ExtendedLongMemDisp || Kind == ExtendedDoubleMemDisp ||
           Kind == GBRByteDisp || Kind == GBRWordDisp || Kind == GBRLongDisp ||
           Kind == PreDecGPR || Kind == PostIncGPR;
  }
  bool isLongMemReg() const { return Kind == LongMemReg; }
  bool isIndexedMem() const { return Kind == IndexedMem; }
  bool isTasMemReg() const {
    return Kind == LongMemReg && Reg >= SH::R0 && Reg <= SH::R15;
  }
  bool isLongMemDisp() const { return Kind == LongMemDisp; }
  bool isByteMemDisp() const { return Kind == ByteMemDisp; }
  bool isWordMemDisp() const { return Kind == WordMemDisp; }
  bool isExtendedByteMemDisp() const { return Kind == ExtendedByteMemDisp; }
  bool isExtendedWordMemDisp() const { return Kind == ExtendedWordMemDisp; }
  bool isExtendedLongMemDisp() const { return Kind == ExtendedLongMemDisp; }
  bool isExtendedDoubleMemDisp() const { return Kind == ExtendedDoubleMemDisp; }
  bool isGBRByteDisp() const { return Kind == GBRByteDisp; }
  bool isGBRWordDisp() const { return Kind == GBRWordDisp; }
  bool isGBRLongDisp() const { return Kind == GBRLongDisp; }
  bool isPreDecGPR() const { return Kind == PreDecGPR; }
  bool isPostIncGPR() const { return Kind == PostIncGPR; }
  bool isPreDecR15() const { return Kind == PreDecGPR && Reg == SH::R15; }
  bool isPostIncR15() const { return Kind == PostIncGPR && Reg == SH::R15; }
  bool isPCLiteral() const { return Kind == PCLiteral; }
  bool isPCWordLiteral() const { return Kind == PCLiteral; }
  bool isR0() const { return isReg() && Reg == SH::R0; }
  bool isR0GBR() const { return isReg() && Reg == SH::GBR; }
  bool isBranchTarget() const { return isImm(); }
  bool isTBRDisp() const { return Kind == TBRDisp; }

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

  bool isUImm8() const {
    const auto *CE = dyn_cast_if_present<MCConstantExpr>(Expr);
    return isImm() && CE && isUInt<8>(CE->getValue());
  }

  bool isUImm3() const {
    const auto *CE = dyn_cast_if_present<MCConstantExpr>(Expr);
    return isImm() && CE && isUInt<3>(CE->getValue());
  }

  bool isSImm20() const {
    const auto *CE = dyn_cast_if_present<MCConstantExpr>(Expr);
    return isImm() && ((!CE && Expr) || (CE && isInt<20>(CE->getValue())));
  }

  bool isSImm20By8() const {
    const auto *CE = dyn_cast_if_present<MCConstantExpr>(Expr);
    return isImm() && CE && isInt<28>(CE->getValue()) &&
           CE->getValue() % 256 == 0;
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
      if (Kind == LongMemDisp || Kind == ByteMemDisp || Kind == WordMemDisp ||
          Kind == ExtendedByteMemDisp || Kind == ExtendedWordMemDisp ||
          Kind == ExtendedLongMemDisp || Kind == ExtendedDoubleMemDisp ||
          Kind == GBRByteDisp || Kind == GBRWordDisp || Kind == GBRLongDisp) {
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

  void addSImm20Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1);
    if (const auto *CE = dyn_cast<MCConstantExpr>(Expr))
      Inst.addOperand(MCOperand::createImm(CE->getValue()));
    else
      Inst.addOperand(MCOperand::createExpr(Expr));
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

  void addIndexedMemOperands(MCInst &Inst, unsigned N) const {
    assert(Kind == IndexedMem && N == 1);
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

  void addExtendedMemDispOperands(MCInst &Inst, unsigned N) const {
    assert((isExtendedByteMemDisp() || isExtendedWordMemDisp() ||
            isExtendedLongMemDisp() || isExtendedDoubleMemDisp()) &&
           N == 2);
    Inst.addOperand(MCOperand::createReg(Reg));
    Inst.addOperand(
        MCOperand::createImm(cast<MCConstantExpr>(Expr)->getValue()));
  }

  void addTBRDispOperands(MCInst &Inst, unsigned N) const {
    assert(Kind == TBRDisp && N == 1);
    Inst.addOperand(
        MCOperand::createImm(cast<MCConstantExpr>(Expr)->getValue()));
  }

  void addGBRDispOperands(MCInst &Inst, unsigned N) const {
    assert(
        (Kind == GBRByteDisp || Kind == GBRWordDisp || Kind == GBRLongDisp) &&
        N == 1);
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

  static std::unique_ptr<SHOperand> createIndexedMem(MCRegister Base,
                                                     SMLoc Start, SMLoc End) {
    auto Op = std::unique_ptr<SHOperand>(new SHOperand(IndexedMem));
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

  static std::unique_ptr<SHOperand>
  createExtendedMemDisp(KindTy Kind, MCRegister Base, const MCExpr *Disp,
                        SMLoc Start, SMLoc End) {
    assert((Kind == ExtendedByteMemDisp || Kind == ExtendedWordMemDisp ||
            Kind == ExtendedLongMemDisp || Kind == ExtendedDoubleMemDisp) &&
           "invalid extended memory operand kind");
    auto Op = std::unique_ptr<SHOperand>(new SHOperand(Kind));
    Op->Reg = Base;
    Op->Expr = Disp;
    Op->StartLoc = Start;
    Op->EndLoc = End;
    return Op;
  }

  void promoteToExtendedMemDisp(KindTy ExtendedKind) {
    assert((Kind == ByteMemDisp || Kind == WordMemDisp) &&
           "only a narrow displacement can be promoted");
    Kind = ExtendedKind;
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

  static std::unique_ptr<SHOperand>
  createGBRDisp(KindTy Kind, const MCExpr *Disp, SMLoc Start, SMLoc End) {
    assert(
        (Kind == GBRByteDisp || Kind == GBRWordDisp || Kind == GBRLongDisp) &&
        "invalid GBR displacement kind");
    auto Op = std::unique_ptr<SHOperand>(new SHOperand(Kind));
    Op->Expr = Disp;
    Op->StartLoc = Start;
    Op->EndLoc = End;
    return Op;
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

  static std::unique_ptr<SHOperand> createTBRDisp(const MCExpr *Expr,
                                                  SMLoc Start, SMLoc End) {
    auto Op = std::unique_ptr<SHOperand>(new SHOperand(TBRDisp));
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
  ParseStatus parseDirective(AsmToken DirectiveID) override;

  ParseStatus parseOperand(OperandVector &Operands, StringRef Mnemonic);
  bool parseDSPInstruction(StringRef Name, SMLoc NameLoc,
                           OperandVector &Operands);
  bool parseDSPRepeat(SMLoc NameLoc, OperandVector &Operands);
  bool
  parseDSPSubInstruction(StringRef Name,
                         SmallVectorImpl<SH::DSPSubInstruction> &Instructions);
  bool parseDSPArgument(SH::DSPArg &Arg);
  ParseStatus parseBranchTarget(OperandVector &Operands);
  ParseStatus parsePCLiteral(OperandVector &Operands);
  ParseStatus parsePCWordLiteral(OperandVector &Operands);
  ParseStatus parseSImm8(OperandVector &Operands);
  ParseStatus parseUImm8(OperandVector &Operands);
  ParseStatus parseUImm3(OperandVector &Operands);
  ParseStatus parseSImm20(OperandVector &Operands);
  ParseStatus parseSImm20By8(OperandVector &Operands);
  ParseStatus parseTBRDisp(OperandVector &Operands);
  ParseStatus parseMemory(OperandVector &Operands, StringRef Mnemonic);
  ParseStatus parsePreDecGPR(OperandVector &Operands);
  ParseStatus parseUnalignedDataDirective(unsigned Size);

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

ParseStatus SHAsmParser::parseUnalignedDataDirective(unsigned Size) {
  SMLoc Loc = Parser.getTok().getLoc();
  if (Parser.getTok().is(AsmToken::EndOfStatement)) {
    Error(Loc, "expected expression");
    return ParseStatus::Failure;
  }

  const MCExpr *Value;
  if (Parser.parseExpression(Value))
    return ParseStatus::Failure;
  if (Parser.parseEOL())
    return ParseStatus::Failure;

  auto &TargetStreamer = static_cast<SHTargetStreamer &>(
      *Parser.getStreamer().getTargetStreamer());
  TargetStreamer.emitUnalignedDataValue(Value, Size, Loc);
  return ParseStatus::Success;
}

ParseStatus SHAsmParser::parseDirective(AsmToken DirectiveID) {
  StringRef Directive = DirectiveID.getString();
  if (Directive == ".uaword")
    return parseUnalignedDataDirective(2);
  if (Directive == ".ualong")
    return parseUnalignedDataDirective(4);
  if (Directive == ".uaquad")
    return parseUnalignedDataDirective(8);
  return ParseStatus::NoMatch;
}

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

ParseStatus SHAsmParser::parseUImm8(OperandVector &Operands) {
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

ParseStatus SHAsmParser::parseUImm3(OperandVector &Operands) {
  return parseUImm8(Operands);
}

ParseStatus SHAsmParser::parseSImm20(OperandVector &Operands) {
  SMLoc Start = Parser.getTok().getLoc();
  Parser.Lex();
  const MCExpr *Expr;
  SMLoc End;
  if (Parser.parseExpression(Expr, End))
    return ParseStatus::Failure;
  Operands.push_back(SHOperand::createImm(Expr, Start, End));
  return ParseStatus::Success;
}

ParseStatus SHAsmParser::parseSImm20By8(OperandVector &Operands) {
  return parseSImm8(Operands);
}

ParseStatus SHAsmParser::parseTBRDisp(OperandVector &Operands) {
  if (Parser.getTok().isNot(AsmToken::At))
    return ParseStatus::NoMatch;
  SMLoc Start = Parser.getTok().getLoc();
  Parser.Lex();
  if (Parser.getTok().isNot(AsmToken::At))
    return ParseStatus::NoMatch;
  Parser.Lex();
  if (Parser.getTok().isNot(AsmToken::LParen)) {
    Error(Parser.getTok().getLoc(), "expected '(' after '@@'");
    return ParseStatus::Failure;
  }
  Parser.Lex();
  const MCExpr *Expr;
  SMLoc End;
  if (Parser.parseExpression(Expr, End))
    return ParseStatus::Failure;
  const auto *Constant = dyn_cast<MCConstantExpr>(Expr);
  if (!Constant || Constant->getValue() < 0 || Constant->getValue() > 1020 ||
      Constant->getValue() % 4 != 0) {
    Error(Start,
          "TBR displacement must be a multiple of 4 in the range [0, 1020]");
    return ParseStatus::Failure;
  }
  if (Parser.getTok().isNot(AsmToken::Comma)) {
    Error(Parser.getTok().getLoc(), "expected comma in TBR displacement");
    return ParseStatus::Failure;
  }
  Parser.Lex();
  if (Parser.getTok().isNot(AsmToken::Identifier) ||
      Parser.getTok().getIdentifier() != "tbr") {
    Error(Parser.getTok().getLoc(), "expected tbr");
    return ParseStatus::Failure;
  }
  Parser.Lex();
  if (Parser.getTok().isNot(AsmToken::RParen)) {
    Error(Parser.getTok().getLoc(), "expected ')' in TBR displacement");
    return ParseStatus::Failure;
  }
  End = Parser.getTok().getEndLoc();
  Parser.Lex();
  Operands.push_back(SHOperand::createTBRDisp(Expr, Start, End));
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
  bool IsDisplacement = false;
  if (getSTI().getFeatureBits()[SH::FeatureDSP]) {
    if (const auto *Constant = dyn_cast<MCConstantExpr>(Expr)) {
      Expr = MCConstantExpr::create(Constant->getValue() - SH::PCRelativeBias,
                                    Parser.getContext());
      IsDisplacement = true;
    }
  }
  Operands.push_back(
      SHOperand::createPCLiteral(Expr, IsDisplacement, Start, End));
  return ParseStatus::Success;
}

ParseStatus SHAsmParser::parsePCWordLiteral(OperandVector &Operands) {
  return parsePCLiteral(Operands);
}

ParseStatus SHAsmParser::parseMemory(OperandVector &Operands,
                                     StringRef Mnemonic) {
  if (Parser.getTok().isNot(AsmToken::At))
    return ParseStatus::NoMatch;

  SMLoc Start = Parser.getTok().getLoc();
  Parser.Lex();

  if (Parser.getTok().is(AsmToken::LParen)) {
    Parser.Lex();

    if (Parser.getTok().is(AsmToken::Identifier)) {
      SMLoc IndexLoc = Parser.getTok().getLoc();
      MCRegister Index = MatchRegisterName(Parser.getTok().getIdentifier());
      if (Index) {
        Parser.Lex();
        if (Index != SH::R0) {
          Error(IndexLoc, "SH indexed memory requires r0 as its index");
          return ParseStatus::Failure;
        }
        if (Parser.getTok().isNot(AsmToken::Comma)) {
          Error(Parser.getTok().getLoc(),
                "expected comma in indexed memory operand");
          return ParseStatus::Failure;
        }
        Parser.Lex();
        MCRegister Base;
        SMLoc RegStart;
        SMLoc RegEnd;
        if (!tryParseRegister(Base, RegStart, RegEnd).isSuccess()) {
          Error(Parser.getTok().getLoc(),
                "expected register base in indexed memory operand");
          return ParseStatus::Failure;
        }
        if (Parser.getTok().isNot(AsmToken::RParen)) {
          Error(Parser.getTok().getLoc(),
                "expected ')' in indexed memory operand");
          return ParseStatus::Failure;
        }
        SMLoc End = Parser.getTok().getEndLoc();
        Parser.Lex();
        if (Base == SH::GBR) {
          if (Mnemonic != "and.b" && Mnemonic != "or.b" &&
              Mnemonic != "tst.b" && Mnemonic != "xor.b") {
            Error(Start, "@(r0,gbr) is only valid for byte logical operations");
            return ParseStatus::Failure;
          }
          Operands.push_back(SHOperand::createReg(Base, Start, End));
          return ParseStatus::Success;
        }
        if (!isSHGPR(Parser, Base)) {
          Error(RegStart, "expected GPR base in indexed memory operand");
          return ParseStatus::Failure;
        }
        if (Mnemonic != "mov.b" && Mnemonic != "mov.w" && Mnemonic != "mov.l" &&
            Mnemonic != "fmov" && Mnemonic != "fmov.s" &&
            Mnemonic != "fmov.d") {
          Error(Start, "indexed memory is only valid for move instructions");
          return ParseStatus::Failure;
        }
        Operands.push_back(SHOperand::createIndexedMem(Base, Start, End));
        return ParseStatus::Success;
      }
    }

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
      if (getSTI().getFeatureBits()[SH::FeatureSH2A] ||
          getSTI().getFeatureBits()[SH::FeatureDSP]) {
        ByteDisp -= SH::PCRelativeBias;
        Disp = MCConstantExpr::create(ByteDisp, Parser.getContext());
      }
      if (!SH::isPCRelativeLongDisplacementInRange(ByteDisp)) {
        Error(Start, "SH PC-relative literal displacement must be a multiple "
                     "of 4 in the range [0, 1020]");
        return ParseStatus::Failure;
      }
      Operands.push_back(SHOperand::createPCLiteral(Disp, true, Start, End));
    } else if ((Mnemonic == "mov.w" || Mnemonic == "ldre" ||
                Mnemonic == "ldrs") &&
               Base == SH::PC) {
      if (getSTI().getFeatureBits()[SH::FeatureSH2A] ||
          getSTI().getFeatureBits()[SH::FeatureDSP]) {
        ByteDisp -= SH::PCRelativeBias;
        Disp = MCConstantExpr::create(ByteDisp, Parser.getContext());
      }
      if (!SH::isPCRelativeWordDisplacementInRange(ByteDisp)) {
        Error(Start, "SH PC-relative word displacement must be an even byte "
                     "offset in the range [0, 510]");
        return ParseStatus::Failure;
      }
      Operands.push_back(SHOperand::createPCLiteral(Disp, true, Start, End));
    } else if (Base == SH::PC) {
      Error(Start, "pc is only valid for PC-relative operands");
      return ParseStatus::Failure;
    } else if (Base == SH::GBR) {
      if (Mnemonic == "mov.b") {
        if (ByteDisp < 0 || ByteDisp > 255) {
          Error(Start, "GBR byte displacement must be in the range [0, 255]");
          return ParseStatus::Failure;
        }
        Operands.push_back(
            SHOperand::createGBRDisp(SHOperand::GBRByteDisp, Disp, Start, End));
      } else if (Mnemonic == "mov.w") {
        if (ByteDisp < 0 || ByteDisp > 510 || ByteDisp % 2 != 0) {
          Error(Start, "GBR word displacement must be an even byte offset in "
                       "the range [0, 510]");
          return ParseStatus::Failure;
        }
        Operands.push_back(
            SHOperand::createGBRDisp(SHOperand::GBRWordDisp, Disp, Start, End));
      } else if (Mnemonic == "mov.l") {
        if (ByteDisp < 0 || ByteDisp > 1020 || ByteDisp % 4 != 0) {
          Error(Start, "GBR longword displacement must be a multiple of 4 in "
                       "the range [0, 1020]");
          return ParseStatus::Failure;
        }
        Operands.push_back(
            SHOperand::createGBRDisp(SHOperand::GBRLongDisp, Disp, Start, End));
      } else {
        Error(Start, "GBR displacement is only valid for move instructions");
        return ParseStatus::Failure;
      }
    } else if (!isSHGPR(Parser, Base)) {
      Error(RegStart, "expected GPR memory base");
      return ParseStatus::Failure;
    } else if (Mnemonic == "mov.b" || Mnemonic == "movu.b" ||
               Mnemonic == "band.b" || Mnemonic == "bandnot.b" ||
               Mnemonic == "bclr.b" || Mnemonic == "bld.b" ||
               Mnemonic == "bldnot.b" || Mnemonic == "bor.b" ||
               Mnemonic == "bornot.b" || Mnemonic == "bset.b" ||
               Mnemonic == "bst.b" || Mnemonic == "bxor.b") {
      bool HasSH2A = getSTI().getFeatureBits()[SH::FeatureSH2A];
      int64_t Maximum = Mnemonic == "mov.b" && !HasSH2A ? 15 : 4095;
      if (ByteDisp < 0 || ByteDisp > Maximum) {
        Error(Start, Maximum == 15
                         ? "byte displacement must be in the range [0, 15]"
                         : "byte displacement must be in the range [0, 4095]");
        return ParseStatus::Failure;
      }
      if (Mnemonic == "mov.b" && ByteDisp <= 15)
        Operands.push_back(
            SHOperand::createByteMemDisp(Base, Disp, Start, End));
      else
        Operands.push_back(SHOperand::createExtendedMemDisp(
            SHOperand::ExtendedByteMemDisp, Base, Disp, Start, End));
    } else if (Mnemonic == "mov.w" || Mnemonic == "movu.w") {
      bool HasSH2A = getSTI().getFeatureBits()[SH::FeatureSH2A];
      int64_t Maximum = Mnemonic == "mov.w" && !HasSH2A ? 30 : 8190;
      if (ByteDisp < 0 || ByteDisp > Maximum || ByteDisp % 2 != 0) {
        Error(Start,
              Twine("word displacement must be an even byte offset in the ") +
                  (Maximum == 30 ? "range [0, 30]" : "range [0, 8190]"));
        return ParseStatus::Failure;
      }
      if (Mnemonic == "mov.w" && ByteDisp <= 30)
        Operands.push_back(
            SHOperand::createWordMemDisp(Base, Disp, Start, End));
      else
        Operands.push_back(SHOperand::createExtendedMemDisp(
            SHOperand::ExtendedWordMemDisp, Base, Disp, Start, End));
    } else if (Mnemonic == "fmov.s") {
      if (ByteDisp < 0 || ByteDisp > 16380 || ByteDisp % 4 != 0) {
        Error(Start, "single-precision displacement must be a multiple of 4 "
                     "in the range [0, 16380]");
        return ParseStatus::Failure;
      }
      Operands.push_back(SHOperand::createExtendedMemDisp(
          SHOperand::ExtendedLongMemDisp, Base, Disp, Start, End));
    } else if (Mnemonic == "fmov.d") {
      if (ByteDisp < 0 || ByteDisp > 32760 || ByteDisp % 8 != 0) {
        Error(Start, "double-precision displacement must be a multiple of 8 "
                     "in the range [0, 32760]");
        return ParseStatus::Failure;
      }
      Operands.push_back(SHOperand::createExtendedMemDisp(
          SHOperand::ExtendedDoubleMemDisp, Base, Disp, Start, End));
    } else {
      int64_t Maximum = getSTI().getFeatureBits()[SH::FeatureSH2A] ? 16380 : 60;
      if (ByteDisp < 0 || ByteDisp > Maximum || ByteDisp % 4 != 0) {
        Error(Start,
              Twine("longword displacement must be a multiple of 4 in the ") +
                  (Maximum == 60 ? "range [0, 60]" : "range [0, 16380]"));
        return ParseStatus::Failure;
      }
      if (Mnemonic == "mov.l" && ByteDisp > 60)
        Operands.push_back(SHOperand::createExtendedMemDisp(
            SHOperand::ExtendedLongMemDisp, Base, Disp, Start, End));
      else
        Operands.push_back(
            SHOperand::createLongMemDisp(Base, Disp, Start, End));
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
  if (!isSHGPR(Parser, Base)) {
    Error(RegStart, "expected GPR memory base");
    return ParseStatus::Failure;
  }
  if (Parser.getTok().is(AsmToken::Plus)) {
    SMLoc End = Parser.getTok().getEndLoc();
    Parser.Lex();
    Operands.push_back(SHOperand::createPostIncGPR(Base, Start, End));
    return ParseStatus::Success;
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

ParseStatus SHAsmParser::parseOperand(OperandVector &Operands,
                                      StringRef Mnemonic) {
  if (Mnemonic == "jsr/n" && Parser.getTok().is(AsmToken::At) &&
      Parser.getLexer().peekTok().is(AsmToken::At))
    return parseTBRDisp(Operands);

  if ((Mnemonic == "jsr" || Mnemonic == "jsr/n" || Mnemonic == "jmp") &&
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

  if (Parser.getTok().is(AsmToken::At) &&
      Parser.getLexer().peekTok().is(AsmToken::Minus))
    return parsePreDecGPR(Operands);

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

bool SHAsmParser::parseDSPArgument(SH::DSPArg &Arg) {
  if (Parser.getTok().is(AsmToken::Hash)) {
    SMLoc Loc = Parser.getTok().getLoc();
    Parser.Lex();
    const MCExpr *Expr;
    if (Parser.parseExpression(Expr))
      return true;
    const auto *Constant = dyn_cast<MCConstantExpr>(Expr);
    if (!Constant)
      return Error(Loc, "DSP immediate must be an integer constant");
    Arg.Kind = SH::DSPArgKind::Immediate;
    Arg.Immediate = Constant->getValue();
    return false;
  }

  if (Parser.getTok().is(AsmToken::At)) {
    Parser.Lex();
    Arg.Kind = SH::DSPArgKind::Memory;
    if (Parser.getTok().is(AsmToken::Minus)) {
      Arg.Update = SH::DSPUpdate::PreDecrement;
      Parser.Lex();
    }
    if (Parser.getTok().isNot(AsmToken::Identifier) ||
        !SH::getDSPRegister(Parser.getTok().getIdentifier(), Arg.Reg) ||
        Arg.Reg > SH::DSP_R15)
      return Error(Parser.getTok().getLoc(),
                   "expected GPR in DSP memory operand");
    Parser.Lex();
    if (Arg.Update == SH::DSPUpdate::PreDecrement)
      return false;
    if (Parser.getTok().isNot(AsmToken::Plus))
      return false;
    Parser.Lex();
    Arg.Update = SH::DSPUpdate::PostIncrement;
    if (Parser.getTok().is(AsmToken::Identifier)) {
      unsigned Index;
      if (SH::getDSPRegister(Parser.getTok().getIdentifier(), Index) &&
          (Index == SH::DSP_R8 || Index == SH::DSP_R9)) {
        Arg.IndexReg = Index;
        Arg.Update = SH::DSPUpdate::Modulo;
        Parser.Lex();
      }
    }
    return false;
  }

  if (Parser.getTok().isNot(AsmToken::Identifier))
    return Error(Parser.getTok().getLoc(),
                 "expected DSP register or memory operand");
  if (!SH::getDSPRegister(Parser.getTok().getIdentifier(), Arg.Reg))
    return Error(Parser.getTok().getLoc(), "invalid DSP register name");
  Arg.Kind = SH::DSPArgKind::Register;
  Parser.Lex();
  return false;
}

bool SHAsmParser::parseDSPSubInstruction(
    StringRef Name, SmallVectorImpl<SH::DSPSubInstruction> &Instructions) {
  SH::DSPSubInstruction Instruction;
  Instruction.Mnemonic = Name.lower();
  unsigned Count =
      StringSwitch<unsigned>(StringRef(Instruction.Mnemonic))
          .Cases({"nopx", "nopy", "dct", "dcf"}, 0)
          .Case("pclr", 1)
          .Cases({"pabs", "prnd", "pdec", "pinc", "pdmsb", "pneg", "pcopy",
                  "pswap", "psts", "plds", "pcmp"},
                 2)
          .Cases({"movs.w", "movs.l", "movx.w", "movx.l", "movy.w", "movy.l"},
                 2)
          .Cases({"pmuls", "psubc", "paddc", "pwsb", "pwad", "psub", "padd",
                  "pand", "pxor", "por"},
                 3)
          .Cases({"pshl", "psha"}, Parser.getTok().is(AsmToken::Hash) ? 2 : 3)
          .Default(~0U);
  if (Count == ~0U)
    return Error(Parser.getTok().getLoc(), "unrecognized DSP subinstruction");
  for (unsigned I = 0; I != Count; ++I) {
    if (I) {
      if (Parser.getTok().isNot(AsmToken::Comma))
        return Error(Parser.getTok().getLoc(),
                     "expected comma in DSP subinstruction");
      Parser.Lex();
    }
    SH::DSPArg Arg;
    if (parseDSPArgument(Arg))
      return true;
    Instruction.Args.push_back(Arg);
  }
  Instructions.push_back(std::move(Instruction));
  return false;
}

bool SHAsmParser::parseDSPInstruction(StringRef Name, SMLoc NameLoc,
                                      OperandVector &Operands) {
  SmallVector<SH::DSPSubInstruction, 4> Instructions;
  if (parseDSPSubInstruction(Name, Instructions))
    return true;
  while (Parser.getTok().isNot(AsmToken::EndOfStatement)) {
    if (Parser.getTok().isNot(AsmToken::Identifier))
      return Error(Parser.getTok().getLoc(),
                   "expected a DSP parallel subinstruction");
    std::string NextName = Parser.getTok().getIdentifier().lower();
    SMLoc NextLoc = Parser.getTok().getLoc();
    Parser.Lex();
    if (!SH::isDSPCompoundMnemonic(NextName))
      return Error(NextLoc, "unrecognized DSP parallel subinstruction");
    if (parseDSPSubInstruction(NextName, Instructions))
      return true;
  }

  uint32_t Encoding;
  unsigned Size;
  std::string Message;
  if (!SH::encodeDSPInstruction(Instructions, Encoding, Size, Message))
    return Error(NameLoc, Message);
  StringRef InternalName =
      Size == 4 ? "dsp.ppi"
      : StringRef(Instructions.front().Mnemonic).starts_with("movs.")
          ? "dsp.sdt"
          : "dsp.ddt";
  unsigned Payload = Size == 4 ? Encoding & 0x3ffffff : Encoding & 0x3ff;
  Operands.push_back(SHOperand::createToken(InternalName, NameLoc));
  const MCExpr *Expr = MCConstantExpr::create(Payload, Parser.getContext());
  Operands.push_back(SHOperand::createImm(Expr, NameLoc, NameLoc));
  Parser.Lex();
  return false;
}

bool SHAsmParser::parseDSPRepeat(SMLoc NameLoc, OperandVector &Operands) {
  Operands.push_back(SHOperand::createToken("repeat", NameLoc));
  for (unsigned I = 0; I != 2; ++I) {
    const MCExpr *Expr;
    SMLoc Start = Parser.getTok().getLoc();
    if (Parser.parseExpression(Expr))
      return true;
    Operands.push_back(
        SHOperand::createImm(Expr, Start, Parser.getTok().getLoc()));
    if (Parser.getTok().is(AsmToken::Comma))
      Parser.Lex();
  }

  if (Parser.getTok().is(AsmToken::Hash)) {
    SMLoc Start = Parser.getTok().getLoc();
    Parser.Lex();
    const MCExpr *Expr;
    if (Parser.parseExpression(Expr))
      return true;
    const auto *Constant = dyn_cast<MCConstantExpr>(Expr);
    if (!Constant || !isUInt<8>(Constant->getValue()))
      return Error(Start,
                   "repeat count must be an integer in the range [0, 255]");
    Operands.push_back(
        SHOperand::createImm(Expr, Start, Parser.getTok().getLoc()));
  } else {
    MCRegister Reg;
    SMLoc Start;
    SMLoc End;
    if (!tryParseRegister(Reg, Start, End).isSuccess() || !isSHGPR(Parser, Reg))
      return Error(Parser.getTok().getLoc(),
                   "expected GPR or immediate repeat count");
    Operands.push_back(SHOperand::createReg(Reg, Start, End));
  }
  if (Parser.getTok().isNot(AsmToken::EndOfStatement))
    return Error(Parser.getTok().getLoc(), "unexpected operand in repeat");
  Parser.Lex();
  return false;
}

bool SHAsmParser::parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                                   SMLoc NameLoc, OperandVector &Operands) {
  std::string FullName;
  if (Name == "bt.s")
    Name = "bt/s";
  else if (Name == "bf.s")
    Name = "bf/s";
  else if (Name == "muls")
    Name = "muls.w";
  else if (Name == "mulu")
    Name = "mulu.w";
  if (Name == "dsp.ddt" || Name == "dsp.sdt" || Name == "dsp.ppi")
    return Error(NameLoc, "unrecognized instruction mnemonic");
  bool ParseDSP = getSTI().getFeatureBits()[SH::FeatureDSP] ||
                  getSTI().getCPU() == "sh4al-dsp";
  if (Name == "repeat" && ParseDSP)
    return parseDSPRepeat(NameLoc, Operands);
  if (SH::isDSPCompoundMnemonic(Name)) {
    unsigned DSPReg;
    bool HasDSPOperand =
        Parser.getTok().is(AsmToken::At) ||
        Parser.getTok().is(AsmToken::Hash) ||
        (Parser.getTok().is(AsmToken::Identifier) &&
         SH::getDSPRegister(Parser.getTok().getIdentifier(), DSPReg) &&
         DSPReg > SH::DSP_R15);
    bool IsDSPPrefix =
        Name == "nopx" || Name == "nopy" || Name == "dct" || Name == "dcf";
    if (ParseDSP || HasDSPOperand || IsDSPPrefix)
      return parseDSPInstruction(Name, NameLoc, Operands);
  }
  if (Parser.getTok().is(AsmToken::Slash)) {
    if (Name != "cmp" && Name != "fcmp" && Name != "bf" && Name != "bt" &&
        Name != "jsr" && Name != "rts" && Name != "rtv")
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
    auto *First = static_cast<SHOperand *>(Operands[1].get());
    auto *Second = static_cast<SHOperand *>(Operands[2].get());
    auto Promote = [&](SHOperand *Memory) {
      Memory->promoteToExtendedMemDisp(Name == "mov.b"
                                           ? SHOperand::ExtendedByteMemDisp
                                           : SHOperand::ExtendedWordMemDisp);
    };
    if ((Second->isByteMemDisp() || Second->isWordMemDisp()) &&
        !First->isR0()) {
      if (!getSTI().getFeatureBits()[SH::FeatureSH2A])
        return Error(First->getStartLoc(), "operand must be r0");
      Promote(Second);
    }
    if ((First->isByteMemDisp() || First->isWordMemDisp()) && !Second->isR0()) {
      if (!getSTI().getFeatureBits()[SH::FeatureSH2A])
        return Error(Second->getStartLoc(), "operand must be r0");
      Promote(First);
    }
  }

  Parser.Lex();
  return false;
}

bool SHAsmParser::matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                          OperandVector &Operands,
                                          MCStreamer &Out, uint64_t &ErrorInfo,
                                          bool MatchingInlineAsm) {
  const auto *Mnemonic = static_cast<const SHOperand *>(Operands[0].get());
  if (Mnemonic->getToken() == "repeat") {
    if (!getSTI().getFeatureBits()[SH::FeatureDSP])
      return Error(IDLoc, "instruction requires an unavailable feature");
    auto *Start = static_cast<SHOperand *>(Operands[1].get());
    auto *End = static_cast<SHOperand *>(Operands[2].get());
    for (unsigned LoopOpcode : {SH::DSP_LOOP_START, SH::DSP_LOOP_END}) {
      MCInst Bound;
      Bound.setOpcode(LoopOpcode);
      Bound.setLoc(IDLoc);
      Bound.addOperand(MCOperand::createExpr(Start->getImm()));
      Bound.addOperand(MCOperand::createExpr(End->getImm()));
      Out.emitInstruction(Bound, getSTI());
    }
    MCInst Count;
    Count.setOpcode(Operands[3]->isReg() ? SH::SETRC_REG : SH::SETRC_IMM);
    Count.setLoc(IDLoc);
    if (Operands[3]->isReg())
      Count.addOperand(MCOperand::createReg(Operands[3]->getReg()));
    else
      Count.addOperand(MCOperand::createImm(
          cast<MCConstantExpr>(
              static_cast<SHOperand *>(Operands[3].get())->getImm())
              ->getValue()));
    Out.emitInstruction(Count, getSTI());
    return false;
  }
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
  case Match_InvalidUImm8:
    return Error(Operands[ErrorInfo]->getStartLoc(),
                 "immediate must be an integer in the range [0, 255]");
  case Match_InvalidUImm3:
    return Error(Operands[ErrorInfo]->getStartLoc(),
                 "immediate must be an integer in the range [0, 7]");
  case Match_InvalidSImm20:
    return Error(Operands[ErrorInfo]->getStartLoc(),
                 "immediate must be an integer in the signed 20-bit range");
  case Match_InvalidSImm20By8:
    return Error(Operands[ErrorInfo]->getStartLoc(),
                 "immediate must be a signed 28-bit multiple of 256");
  case Match_InvalidTBRDisp:
    return Error(Operands[ErrorInfo]->getStartLoc(),
                 "invalid SH-2A TBR displacement");
  case Match_InvalidPCLiteral:
    return Error(Operands[ErrorInfo]->getStartLoc(),
                 "invalid SH PC-relative literal operand");
  case Match_InvalidPCWordLiteral:
    return Error(Operands[ErrorInfo]->getStartLoc(),
                 "invalid SH PC-relative word literal operand");
  case Match_InvalidIndexedMem:
    if (ErrorInfo < Operands.size() && !Operands[ErrorInfo]->isMem() &&
        static_cast<const SHOperand *>(Operands[0].get())
            ->getToken()
            .starts_with("f"))
      return Error(Operands[ErrorInfo]->getStartLoc(),
                   "invalid operand for instruction");
    return Error(Operands[ErrorInfo]->getStartLoc(),
                 "invalid SH indexed memory operand");
  case Match_InvalidR0:
    return Error(Operands[ErrorInfo]->getStartLoc(), "operand must be r0");
  case Match_InvalidR0GBR:
    return Error(Operands[ErrorInfo]->getStartLoc(),
                 "operand must be @(r0,gbr)");
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
