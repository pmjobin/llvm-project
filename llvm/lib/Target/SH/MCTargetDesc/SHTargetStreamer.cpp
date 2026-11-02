//===-- SHTargetStreamer.cpp - SH target streamer methods ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHTargetStreamer.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

SHTargetStreamer::SHTargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {}

void SHTargetStreamer::emitMachine(const MCSubtargetInfo &) {}

void SHTargetStreamer::emitUnalignedDataValue(const MCExpr *Value,
                                              unsigned Size, SMLoc Loc) {
  Streamer.emitValue(Value, Size, Loc);
}

SHTargetAsmStreamer::SHTargetAsmStreamer(MCStreamer &S,
                                         formatted_raw_ostream &OS)
    : SHTargetStreamer(S), OS(OS) {}

const char *SHTargetAsmStreamer::getDataDirective(const MCSection *Section,
                                                  unsigned Size) const {
  if (static_cast<const MCSectionELF *>(Section)->getFlags() & ELF::SHF_ALLOC)
    return nullptr;
  switch (Size) {
  case 2:
    return "\t.uaword\t";
  case 4:
    return "\t.ualong\t";
  case 8:
    return "\t.uaquad\t";
  default:
    return nullptr;
  }
}

void SHTargetAsmStreamer::emitUnalignedDataValue(const MCExpr *Value,
                                                 unsigned Size, SMLoc Loc) {
  StringRef Directive;
  switch (Size) {
  case 2:
    Directive = ".uaword";
    break;
  case 4:
    Directive = ".ualong";
    break;
  case 8:
    Directive = ".uaquad";
    break;
  default:
    llvm_unreachable("invalid SH unaligned-data width");
  }
  OS << '\t' << Directive << '\t';
  getContext().getAsmInfo().printExpr(OS, *Value);
  OS << '\n';
}
