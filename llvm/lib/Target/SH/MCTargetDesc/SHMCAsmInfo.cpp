//===-- SHMCAsmInfo.cpp - SH asm properties -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHMCAsmInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

static const MCAsmInfo::AtSpecifier AtSpecifiers[] = {
    {SH::S_GOT, "GOT"},       {SH::S_GOTOFF, "GOTOFF"},
    {SH::S_GOTPC, "GOTPC"},   {SH::S_PLT, "PLT"},
    {SH::S_TLSGD, "TLSGD"},   {SH::S_TLSLDM, "TLSLDM"},
    {SH::S_DTPOFF, "DTPOFF"}, {SH::S_GOTTPOFF, "GOTTPOFF"},
    {SH::S_TPOFF, "TPOFF"},
};

void SHMCAsmInfo::anchor() {}

SHMCAsmInfo::SHMCAsmInfo(const Triple &TT, const MCTargetOptions &Options)
    : MCAsmInfoELF(Options) {
  CodePointerSize = 4;
  CalleeSaveStackSlotSize = 4;
  CommentString = "!";
  IsLittleEndian = TT.isLittleEndian();
  MinInstAlignment = 2;
  SupportsDebugInformation = true;
  ExceptionsType = ExceptionHandling::DwarfCFI;
  initializeAtSpecifiers(AtSpecifiers);
}
