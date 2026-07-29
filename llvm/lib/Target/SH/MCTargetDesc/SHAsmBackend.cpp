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
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

namespace {

class SHObjectTargetWriter : public MCELFObjectTargetWriter {
public:
  // R_SH_IND12W is not a partial-in-place relocation.  Its architectural PC
  // bias must therefore be carried in an explicit addend.
  SHObjectTargetWriter(uint8_t OSABI)
      : MCELFObjectTargetWriter(/*Is64Bit=*/false, OSABI, ELF::EM_SH,
                                /*HasRelocationAddend=*/true) {}

  unsigned getRelocType(const MCFixup &Fixup, const MCValue &Target,
                        bool IsPCRel) const override {
    if (Fixup.getKind() == SH::fixup_SH_PCREL8_4) {
      reportError(Fixup.getLoc(),
                  "SH PC-relative literal target must be defined in the same "
                  "section");
      return 0;
    }
    if (Fixup.getKind() == SH::fixup_SH_BSR12_2)
      return ELF::R_SH_IND12W;
    if (Fixup.getKind() == SH::fixup_SH_PCREL8_2 ||
        Fixup.getKind() == SH::fixup_SH_PCREL12_2) {
      reportError(Fixup.getLoc(),
                  "SH unresolved branch relocations are not supported");
      return 0;
    }
    if (Fixup.getKind() == FK_Data_4 && !IsPCRel)
      return ELF::R_SH_DIR32;
    if (Fixup.getKind() == FK_Data_4 && IsPCRel)
      return ELF::R_SH_REL32;
    if (Fixup.getKind() == FK_Data_1)
      reportError(Fixup.getLoc(),
                  "SH unresolved one-byte relocations are not supported");
    else if (Fixup.getKind() == FK_Data_2)
      reportError(Fixup.getLoc(),
                  "SH unresolved two-byte relocations are not supported");
    else if (Fixup.getKind() == FK_Data_8)
      reportError(Fixup.getLoc(),
                  "SH unresolved eight-byte relocations are not supported");
    else if (IsPCRel)
      reportError(Fixup.getLoc(),
                  "SH PC-relative data relocations are not supported");
    else
      reportError(Fixup.getLoc(), "unsupported SH relocation");
    return 0;
  }

  bool needsRelocateWithSymbol(const MCValue &, unsigned Type) const override {
    // These are partial-in-place relocations.  Keep the original symbol so
    // section-symbol conversion does not move any addend into the RELA record.
    return Type == ELF::R_SH_DIR32 || Type == ELF::R_SH_REL32;
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

  std::optional<bool> evaluateFixup(const MCFragment &F, MCFixup &Fixup,
                                    MCValue &Target, uint64_t &Value) override {
    if (Fixup.getKind() == SH::fixup_SH_PCREL8_4) {
      uint64_t P =
          Asm->getFragmentOffset(F) - Asm->getStretch() + Fixup.getOffset();
      Value = P % 4;
    }
    return {};
  }

  void applyFixup(const MCFragment &F, const MCFixup &Fixup,
                  const MCValue &Target, uint8_t *Data, uint64_t Value,
                  bool IsResolved) override {
    switch (Fixup.getKind()) {
    case SH::fixup_SH_PCREL8_4: {
      if (!IsResolved) {
        getContext().reportError(
            Fixup.getLoc(),
            "SH PC-relative literal target must be defined in the same "
            "section");
        return;
      }

      int64_t ByteDisp = static_cast<int64_t>(Value) - 4;
      if (ByteDisp % 4 != 0) {
        getContext().reportError(
            Fixup.getLoc(),
            "SH PC-relative literal target must be four-byte aligned");
        return;
      }
      if (ByteDisp < 0) {
        getContext().reportError(
            Fixup.getLoc(),
            "SH PC-relative literal target is behind the instruction");
        return;
      }
      if (ByteDisp > 1020) {
        getContext().reportError(
            Fixup.getLoc(), "SH PC-relative literal target is out of range");
        return;
      }

      uint16_t Word = support::endian::read<uint16_t>(Data, Endian);
      Word = (Word & 0xff00) | static_cast<uint16_t>(ByteDisp / 4);
      support::endian::write<uint16_t>(Data, Word, Endian);
      return;
    }
    case SH::fixup_SH_PCREL8_2:
    case SH::fixup_SH_PCREL12_2:
    case SH::fixup_SH_BSR12_2: {
      if (!IsResolved) {
        MCValue RelocTarget = Target;
        if (Fixup.getKind() == SH::fixup_SH_BSR12_2) {
          if (RelocTarget.getConstant() % 2 != 0) {
            getContext().reportError(
                Fixup.getLoc(),
                "SH unresolved BSR addend must be two-byte aligned");
            return;
          }
          RelocTarget.setConstant(RelocTarget.getConstant() - 4);
        }
        maybeAddReloc(F, Fixup, RelocTarget, Value, IsResolved);
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
      if (!IsResolved) {
        // GNU SH applies DIR32 and REL32 addends from the relocated word even
        // when the relocation section uses RELA for IND12W.
        int64_t InPlaceAddend = Target.getConstant();
        MCFixup RelocFixup = Fixup;
        if (const MCSymbol *Sub = Target.getSubSym()) {
          if (!Sub->isDefined()) {
            getContext().reportError(
                Fixup.getLoc(),
                "SH R_SH_REL32 subtraction symbol must be defined");
            return;
          }
          if (!Sub->isInSection() || &Sub->getSection() != F.getParent()) {
            getContext().reportError(
                Fixup.getLoc(),
                "SH R_SH_REL32 subtraction across sections is not supported");
            return;
          }
          uint64_t FixupOffset = Asm->getFragmentOffset(F) + Fixup.getOffset();
          InPlaceAddend += static_cast<int64_t>(FixupOffset) -
                           static_cast<int64_t>(Asm->getSymbolOffset(*Sub));
          RelocFixup.setPCRel();
        }
        MCValue RelocTarget =
            MCValue::get(Target.getAddSym(), nullptr, 0, Target.getSpecifier());
        uint64_t RelocValue = Value;
        maybeAddReloc(F, RelocFixup, RelocTarget, RelocValue, IsResolved);
        Value = InPlaceAddend;
      }
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
        {"fixup_SH_BSR12_2", 0, 12, 0},
        {"fixup_SH_PCREL8_4", 0, 8, 0},
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
