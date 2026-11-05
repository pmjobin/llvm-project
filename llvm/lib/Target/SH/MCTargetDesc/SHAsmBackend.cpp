//===-- SHAsmBackend.cpp - SH assembler backend --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHBaseInfo.h"
#include "SHFixupKinds.h"
#include "SHMCAsmInfo.h"
#include "SHMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/SHTargetParser.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

namespace {

static bool isTLSModifier(unsigned Specifier) {
  return Specifier == SH::S_TLSGD || Specifier == SH::S_TLSLDM ||
         Specifier == SH::S_DTPOFF || Specifier == SH::S_GOTTPOFF ||
         Specifier == SH::S_TPOFF;
}

static bool isFDPICModifier(unsigned Specifier) {
  return Specifier == SH::S_GOTFUNCDESC || Specifier == SH::S_GOTOFFFUNCDESC ||
         Specifier == SH::S_FUNCDESC;
}

static bool isFDPICMode(const Triple &TT, const MCTargetOptions &Options) {
  if (!Options.ABIName.empty())
    return SH::isFDPICABI(SH::parseABI(Options.ABIName));
  return TT.isMuslFDPIC() || Options.FDPIC;
}

class SHObjectTargetWriter : public MCELFObjectTargetWriter {
  bool IsFDPIC;
  static bool isGOTSymbol(const MCValue &Target) {
    const MCSymbol *Add = Target.getAddSym();
    return Add && Add->getName() == "_GLOBAL_OFFSET_TABLE_";
  }

public:
  // R_SH_IND12W is not a partial-in-place relocation.  Its architectural PC
  // bias must therefore be carried in an explicit addend.
  SHObjectTargetWriter(uint8_t OSABI, bool IsFDPIC)
      : MCELFObjectTargetWriter(/*Is64Bit=*/false, OSABI, ELF::EM_SH,
                                /*HasRelocationAddend=*/true),
        IsFDPIC(IsFDPIC) {}

  unsigned getRelocType(const MCFixup &Fixup, const MCValue &Target,
                        bool IsPCRel) const override {
    if (Fixup.getKind() == SH::fixup_SH_MOVI20) {
      switch (Target.getSpecifier()) {
      case SH::S_GOT:
        return ELF::R_SH_GOT20;
      case SH::S_GOTOFF:
        return ELF::R_SH_GOTOFF20;
      case SH::S_GOTFUNCDESC:
        return ELF::R_SH_GOTFUNCDESC20;
      case SH::S_GOTOFFFUNCDESC:
        return ELF::R_SH_GOTOFFFUNCDESC20;
      default:
        reportError(Fixup.getLoc(),
                    "SH movi20 relocation requires @GOT, @GOTOFF, "
                    "@GOTFUNCDESC, or @GOTOFFFUNCDESC");
        return 0;
      }
    }
    if (Fixup.getKind() == SH::fixup_SH_PCREL8_4)
      return ELF::R_SH_DIR8WPL;
    if (Fixup.getKind() == SH::fixup_SH_PCREL8_2_LITERAL)
      return ELF::R_SH_DIR8WPZ;
    if (Fixup.getKind() == SH::fixup_SH_LOOP_START)
      return ELF::R_SH_LOOP_START;
    if (Fixup.getKind() == SH::fixup_SH_LOOP_END)
      return ELF::R_SH_LOOP_END;
    if (Fixup.getKind() == SH::fixup_SH_BSR12_2)
      return ELF::R_SH_IND12W;
    if (Fixup.getKind() == SH::fixup_SH_PCREL8_2)
      return ELF::R_SH_DIR8WPN;
    if (Fixup.getKind() == SH::fixup_SH_PCREL12_2)
      return ELF::R_SH_IND12W;
    if (Target.getSpecifier()) {
      if (isFDPICModifier(Target.getSpecifier()) && !IsFDPIC) {
        reportError(Fixup.getLoc(),
                    "SH function-descriptor relocation requires FDPIC mode");
        return 0;
      }
      if (Target.getSubSym()) {
        reportError(
            Fixup.getLoc(),
            isTLSModifier(Target.getSpecifier())
                ? "SH TLS modifiers do not support symbol subtraction"
                : "SH GOT and PLT modifiers do not support symbol subtraction");
        return 0;
      }
      if (Fixup.getKind() != FK_Data_4) {
        reportError(Fixup.getLoc(),
                    isTLSModifier(Target.getSpecifier())
                        ? "SH TLS modifiers require a four-byte value"
                        : "SH GOT and PLT modifiers require a four-byte value");
        return 0;
      }
      if (isTLSModifier(Target.getSpecifier()) && Target.getAddSym()) {
        unsigned Type =
            static_cast<const MCSymbolELF *>(Target.getAddSym())->getType();
        if (Type != ELF::STT_NOTYPE && Type != ELF::STT_TLS) {
          reportError(Fixup.getLoc(),
                      "SH TLS modifier requires an STT_TLS symbol");
          return 0;
        }
      }
      switch (Target.getSpecifier()) {
      case SH::S_GOT:
        return ELF::R_SH_GOT32;
      case SH::S_GOTOFF:
        return ELF::R_SH_GOTOFF;
      case SH::S_GOTPC:
        return ELF::R_SH_GOTPC;
      case SH::S_PLT:
        return ELF::R_SH_PLT32;
      case SH::S_TLSGD:
        return ELF::R_SH_TLS_GD_32;
      case SH::S_TLSLDM:
        return ELF::R_SH_TLS_LD_32;
      case SH::S_DTPOFF:
        return ELF::R_SH_TLS_LDO_32;
      case SH::S_GOTTPOFF:
        return ELF::R_SH_TLS_IE_32;
      case SH::S_TPOFF:
        return ELF::R_SH_TLS_LE_32;
      case SH::S_GOTFUNCDESC:
        return ELF::R_SH_GOTFUNCDESC;
      case SH::S_GOTOFFFUNCDESC:
        return ELF::R_SH_GOTOFFFUNCDESC;
      case SH::S_FUNCDESC:
        return ELF::R_SH_FUNCDESC;
      default:
        reportError(Fixup.getLoc(), "unsupported SH relocation modifier");
        return 0;
      }
    }
    if (Fixup.getKind() == FK_Data_4 && !IsPCRel && isGOTSymbol(Target))
      return ELF::R_SH_GOTPC;
    if (Fixup.getKind() == FK_Data_4 && !IsPCRel)
      return ELF::R_SH_DIR32;
    if (Fixup.getKind() == FK_Data_2 && !IsPCRel)
      return ELF::R_SH_DIR16;
    if (Fixup.getKind() == FK_Data_4 && IsPCRel)
      return ELF::R_SH_REL32;
    if (Fixup.getKind() == FK_Data_1)
      reportError(Fixup.getLoc(),
                  "SH unresolved one-byte relocations are not supported");
    else if (Fixup.getKind() == FK_Data_2)
      reportError(Fixup.getLoc(),
                  "SH PC-relative two-byte relocations are not supported");
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
    return Type == ELF::R_SH_DIR8WPN || Type == ELF::R_SH_DIR8WPL ||
           Type == ELF::R_SH_DIR8WPZ || Type == ELF::R_SH_DIR16 ||
           Type == ELF::R_SH_LOOP_START || Type == ELF::R_SH_LOOP_END ||
           Type == ELF::R_SH_DIR32 || Type == ELF::R_SH_REL32 ||
           Type == ELF::R_SH_GOT32 || Type == ELF::R_SH_PLT32 ||
           Type == ELF::R_SH_GOTOFF || Type == ELF::R_SH_GOTPC ||
           Type == ELF::R_SH_TLS_GD_32 || Type == ELF::R_SH_TLS_LD_32 ||
           Type == ELF::R_SH_TLS_LDO_32 || Type == ELF::R_SH_TLS_IE_32 ||
           Type == ELF::R_SH_TLS_LE_32 || Type == ELF::R_SH_GOT20 ||
           Type == ELF::R_SH_GOTOFF20 || Type == ELF::R_SH_GOTFUNCDESC ||
           Type == ELF::R_SH_GOTFUNCDESC20 ||
           Type == ELF::R_SH_GOTOFFFUNCDESC ||
           Type == ELF::R_SH_GOTOFFFUNCDESC20 || Type == ELF::R_SH_FUNCDESC;
  }
};

class SHAsmBackend : public MCAsmBackend {
  bool IsLittleEndian;
  bool IsFDPIC;
  uint8_t OSABI;

public:
  explicit SHAsmBackend(const Triple &TT, const MCTargetOptions &Options)
      : MCAsmBackend(TT.isLittleEndian() ? endianness::little
                                         : endianness::big),
        IsLittleEndian(TT.isLittleEndian()), IsFDPIC(isFDPICMode(TT, Options)),
        OSABI(MCELFObjectTargetWriter::getOSABI(TT.getOS())) {}

  std::optional<bool> evaluateFixup(const MCFragment &F, MCFixup &Fixup,
                                    MCValue &Target, uint64_t &Value) override {
    if (Fixup.getKind() == SH::fixup_SH_PCREL8_4) {
      uint64_t P =
          Asm->getFragmentOffset(F) - Asm->getStretch() + Fixup.getOffset();
      Value = P % SH::PCRelativeLongScale;
    }
    return {};
  }

  void applyPCRel8Relocation(const MCFragment &F, const MCFixup &Fixup,
                             const MCValue &Target, uint8_t *Data,
                             uint64_t Value) {
    // Absolute numeric operands also reach this path. Keep their existing
    // handling, including the disassembler's numeric branch round trip.
    if (!Target.getAddSym()) {
      maybeAddReloc(F, Fixup, Target, Value, false);
      return;
    }
    unsigned Scale = Fixup.getKind() == SH::fixup_SH_PCREL8_4 ? 4 : 2;
    int64_t Addend = Target.getConstant();
    if (Addend % Scale) {
      getContext().reportError(Fixup.getLoc(),
                               "SH relocation addend must be a multiple of " +
                                   Twine(Scale));
      return;
    }
    int64_t Encoded = Addend / Scale;
    bool IsSigned = Fixup.getKind() == SH::fixup_SH_PCREL8_2;
    if (IsSigned ? !isInt<8>(Encoded) : !isUInt<8>(Encoded)) {
      getContext().reportError(Fixup.getLoc(),
                               "SH relocation addend is out of range");
      return;
    }

    // These relocations are partial-in-place even in RELA objects. In
    // particular, r_addend is ignored by GNU SH linkers. Unlike IND12W, the
    // PC+4 bias is applied by the linker rather than included in this addend.
    MCValue RelocTarget =
        MCValue::get(Target.getAddSym(), Target.getSubSym(), 0);
    uint64_t RelocValue = 0;
    maybeAddReloc(F, Fixup, RelocTarget, RelocValue, false);
    uint16_t Word = support::endian::read<uint16_t>(Data, Endian);
    Word = (Word & 0xff00) | (static_cast<uint16_t>(Encoded) & 0xff);
    support::endian::write<uint16_t>(Data, Word, Endian);
  }

  void applyFixup(const MCFragment &F, const MCFixup &Fixup,
                  const MCValue &Target, uint8_t *Data, uint64_t Value,
                  bool IsResolved) override {
    switch (Fixup.getKind()) {
    case SH::fixup_SH_LOOP_START:
    case SH::fixup_SH_LOOP_END:
      maybeAddReloc(F, Fixup, Target, Value, false);
      return;
    case SH::fixup_SH_MOVI20: {
      unsigned Specifier = Target.getSpecifier();
      if (Specifier != SH::S_GOT && Specifier != SH::S_GOTOFF &&
          Specifier != SH::S_GOTFUNCDESC && Specifier != SH::S_GOTOFFFUNCDESC) {
        getContext().reportError(Fixup.getLoc(),
                                 "SH movi20 relocation requires @GOT, @GOTOFF, "
                                 "@GOTFUNCDESC, or @GOTOFFFUNCDESC");
        return;
      }
      if (isFDPICModifier(Target.getSpecifier()) && !IsFDPIC) {
        getContext().reportError(
            Fixup.getLoc(),
            "SH function-descriptor relocation requires FDPIC mode");
        return;
      }
      if (Target.getSubSym()) {
        getContext().reportError(
            Fixup.getLoc(),
            "SH movi20 relocations do not support symbol subtraction");
        return;
      }
      if (Target.getConstant() && Target.getSpecifier() != SH::S_GOTOFF) {
        getContext().reportError(
            Fixup.getLoc(),
            "only an SH movi20 @GOTOFF relocation supports an addend");
        return;
      }
      if (!isInt<20>(Target.getConstant())) {
        getContext().reportError(
            Fixup.getLoc(),
            "SH movi20 relocation addend is out of signed 20-bit range");
        return;
      }
      if (!IsResolved) {
        uint64_t RelocValue = Value;
        maybeAddReloc(F, Fixup, Target, RelocValue, IsResolved);
      }
      uint32_t Bits =
          (static_cast<uint32_t>(support::endian::read<uint16_t>(Data, Endian))
           << 16) |
          support::endian::read<uint16_t>(Data + 2, Endian);
      uint32_t Encoded = 0;
      Bits = (Bits & ~0x00f0ffffU) | ((Encoded & 0xf0000U) << 4) |
             (Encoded & 0xffffU);
      support::endian::write<uint16_t>(Data, Bits >> 16, Endian);
      support::endian::write<uint16_t>(Data + 2, Bits, Endian);
      return;
    }
    case SH::fixup_SH_PCREL8_4: {
      if (!IsResolved) {
        applyPCRel8Relocation(F, Fixup, Target, Data, Value);
        return;
      }

      int64_t ByteDisp = static_cast<int64_t>(Value) - SH::PCRelativeBias;
      if (ByteDisp % SH::PCRelativeLongScale != 0) {
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
      if (ByteDisp > SH::PCRelativeLongMaxDisplacement) {
        getContext().reportError(
            Fixup.getLoc(), "SH PC-relative literal target is out of range");
        return;
      }

      uint16_t Word = support::endian::read<uint16_t>(Data, Endian);
      Word = (Word & 0xff00) |
             static_cast<uint16_t>(ByteDisp / SH::PCRelativeLongScale);
      support::endian::write<uint16_t>(Data, Word, Endian);
      return;
    }
    case SH::fixup_SH_PCREL8_2_LITERAL: {
      if (!IsResolved) {
        applyPCRel8Relocation(F, Fixup, Target, Data, Value);
        return;
      }

      int64_t ByteDisp = static_cast<int64_t>(Value) - SH::PCRelativeBias;
      if (!SH::isPCRelativeWordDisplacementInRange(ByteDisp)) {
        getContext().reportError(
            Fixup.getLoc(),
            "SH PC-relative word target must be an even forward address in "
            "range");
        return;
      }
      uint16_t Word = support::endian::read<uint16_t>(Data, Endian);
      Word = (Word & 0xff00) |
             static_cast<uint16_t>(ByteDisp / SH::PCRelativeWordScale);
      support::endian::write<uint16_t>(Data, Word, Endian);
      return;
    }
    case SH::fixup_SH_PCREL8_2:
    case SH::fixup_SH_PCREL12_2:
    case SH::fixup_SH_BSR12_2: {
      if (!IsResolved) {
        if (Fixup.getKind() == SH::fixup_SH_PCREL8_2) {
          applyPCRel8Relocation(F, Fixup, Target, Data, Value);
          return;
        }
        MCValue RelocTarget = Target;
        if (Fixup.getKind() == SH::fixup_SH_BSR12_2 ||
            Fixup.getKind() == SH::fixup_SH_PCREL12_2) {
          if (RelocTarget.getConstant() % SH::BranchDisplacementScale != 0) {
            getContext().reportError(
                Fixup.getLoc(),
                "SH unresolved branch addend must be two-byte aligned");
            return;
          }
          RelocTarget.setConstant(RelocTarget.getConstant() -
                                  SH::PCRelativeBias);
        }
        maybeAddReloc(F, Fixup, RelocTarget, Value, IsResolved);
        return;
      }

      int64_t ByteDisp = static_cast<int64_t>(Value) - SH::PCRelativeBias;
      if (ByteDisp % SH::BranchDisplacementScale != 0) {
        getContext().reportError(Fixup.getLoc(),
                                 "SH branch target must be two-byte aligned");
        return;
      }

      int64_t Encoded = ByteDisp / SH::BranchDisplacementScale;
      unsigned Width = Fixup.getKind() == SH::fixup_SH_PCREL8_2
                           ? SH::ConditionalBranchDisplacementBits
                           : SH::UnconditionalBranchDisplacementBits;
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
        // GNU SH applies 32-bit data-relocation addends from the relocated
        // word even though LLVM emits RELA sections for this target.
        int64_t InPlaceAddend = Target.getConstant();
        if (Target.getSpecifier() && Target.getSubSym()) {
          getContext().reportError(
              Fixup.getLoc(),
              isTLSModifier(Target.getSpecifier())
                  ? "SH TLS modifiers do not support symbol subtraction"
                  : "SH GOT and PLT modifiers do not support symbol "
                    "subtraction");
          return;
        }
        if (Target.getSpecifier() == SH::S_GOT && InPlaceAddend != 0) {
          getContext().reportError(Fixup.getLoc(),
                                   "SH @GOT does not support an addend; add it "
                                   "after loading the GOT slot");
          InPlaceAddend = 0;
        }
        if ((Target.getSpecifier() == SH::S_GOTFUNCDESC ||
             Target.getSpecifier() == SH::S_GOTOFFFUNCDESC ||
             Target.getSpecifier() == SH::S_FUNCDESC) &&
            InPlaceAddend != 0) {
          getContext().reportError(
              Fixup.getLoc(),
              "SH function-descriptor relocations do not support addends");
          InPlaceAddend = 0;
        }
        if ((Target.getSpecifier() == SH::S_TLSGD ||
             Target.getSpecifier() == SH::S_TLSLDM ||
             Target.getSpecifier() == SH::S_GOTTPOFF) &&
            InPlaceAddend != 0) {
          getContext().reportError(
              Fixup.getLoc(),
              "SH @TLSGD, @TLSLDM, and @GOTTPOFF do not support addends");
          InPlaceAddend = 0;
        }
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
        {"fixup_SH_PCREL8_2_LITERAL", 0, 8, 0},
        {"fixup_SH_LOOP_START", 0, 0, 0},
        {"fixup_SH_LOOP_END", 0, 0, 0},
        {"fixup_SH_MOVI20", 0, 20, 0},
    };

    if (Kind < FirstTargetFixupKind)
      return MCAsmBackend::getFixupKindInfo(Kind);
    assert(Kind - FirstTargetFixupKind < SH::NumTargetFixupKinds &&
           "invalid SH fixup kind");
    return Infos[Kind - FirstTargetFixupKind];
  }

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return std::make_unique<SHObjectTargetWriter>(OSABI, IsFDPIC);
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override {
    if (Count % SH::NopSize)
      return false;
    StringRef Nop = IsLittleEndian ? StringRef("\x09\x00", SH::NopSize)
                                   : StringRef("\x00\x09", SH::NopSize);
    while (Count) {
      OS << Nop;
      Count -= SH::NopSize;
    }
    return true;
  }
};

} // namespace

MCAsmBackend *llvm::createSHMCAsmBackend(const Target &T,
                                         const MCSubtargetInfo &STI,
                                         const MCRegisterInfo &MRI,
                                         const MCTargetOptions &Options) {
  return new SHAsmBackend(STI.getTargetTriple(), Options);
}
