//===-- SHMCAsmInfo.cpp - SH asm properties -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHMCAsmInfo.h"
#include "SHBaseInfo.h"
#include "llvm/ADT/Enum.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

constexpr EnumStringDef<MCAsmInfo::AtSpecifierKind> AtSpecifierDefs[] = {
    {{"GOT"}, SH::S_GOT},
    {{"GOTOFF"}, SH::S_GOTOFF},
    {{"GOTPC"}, SH::S_GOTPC},
    {{"PLT"}, SH::S_PLT},
    {{"TLSGD"}, SH::S_TLSGD},
    {{"TLSLDM"}, SH::S_TLSLDM},
    {{"DTPOFF"}, SH::S_DTPOFF},
    {{"GOTTPOFF"}, SH::S_GOTTPOFF},
    {{"TPOFF"}, SH::S_TPOFF},
    {{"GOTFUNCDESC"}, SH::S_GOTFUNCDESC},
    {{"GOTOFFFUNCDESC"}, SH::S_GOTOFFFUNCDESC},
    {{"FUNCDESC"}, SH::S_FUNCDESC},
};
constexpr auto AtSpecifiers = BUILD_ENUM_STRINGS(AtSpecifierDefs);

void SHMCAsmInfo::anchor() {}

SHMCAsmInfo::SHMCAsmInfo(const Triple &TT, const MCTargetOptions &Options)
    : MCAsmInfoELF(Options) {
  CodePointerSize = 4;
  CalleeSaveStackSlotSize = 4;
  CommentString = "!";
  HasPreferredAlignment = false;
  IsLittleEndian = TT.isLittleEndian();
  MinInstAlignment = SH::InstructionAlignment;
  SupportsDebugInformation = true;
  initializeAtSpecifiers(AtSpecifiers);
}

const MCExpr *
SHMCAsmInfo::getExprForPersonalitySymbol(const MCSymbol *Sym, unsigned Encoding,
                                         MCStreamer &Streamer) const {
  if ((Encoding & dwarf::DW_EH_PE_indirect) &&
      (Encoding & 0x70) == dwarf::DW_EH_PE_datarel)
    return MCSymbolRefExpr::create(Sym, SH::S_GOTFUNCDESC,
                                   Streamer.getContext());
  return MCAsmInfo::getExprForPersonalitySymbol(Sym, Encoding, Streamer);
}
