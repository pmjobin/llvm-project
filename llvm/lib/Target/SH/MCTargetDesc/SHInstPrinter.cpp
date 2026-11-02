//===-- SHInstPrinter.cpp - Convert SH MCInst to assembly syntax ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHInstPrinter.h"
#include "SHBaseInfo.h"
#include "SHDSPInfo.h"
#include "SHMCTargetDesc.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define PRINT_ALIAS_INSTR
#include "SHGenAsmWriter.inc"

void SHInstPrinter::printRegName(raw_ostream &OS, MCRegister Reg) {
  OS << getRegisterName(Reg);
}

void SHInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                 raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg()) {
    printRegName(OS, Op.getReg());
    return;
  }
  if (Op.isImm()) {
    OS << Op.getImm();
    return;
  }
  MAI.printExpr(OS, *Op.getExpr());
}

void SHInstPrinter::printBranchTarget(const MCInst *MI, uint64_t Address,
                                      unsigned OpNo, raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isExpr()) {
    MAI.printExpr(OS, *Op.getExpr());
    return;
  }
  assert(Op.isImm() && "invalid SH branch target");
  OS << format_hex(Address + SH::PCRelativeBias + Op.getImm(), 0);
}

void SHInstPrinter::printPCLiteral(const MCInst *MI, uint64_t Address,
                                   unsigned OpNo, raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isExpr()) {
    MAI.printExpr(OS, *Op.getExpr());
    return;
  }
  assert(Op.isImm() && "invalid SH PC-relative literal target");
  OS << "@(" << Op.getImm() + (UseSH2APCLiteralSyntax ? SH::PCRelativeBias : 0)
     << ",pc)";
}

void SHInstPrinter::printSImm8(const MCInst *MI, unsigned OpNo,
                               raw_ostream &OS) {
  OS << '#' << MI->getOperand(OpNo).getImm();
}

void SHInstPrinter::printUImm8(const MCInst *MI, unsigned OpNo,
                               raw_ostream &OS) {
  OS << '#' << MI->getOperand(OpNo).getImm();
}

void SHInstPrinter::printUImm3(const MCInst *MI, unsigned OpNo,
                               raw_ostream &OS) {
  OS << '#' << MI->getOperand(OpNo).getImm();
}

void SHInstPrinter::printSImm20(const MCInst *MI, unsigned OpNo,
                                raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  OS << '#';
  if (Op.isExpr()) {
    MAI.printExpr(OS, *Op.getExpr());
    return;
  }
  OS << Op.getImm();
}

void SHInstPrinter::printLongMemReg(const MCInst *MI, unsigned OpNo,
                                    raw_ostream &OS) {
  OS << '@';
  printRegName(OS, MI->getOperand(OpNo).getReg());
}

void SHInstPrinter::printIndexedMem(const MCInst *MI, unsigned OpNo,
                                    raw_ostream &OS) {
  OS << "@(r0,";
  printRegName(OS, MI->getOperand(OpNo).getReg());
  OS << ')';
}

void SHInstPrinter::printR0GBR(const MCInst *MI, unsigned OpNo,
                               raw_ostream &OS) {
  OS << "@(r0,gbr)";
}

void SHInstPrinter::printGBRDisp(const MCInst *MI, unsigned OpNo,
                                 raw_ostream &OS) {
  OS << "@(" << MI->getOperand(OpNo).getImm() << ",gbr)";
}

void SHInstPrinter::printLongMemDisp(const MCInst *MI, unsigned OpNo,
                                     raw_ostream &OS) {
  OS << "@(" << MI->getOperand(OpNo + 1).getImm() << ',';
  printRegName(OS, MI->getOperand(OpNo).getReg());
  OS << ')';
}

void SHInstPrinter::printNarrowMemDisp(const MCInst *MI, unsigned OpNo,
                                       raw_ostream &OS) {
  OS << "@(" << MI->getOperand(OpNo + 1).getImm() << ',';
  printRegName(OS, MI->getOperand(OpNo).getReg());
  OS << ')';
}

void SHInstPrinter::printExtendedMemDisp(const MCInst *MI, unsigned OpNo,
                                         raw_ostream &OS) {
  OS << "@(" << MI->getOperand(OpNo + 1).getImm() << ',';
  printRegName(OS, MI->getOperand(OpNo).getReg());
  OS << ')';
}

void SHInstPrinter::printTBRDisp(const MCInst *MI, unsigned OpNo,
                                 raw_ostream &OS) {
  OS << "@@(" << MI->getOperand(OpNo).getImm() << ",tbr)";
}

void SHInstPrinter::printPreDecGPR(const MCInst *MI, unsigned OpNo,
                                   raw_ostream &OS) {
  OS << "@-";
  printRegName(OS, MI->getOperand(OpNo).getReg());
}

void SHInstPrinter::printPostIncGPR(const MCInst *MI, unsigned OpNo,
                                    raw_ostream &OS) {
  OS << '@';
  printRegName(OS, MI->getOperand(OpNo).getReg());
  OS << '+';
}

void SHInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                              StringRef Annot, const MCSubtargetInfo &STI,
                              raw_ostream &OS) {
  UseSH2APCLiteralSyntax = STI.getFeatureBits()[SH::FeatureSH2A] ||
                           STI.getFeatureBits()[SH::FeatureDSP];
  if (MI->getOpcode() == SH::DSP_DDT || MI->getOpcode() == SH::DSP_SDT ||
      MI->getOpcode() == SH::DSP_PPI) {
    unsigned Size = MII.get(MI->getOpcode()).getSize();
    uint32_t Encoding = MI->getOpcode() == SH::DSP_DDT   ? 0xf000
                        : MI->getOpcode() == SH::DSP_SDT ? 0xf400
                                                         : 0xf8000000;
    Encoding |= MI->getOperand(0).getImm();
    if (!SH::printDSPInstruction(Encoding, Size, OS))
      OS << (Size == 2 ? ".word\t" : ".long\t") << format_hex(Encoding, 0);
    UseSH2APCLiteralSyntax = false;
    printAnnotation(OS, Annot);
    return;
  }
  if (!printAliasInstr(MI, Address, OS))
    printInstruction(MI, Address, OS);
  UseSH2APCLiteralSyntax = false;
  printAnnotation(OS, Annot);
}
