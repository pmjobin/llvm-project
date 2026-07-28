//===-- SHInstPrinter.cpp - Convert SH MCInst to assembly syntax ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHInstPrinter.h"
#include "SHMCTargetDesc.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
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
  OS << format_hex(Address + 4 + Op.getImm(), 0);
}

void SHInstPrinter::printSImm8(const MCInst *MI, unsigned OpNo,
                               raw_ostream &OS) {
  OS << '#' << MI->getOperand(OpNo).getImm();
}

void SHInstPrinter::printLongMemReg(const MCInst *MI, unsigned OpNo,
                                    raw_ostream &OS) {
  OS << '@';
  printRegName(OS, MI->getOperand(OpNo).getReg());
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
  if (!printAliasInstr(MI, Address, OS))
    printInstruction(MI, Address, OS);
  printAnnotation(OS, Annot);
}
