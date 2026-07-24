//===-- SHAsmBackend.cpp - SH assembler backend --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

namespace {

class SHObjectTargetWriter : public MCELFObjectTargetWriter {
public:
  SHObjectTargetWriter()
      : MCELFObjectTargetWriter(/*Is64Bit=*/false, ELF::ELFOSABI_NONE,
                                ELF::EM_NONE,
                                /*HasRelocationAddend=*/false) {}

  unsigned getRelocType(const MCFixup &Fixup, const MCValue &Target,
                        bool IsPCRel) const override {
    report_fatal_error("SH relocations are not supported");
  }
};

class SHAsmBackend : public MCAsmBackend {
  bool IsLittleEndian;

public:
  explicit SHAsmBackend(bool IsLittleEndian)
      : MCAsmBackend(IsLittleEndian ? endianness::little : endianness::big),
        IsLittleEndian(IsLittleEndian) {}

  void applyFixup(const MCFragment &F, const MCFixup &Fixup,
                  const MCValue &Target, uint8_t *Data, uint64_t Value,
                  bool IsResolved) override {
    getContext().reportError(Fixup.getLoc(), "SH fixups are not supported");
  }

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return std::make_unique<SHObjectTargetWriter>();
  }

  bool finishLayout() const override {
    getContext().reportError(SMLoc(), "SH object emission is not supported");
    return false;
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override {
    if (Count % 2)
      return false;
    StringRef Nop =
        IsLittleEndian ? StringRef("\x09\x00", 2) : StringRef("\x00\x09", 2);
    while (Count) {
      OS << Nop;
      Count -= 2;
    }
    return true;
  }
};

} // namespace

MCAsmBackend *llvm::createSHMCAsmBackend(const Target &T,
                                         const MCSubtargetInfo &STI,
                                         const MCRegisterInfo &MRI,
                                         const MCTargetOptions &Options) {
  return new SHAsmBackend(STI.getTargetTriple().isLittleEndian());
}
