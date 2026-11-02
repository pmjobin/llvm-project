//===- MCAsmParserTest.cpp - Assembly expression parser tests
//--------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCAsmInfoELF.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "gtest/gtest.h"

using namespace llvm;

TEST(MCAsmParserTest, ConstantExpressionsWithoutTargetParser) {
  Triple TT("unknown-unknown-elf");
  MCTargetOptions Options;
  struct AsmInfo : MCAsmInfoELF {
    explicit AsmInfo(const MCTargetOptions &Options) : MCAsmInfoELF(Options) {}
  } MAI(Options);
  MCRegisterInfo MRI;
  MCSubtargetInfo STI(TT, "", "", "", "", {}, {}, {}, nullptr, nullptr, nullptr,
                      nullptr, nullptr, nullptr, nullptr);
  for (bool Fold : {false, true}) {
    SourceMgr SM;
    SM.AddNewSourceBuffer(MemoryBuffer::getMemBuffer("(16 >> 1) + (1 == 1)"),
                          SMLoc());
    MCContext Ctx(TT, MAI, MRI, STI, &SM);
    std::unique_ptr<MCStreamer> Streamer(createNullStreamer(Ctx));
    std::unique_ptr<MCAsmParser> Parser(
        createMCAsmParser(SM, Ctx, *Streamer, MAI));
    Parser->setFoldExpressions(Fold);
    Parser->Lex();
    const MCExpr *Expr;
    ASSERT_FALSE(Parser->parseExpression(Expr));
    int64_t Value;
    ASSERT_TRUE(Expr->evaluateAsAbsolute(Value));
    EXPECT_EQ(Value, 7);
    EXPECT_EQ(isa<MCConstantExpr>(Expr), Fold);
    if (!Fold) {
      const auto *Binary = cast<MCBinaryExpr>(Expr);
      EXPECT_TRUE(isa<MCBinaryExpr>(Binary->getLHS()));
      EXPECT_TRUE(isa<MCBinaryExpr>(Binary->getRHS()));
    }
  }
}
