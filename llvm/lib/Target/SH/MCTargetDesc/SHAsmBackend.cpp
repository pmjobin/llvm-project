//===-- SHAsmBackend.cpp - SH assembler backend --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHFixupKinds.h"
#include "SHMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

namespace {

class SHObjectTargetWriter : public MCELFObjectTargetWriter {
public:
  SHObjectTargetWriter(uint8_t OSABI)
      : MCELFObjectTargetWriter(/*Is64Bit=*/false, OSABI, ELF::EM_SH,
                                /*HasRelocationAddend=*/false) {}

  unsigned getRelocType(const MCFixup &Fixup, const MCValue &Target,
                        bool IsPCRel) const override {
    if (Fixup.getKind() == SH::fixup_SH_PCREL8_2 ||
        Fixup.getKind() == SH::fixup_SH_PCREL12_2) {
      reportError(Fixup.getLoc(),
                  "SH branch relocations are not yet supported");
      return 0;
    }
    reportError(Fixup.getLoc(), "SH relocations are not yet supported");
    return 0;
  }
};

class SHAsmBackend : public MCAsmBackend {
  bool IsLittleEndian;
  uint8_t OSABI;

public:
  explicit SHAsmBackend(const Triple &TT)
      : MCAsmBackend(TT.isLittleEndian() ? endianness::little
                                         : endianness::big),
        IsLittleEndian(TT.isLittleEndian()),
        OSABI(MCELFObjectTargetWriter::getOSABI(TT.getOS())) {}

  void applyFixup(const MCFragment &F, const MCFixup &Fixup,
                  const MCValue &Target, uint8_t *Data, uint64_t Value,
                  bool IsResolved) override {
    switch (Fixup.getKind()) {
    case SH::fixup_SH_PCREL8_2:
    case SH::fixup_SH_PCREL12_2: {
      if (!IsResolved) {
        maybeAddReloc(F, Fixup, Target, Value, IsResolved);
        return;
      }

      int64_t ByteDisp = static_cast<int64_t>(Value) - 4;
      if (ByteDisp % 2 != 0) {
        getContext().reportError(Fixup.getLoc(),
                                 "SH branch target must be two-byte aligned");
        return;
      }

      int64_t Encoded = ByteDisp / 2;
      unsigned Width = Fixup.getKind() == SH::fixup_SH_PCREL8_2 ? 8 : 12;
      if (!isIntN(Width, Encoded)) {
        getContext().reportError(Fixup.getLoc(),
                                 "SH branch target is out of range");
        return;
      }

      uint16_t Mask = Width == 8 ? 0x00ff : 0x0fff;
      uint16_t Word = support::endian::read<uint16_t>(Data, Endian);
      Word = (Word & ~Mask) | (static_cast<uint16_t>(Encoded) & Mask);
      support::endian::write<uint16_t>(Data, Word, Endian);
      return;
    }
    case FK_Data_1:
      maybeAddReloc(F, Fixup, Target, Value, IsResolved);
      support::endian::write<uint8_t>(Data, Value, Endian);
      return;
    case FK_Data_2:
      maybeAddReloc(F, Fixup, Target, Value, IsResolved);
      support::endian::write<uint16_t>(Data, Value, Endian);
      return;
    case FK_Data_4:
      maybeAddReloc(F, Fixup, Target, Value, IsResolved);
      support::endian::write<uint32_t>(Data, Value, Endian);
      return;
    case FK_Data_8:
      maybeAddReloc(F, Fixup, Target, Value, IsResolved);
      support::endian::write<uint64_t>(Data, Value, Endian);
      return;
    default:
      getContext().reportError(Fixup.getLoc(), "unsupported SH fixup");
      return;
    }
  }

  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override {
    static const MCFixupKindInfo Infos[SH::NumTargetFixupKinds] = {
        {"fixup_SH_PCREL8_2", 0, 8, 0},
        {"fixup_SH_PCREL12_2", 0, 12, 0},
    };

    if (Kind < FirstTargetFixupKind)
      return MCAsmBackend::getFixupKindInfo(Kind);
    assert(Kind - FirstTargetFixupKind < SH::NumTargetFixupKinds &&
           "invalid SH fixup kind");
    return Infos[Kind - FirstTargetFixupKind];
  }

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return std::make_unique<SHObjectTargetWriter>(OSABI);
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
  return new SHAsmBackend(STI.getTargetTriple());
}
