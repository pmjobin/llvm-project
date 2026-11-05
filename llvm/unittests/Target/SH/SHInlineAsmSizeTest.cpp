//===- SHInlineAsmSizeTest.cpp - SH inline assembly sizing tests ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHInlineAsmSize.h"
#include "MCTargetDesc/SHInstPrinter.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/Twine.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/TargetParser/Triple.h"

#include "gtest/gtest.h"

using namespace llvm;

namespace {

void expectSize(StringRef Asm, uint64_t Bytes,
                SHInlineAsmSizeKind Kind = SHInlineAsmSizeKind::Exact) {
  SHInlineAsmSize Size = getSHInlineAsmSize(Asm);
  EXPECT_EQ(Kind, Size.Kind) << Size.Diagnostic;
  EXPECT_EQ(Bytes, Size.Bytes) << Size.Diagnostic;
  EXPECT_TRUE(Size.Diagnostic.empty());
}

void expectFailure(StringRef Asm, SHInlineAsmSizeKind Kind,
                   StringRef Diagnostic) {
  SHInlineAsmSize Size = getSHInlineAsmSize(Asm);
  EXPECT_EQ(Kind, Size.Kind);
  EXPECT_EQ(0u, Size.Bytes);
  EXPECT_TRUE(StringRef(Size.Diagnostic).contains(Diagnostic))
      << Size.Diagnostic;
}

void expectSelected(StringRef Asm, StringRef Expected) {
  std::string Diagnostic;
  std::optional<std::string> Selected =
      selectSHInlineAsmAlternative(Asm, Diagnostic);
  ASSERT_TRUE(Selected) << Diagnostic;
  EXPECT_EQ(Expected, *Selected);
}

std::string nestExpression(StringRef Prefix, StringRef Suffix, unsigned Depth,
                           StringRef Value = "1") {
  std::string Expression;
  for (unsigned I = 0; I != Depth; ++I)
    Expression += Prefix;
  Expression += Value;
  for (unsigned I = 0; I != Depth; ++I)
    Expression += Suffix;
  return Expression;
}

TEST(SHInlineAsmSizeTest, SizesAllSupportedInstructionsExactly) {
  expectSize("nop\nadd r0,r1;mov.l @r2,r3\nrts\nnop\nbt 1f\n1: clrt", 14);
  expectSize("add $2,$0\nmov #$1,$0\nmov.l @$0,$1", 6);
  expectSize("rts\nnop\njsr @r4\nnop", 8);
}

TEST(SHInlineAsmSizeTest, SizesSH2AMixedWidthInstructionsExactly) {
  expectSize("movi20 #1,r1\nmovi20s #256,r2", 8);
  expectSize("mov.b r1,@(0,r2)\nmov.w @(2,r3),r4\nmov.l r5,@(64,r6)", 12);
  expectSize("mov.b r0,@(15,r2)\nmov.w @(30,r3),r0\nmov.l r5,@(60,r6)", 6);
  expectSize("fmov.s fr0,@(4,r1)\nfmov.d @(8,r2),dr0", 8);
  expectSize("bclr #1,r1\nbclr.b #1,@(1,r1)", 6);
  expectSize("bt.s 1f\nmuls r1,r2\n1: rts/n", 6);
  expectSize(".rept 3\nmovi20 #1,r1\nnop\n.endr", 18);
  expectFailure("mov.b $0,@(1,r1)", SHInlineAsmSizeKind::Indeterminate,
                "operand-dependent mixed-width");
  expectFailure("mov.l r0,@($1,r1)", SHInlineAsmSizeKind::Indeterminate,
                "operand-dependent mixed-width");
}

TEST(SHInlineAsmSizeTest, SizesSH4AInstructionsAndRepeatsExactly) {
  expectSize("movco.l r0,@r1\nmovli.l @r2,r0\nmovua.l @r3,r0\nmovua.l @r4+,r0",
             8);
  expectSize("icbi @r5\nldc r6,sgr\nldc.l @r7+,sgr\nprefi @r8\nsynco", 10);
  expectSize("fsrra fr9\nfsca fpul,dr10\nfpchg", 6);
  expectSize(".rept 3\nmovua.l @r1,r0\nmovi20 #1,r2\nsynco\n.endr", 24);
}

TEST(SHInlineAsmSizeTest, SizesSH4ALDSPCompoundInstructionsExactly) {
  expectSize("clrdmxy\nldrc #4\nldre 1f\nldrs 1f\nsetdmx\nsetdmy\nsetrc #2\n1:",
             14);
  expectSize(
      "movx.w @r4,x0 nopy\nmovx.l @r5+,x1 movy.l @r6+,y1\nmovs.l @r7+,m0", 6);
  expectSize("padd x0,y0,a0\ndct pabs x0,a1\nmovx.w @r4,x0 padd x0,y0,a0", 12);
  expectSize("padd x0,y0,a0 pmuls x0,y0,m0\nmovx.w @r4,x0 dcf psub x0,y0,a0",
             8);
  expectSize("repeat 1f 2f #2\n1: nop\n2:", 8);
}

TEST(SHInlineAsmSizeTest, SizesRequiredRepeatCountsExactly) {
  const uint64_t Counts[] = {0,   1,   2,    15,   16,   127,  128,
                             255, 256, 2047, 2048, 2499, 2500, 2501};
  for (uint64_t Count : Counts) {
    std::string Asm = (Twine(".rept ") + Twine(Count) + "\nnop\n.endr").str();
    expectSize(Asm, Count * 2);
  }
}

TEST(SHInlineAsmSizeTest, SizesRepeatFormsAndBodiesExactly) {
  expectSize(".rept 2; nop; .endr", 4);
  expectSize(".rept 3\nnop\nadd r0,r1\n.endr", 12);
  expectSize(".rept 4\n.endr", 0);
  expectSize(".rept 2\nrts\nnop\n.endr", 8);
  expectSize(".rept 2\n.byte 1,2\n.long 3\n.endr", 12);
  expectSize(".rept (2 + 3) * 4\nnop\n.endr", 40);
  expectSize(".rept 2\n.rept 3\nnop\n.endr\n.endr", 12);
  expectSize(".rept 0\nunknown_macro\n.endr", 0);
}

TEST(SHInlineAsmSizeTest, HandlesLabelsCommentsWhitespaceAndCRLF) {
  expectSize("\t.rept\t2\r\n1:\tnop ! comment ; ignored\r\n.endr\r\n", 4);
  expectSize(".rept 1\n.Llocal: nop\n.endr", 2);
  expectSize("! only a comment\n.Lempty:\n", 0);
}

TEST(SHInlineAsmSizeTest, HashCommentsOnlyAtStatementStart) {
  expectSize("# arbitrary comment without a final newline", 0);
  expectSize(" \t# comment ; nop\r\n# another comment\r\n", 0);
  expectSize("1: .Llocal: # comment after labels\n", 0);
  expectSize("nop; # comment until newline ; invalid_mnemonic\n", 2);
  expectSize("# comment\nadd #1,r0\nmov #7,r1 ! trailing SH comment", 4);
  expectSize(".rept 2\n# comment\n1: add #1,r0\n.endr", 4);
  expectSize("# comment\n.byte 1,2,3,4\n.long 7", 8);
  expectSize(".ascii \"# ! ;\"", 5);
  expectFailure("# valid comment\ninvalid_sh_assembly_mnemonic",
                SHInlineAsmSizeKind::Indeterminate,
                "unknown instruction or macro invocation");
  expectFailure("! valid comment\ninvalid_sh_assembly_mnemonic",
                SHInlineAsmSizeKind::Indeterminate,
                "unknown instruction or macro invocation");
}

TEST(SHInlineAsmSizeTest, SizesConstantConditionalsExactly) {
  expectSize(".if 1\nnop\n.else\nnop\nnop\n.endif", 2);
  expectSize(".if 0\nunknown_macro\n.else\n.rept 2\nnop\n.endr\n.endif", 4);
  expectSize(".if (3 * 4) == 12 && 7 > 2\nnop\n.endif", 2);
}

TEST(SHInlineAsmSizeTest, UsesGNUExpressionPrecedenceAndTruthValues) {
  expectSize(".space 16 >> 1 + 2", 10);
  expectSize(".space 1 + 2 << 3", 17);
  expectSize(".space 1 | 2 & 4", 0);
  expectSize(".if (1 == 1) < 0\nnop\n.endif", 2);
  expectSize(".if 1 == 2 < 1\nnop\n.endif", 2);
  expectSize(".space -(3 > 2)", 1);
  expectSize(".if (1 <> 2) == -1\nnop\n.endif", 2);
  expectSize(".space (16 >> 1) + 2", 10);
  expectSize(".space 16 >> (1 + 2)", 2);
}

TEST(SHInlineAsmSizeTest, SizesDataDirectivesExactly) {
  expectSize(
      ".byte 1,2,3\n.short 1,2\n.long 1,2\n.fill 3,2,7\n.space 5\n.zero 7", 33);
  expectSize(".fill 3,9,7", 24);
  expectSize(".fill 3,-1,7", 0);
  expectSize(".space 4294967296", uint64_t(1) << 32);
  expectSize(".value 1\n.2byte 1\n.int 1\n.4byte 1\n.quad 1\n.8byte 1", 28);
  expectSize(".ascii \"a;b\",\"c\\n\"\n.asciz \"a\",\"bc\"\n.string \"x\"", 12);
}

TEST(SHInlineAsmSizeTest, BoundsAlignmentConservatively) {
  expectSize(".align 2", 3, SHInlineAsmSizeKind::Conservative);
  expectSize(".balign 8", 7, SHInlineAsmSizeKind::Conservative);
  expectSize(".p2align 3,,4", 4, SHInlineAsmSizeKind::Conservative);
  expectSize(".p2align 4,0,2", 2, SHInlineAsmSizeKind::Conservative);
  expectSize(".p2align 0", 0);
  expectSize(".p2align", 0);
}

TEST(SHInlineAsmSizeTest, IgnoresNonEmittingDirectives) {
  expectSize("label:\n1:\n.file 1 \"path;name.c\"\n.loc 1 2 0\n.type "
             "label,@function\n.size label,.-label\n.cfi_def_cfa_offset "
             "4\n.globl label\n.hidden label\n.ident \"compiler\"",
             0);
}

TEST(SHInlineAsmSizeTest, SizesSymbolAssignmentsExactly) {
  expectSize("alias = base\nnop", 2);
  expectSize(".Lalias=base+4\nrts\nnop", 4);
  expectSize("${0:c} = ${1:c}\nnop", 2);
  expectFailure(". = . + 16", SHInlineAsmSizeKind::Indeterminate,
                "location counter assignment");
}

TEST(SHInlineAsmSizeTest, SelectsTheSHAssemblyAlternative) {
  expectSize("$(nop$|nop;nop$)", 2);
  expectSize("$(.rept 2;nop;.endr$|nop$)", 4);
  expectSize("mov #$$symbol,r0", 2);
  expectSize("$(.ascii \"$$\"$|nop$)", 1);
}

TEST(SHInlineAsmSizeTest, MatchesAsmPrinterAlternativeSelection) {
  expectSelected("nop", "nop");
  expectSelected("$(nop$|add r0,r1$)", "nop");
  expectSelected("$(nop$|add r0,r1$|rts$)", "nop");
  expectSelected("$($|nop$)", "");
  expectSelected("$(nop$|$)", "nop");
  expectSelected("mov #$$symbol,r0", "mov #$symbol,r0");
  expectSelected("%%literal", "%%literal");
  expectSelected("$(.ascii \"{a|b}\"$|nop$)", ".ascii \"{a|b}\"");
  expectSelected("$(.ascii \"a;b\"$|nop$)", ".ascii \"a;b\"");
  expectSelected("$) nop", "} nop");
  expectSelected("$(nop$|rts", "nop");
  expectSelected("{nested|looking} text", "{nested|looking} text");
  expectSelected("$(mov ${0},${1}$|add ${2},${3}$)", "mov ${0},${1}");
  expectSelected("$(nop\n$|rts\n$)", "nop\n\n");

  std::string Diagnostic;
  EXPECT_FALSE(selectSHInlineAsmAlternative("$(nop$(rts$)$)", Diagnostic));
  EXPECT_EQ("nested inline assembly alternatives", Diagnostic);
}

TEST(SHInlineAsmSizeTest, RejectsIndeterminateSizeConstructs) {
  expectFailure(".rept count\nnop\n.endr", SHInlineAsmSizeKind::Indeterminate,
                "symbol-dependent");
  expectFailure(".space $0", SHInlineAsmSizeKind::Indeterminate,
                "operand-dependent");
  expectFailure(".ascii \"$0\"", SHInlineAsmSizeKind::Indeterminate,
                "operand-dependent");
  for (StringRef Asm :
       {"macro_call", ".macro m", ".irp r,r0", ".irpc c,ab", ".incbin \"file\"",
        ".org 32", ".section .other", ".word 1"})
    expectFailure(Asm, SHInlineAsmSizeKind::Indeterminate, "");
}

TEST(SHInlineAsmSizeTest, RejectsMalformedConstructs) {
  expectFailure(".rept -1\nnop\n.endr", SHInlineAsmSizeKind::Malformed,
                "negative");
  expectFailure(".rept 2\nnop", SHInlineAsmSizeKind::Malformed, "unterminated");
  expectFailure(".endr", SHInlineAsmSizeKind::Malformed, "unmatched");
  expectFailure(".if 1\nnop", SHInlineAsmSizeKind::Malformed, "unterminated");
  expectFailure(".balign 3", SHInlineAsmSizeKind::Malformed, "power of two");
  expectFailure(".p2align 32", SHInlineAsmSizeKind::Malformed,
                "invalid alignment");
  expectFailure(".fill -1,4,0", SHInlineAsmSizeKind::Malformed, "negative");
  expectFailure(".zero -1", SHInlineAsmSizeKind::Malformed, "negative");
  expectFailure(".rept 9223372036854775807 + 1\nnop\n.endr",
                SHInlineAsmSizeKind::Malformed, "addition overflows");
  expectFailure(".fill 9223372036854775807,8,0", SHInlineAsmSizeKind::Malformed,
                "multiplication overflows");
  expectFailure(".rept 9223372036854775807\n.quad 0,0,0\n.endr",
                SHInlineAsmSizeKind::Malformed, "multiplication overflows");
  expectFailure(
      ".space 9223372036854775807\n.space 9223372036854775807\n.space 2",
      SHInlineAsmSizeKind::Malformed, "addition overflows");
  expectFailure("$(nop$(rts$)$)", SHInlineAsmSizeKind::Malformed,
                "nested inline assembly alternatives");
}

TEST(SHInlineAsmSizeTest, BoundsDirectiveNestingDepth) {
  std::string Asm;
  for (unsigned I = 0; I != 65; ++I)
    Asm += ".rept 1\n";
  Asm += "nop\n";
  for (unsigned I = 0; I != 65; ++I)
    Asm += ".endr\n";
  expectFailure(Asm, SHInlineAsmSizeKind::Malformed, "nesting is too deep");
}

TEST(SHInlineAsmSizeTest, BoundsExpressionNestingDepth) {
  expectSize(".space " + nestExpression("(", ")", 64), 1);
  expectFailure(".space " + nestExpression("(", ")", 65),
                SHInlineAsmSizeKind::Malformed,
                "expression nesting is too deep");
}

TEST(SHInlineAsmSizeTest, BoundsUnaryExpressionNestingDepth) {
  expectSize(".space " + nestExpression("+", "", 64), 1);
  expectFailure(".space " + nestExpression("+", "", 65),
                SHInlineAsmSizeKind::Malformed,
                "expression nesting is too deep");
}

TEST(SHInlineAsmSizeTest, BoundsBracketExpressionNestingDepth) {
  expectSize(".space " + nestExpression("[", "]", 64), 1);
  expectFailure(".space " + nestExpression("[", "]", 65),
                SHInlineAsmSizeKind::Malformed,
                "expression nesting is too deep");
  for (StringRef Opening : {"(", "["}) {
    StringRef Closing = Opening == "(" ? ")" : "]";
    for (StringRef Inner : {"(", "[", "+", "-", "~"}) {
      SCOPED_TRACE((Opening + Inner).str());
      StringRef End = Inner == "(" ? ")" : Inner == "[" ? "]" : "";
      // Use an even number of unary operators to retain the value one.
      expectSize(".space " + nestExpression(Opening, Closing, 32,
                                            nestExpression(Inner, End, 32)),
                 1);
      expectFailure(".space " + nestExpression(Opening, Closing, 32,
                                               nestExpression(Inner, End, 33)),
                    SHInlineAsmSizeKind::Malformed,
                    "expression nesting is too deep");
    }
  }
  expectSize(".space " + nestExpression("+[", "]", 32), 1);
  expectFailure(".space +" + nestExpression("+[", "]", 32),
                SHInlineAsmSizeKind::Malformed,
                "expression nesting is too deep");
}

TEST(SHInlineAsmSizeTest, KeepsMCGroupingGrammar) {
  expectSize(".space [1 + (2 * [3])]", 7);
  // On SH, '!' begins a comment instead of an admitted unary expression.
  expectSize(".space [1] ! comment", 1);
  for (StringRef Expression : {"[1)", "(1]", "[1", "1]"})
    EXPECT_EQ(SHInlineAsmSizeKind::Malformed,
              getSHInlineAsmSize(".space " + Expression.str()).Kind);
  expectFailure(".space [unknown]", SHInlineAsmSizeKind::Indeterminate,
                "symbol-dependent expression");
  std::string Expression = nestExpression("[", "]", 64);
  expectSize(".space " + Expression + " + " + Expression, 2);
  expectSize(".rept " + Expression + "\nnop\n.endr", 2);
}

TEST(SHInlineAsmSizeTest, BoundsMixedExpressionNestingDepth) {
  std::string Accepted =
      nestExpression("(", ")", 32, nestExpression("+", "", 32));
  std::string Rejected =
      nestExpression("(", ")", 32, nestExpression("+", "", 33));
  expectSize(".space " + Accepted, 1);
  expectFailure(".space " + Rejected, SHInlineAsmSizeKind::Malformed,
                "expression nesting is too deep");
}

TEST(SHInlineAsmSizeTest, BoundsNestedDirectiveExpressions) {
  std::string Parentheses = nestExpression("(", ")", 64, "2");
  std::string Unary = nestExpression("+", "", 64, "1");
  expectSize(".rept " + Parentheses + "\nnop\n.endr", 4);
  expectSize(".if " + Unary + "\nnop\n.endif", 2);
  expectFailure(".rept " + nestExpression("(", ")", 65, "2") + "\nnop\n.endr",
                SHInlineAsmSizeKind::Malformed,
                "expression nesting is too deep");
  expectFailure(".if " + nestExpression("+", "", 65) + "\nnop\n.endif",
                SHInlineAsmSizeKind::Malformed,
                "expression nesting is too deep");
}

TEST(SHInlineAsmSizeTest, ResetsExpressionDepthBetweenParses) {
  std::string Expression = nestExpression("(", ")", 64);
  expectSize(".space " + Expression + "\n.space " + Expression + "\n.fill " +
                 Expression + ",1",
             3);
  expectFailure(".space " + nestExpression("(", ")", 65),
                SHInlineAsmSizeKind::Malformed,
                "expression nesting is too deep");
  expectSize(".space " + Expression, 1);
}

TEST(SHInlineAsmSizeTest, ChecksLeftShiftRepresentability) {
  expectSize(".space 0 << 0", 0);
  expectSize(".space 1 << 0", 1);
  expectSize(".space 3 << 4", 48);
  expectSize(".space 1 << 62", uint64_t(1) << 62);
  expectSize(".space 3 << 61", uint64_t(3) << 61);
  expectFailure(".space 1 << 63", SHInlineAsmSizeKind::Malformed,
                "left shift overflows");
  expectFailure(".space 3 << 62", SHInlineAsmSizeKind::Malformed,
                "left shift overflows");
  expectSize(".if (-1 << 62) == -4611686018427387904\nnop\n.endif", 2);
  expectSize(".if (-2 << 62) == (-1 << 63)\nnop\n.endif", 2);
  expectFailure(".if -3 << 62\nnop\n.endif", SHInlineAsmSizeKind::Malformed,
                "left shift overflows");
  expectSize(".space 0 << 63", 0);
}

TEST(SHInlineAsmSizeTest, ChecksShiftCountsAndRightShiftSemantics) {
  expectFailure(".space 1 << 64", SHInlineAsmSizeKind::Malformed,
                "invalid shift count");
  expectFailure(".space 1 << -1", SHInlineAsmSizeKind::Malformed,
                "invalid shift count");
  expectFailure(".space 1 >> 64", SHInlineAsmSizeKind::Malformed,
                "invalid shift count");
  expectFailure(".space 1 >> -1", SHInlineAsmSizeKind::Malformed,
                "invalid shift count");
  expectSize(".space 8 >> 2", 2);
  expectSize(".if (-8 >> 2) == 4611686018427387902\nnop\n.endif", 2);
  expectSize(".space (1 << 2) << 3", 32);
}

TEST(SHInlineAsmSizeTest, SizesShiftExpressionsInDirectives) {
  expectSize(".rept 1 << 2\nnop\n.endr", 8);
  expectSize(".fill 1 << 2, 1 << 1", 8);
  expectSize(".space 1 << 4", 16);
  expectSize(".p2align 1 << 2", 15, SHInlineAsmSizeKind::Conservative);
  expectSize(".balign 1 << 3", 7, SHInlineAsmSizeKind::Conservative);
}

TEST(SHInlineAsmSizeTest, ChecksOtherSignedArithmeticEdges) {
  expectFailure(".if -(-1 << 63)\nnop\n.endif", SHInlineAsmSizeKind::Malformed,
                "subtraction overflows");
  expectFailure(".if (-1 << 63) / -1\nnop\n.endif",
                SHInlineAsmSizeKind::Malformed, "division overflows");
  expectSize(".space ((-1 << 63) % -1)", 0);
}

TEST(SHInlineAsmSizeTest, CoversAuthoritativeSHMnemonicInventory) {
  LLVMInitializeSHTargetInfo();
  LLVMInitializeSHTargetMC();

  Triple TT("sh-unknown-none-elf");
  std::string Error;
  const Target *Target = TargetRegistry::lookupTarget(TT, Error);
  ASSERT_NE(nullptr, Target) << Error;
  std::unique_ptr<MCRegisterInfo> MRI(Target->createMCRegInfo(TT));
  ASSERT_NE(nullptr, MRI);
  std::unique_ptr<MCInstrInfo> MII(Target->createMCInstrInfo());
  ASSERT_NE(nullptr, MII);
  MCTargetOptions Options;
  std::unique_ptr<MCAsmInfo> MAI(Target->createMCAsmInfo(*MRI, TT, Options));
  ASSERT_NE(nullptr, MAI);
  std::unique_ptr<MCInstPrinter> Printer(
      Target->createMCInstPrinter(TT, 0, *MAI, *MII, *MRI));
  ASSERT_NE(nullptr, Printer);

  StringSet<> Authority;
  StringSet<> Delayed;
  StringMap<unsigned> Widths;
  for (unsigned Opcode = 0; Opcode != MII->getNumOpcodes(); ++Opcode) {
    const MCInstrDesc &Description = MII->get(Opcode);
    if (Description.isPseudo() || Description.getSize() == 0)
      continue;
    MCInst Instruction;
    Instruction.setOpcode(Opcode);
    const char *Assembly = Printer->getMnemonic(Instruction).first;
    if (!Assembly) {
      EXPECT_TRUE(StringRef(MII->getName(Opcode)).starts_with("DSP_LOOP_"));
      continue;
    }
    StringRef Mnemonic = StringRef(Assembly).split('\t').first;
    ASSERT_FALSE(Mnemonic.empty()) << MII->getName(Opcode);
    EXPECT_TRUE(Description.getSize() == 2 || Description.getSize() == 4)
        << Mnemonic;
    Authority.insert(Mnemonic);
    Widths[Mnemonic] |= Description.getSize() == 2 ? 1U : 2U;
    if (Description.hasDelaySlot())
      Delayed.insert(Mnemonic);
  }

  StringSet<> Analyzer;
  for (StringRef Mnemonic : getSHInlineAsmInstructionMnemonics()) {
    EXPECT_TRUE(Analyzer.insert(Mnemonic).second) << Mnemonic;
    EXPECT_TRUE(Authority.contains(Mnemonic)) << Mnemonic;
  }
  for (const auto &Entry : Authority)
    EXPECT_TRUE(Analyzer.contains(Entry.getKey())) << Entry.getKey();

  EXPECT_EQ(Authority.size(), Analyzer.size());
  for (StringRef Mnemonic : {"mov.b", "mov.w", "mov.l", "fmov.s", "fmov.d"})
    EXPECT_EQ(3u, Widths.lookup(Mnemonic)) << Mnemonic;
  EXPECT_EQ(10u, Delayed.size());
  for (StringRef Mnemonic : {"bf/s", "bra", "braf", "bsr", "bsrf", "bt/s",
                             "jmp", "jsr", "rte", "rts"})
    EXPECT_TRUE(Delayed.contains(Mnemonic)) << Mnemonic;
}

TEST(SHInlineAsmSizeTest, RejectsInstructionsOutsideSupportedSHMCSurface) {
  for (StringRef Assembly : {"unsupported_dsp r1,r2"})
    expectFailure(Assembly, SHInlineAsmSizeKind::Indeterminate,
                  "unknown instruction");
}

} // namespace
