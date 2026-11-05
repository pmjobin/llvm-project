//===- SH.cpp -------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "InputFiles.h"
#include "RelocScan.h"
#include "Symbols.h"
#include "Target.h"
#include "llvm/BinaryFormat/ELF.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {
class SH final : public TargetInfo {
public:
  SH(Ctx &ctx) : TargetInfo(ctx) { symbolicRel = R_SH_DIR32; }
  uint32_t calcEFlags() const override;
  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  bool usesOnlyImplicitAddend(RelType type) const override;
  int64_t getImplicitAddend(const uint8_t *buf, RelType type) const override;
  template <class ELFT, class RelTy>
  void scanSectionImpl(InputSectionBase &sec, Relocs<RelTy> rels,
                       unsigned shard);
  void scanSection(InputSectionBase &sec, unsigned shard) override {
    if (ctx.arg.isLE)
      elf::scanSection1<SH, ELF32LE>(*this, sec, shard);
    else
      elf::scanSection1<SH, ELF32BE>(*this, sec, shard);
  }
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;
};
} // namespace

uint32_t SH::calcEFlags() const {
  if (!ctx.sharedFiles.empty())
    ErrAlways(ctx) << "SH shared object inputs are not supported";
  uint32_t flags = 0;
  for (InputFile *f : ctx.objectFiles) {
    auto checkFile = [&](const auto &obj) {
      const auto &h = obj.getHeader();
      // Keep the two qualified machine profiles distinct. ISA compatibility
      // alone does not establish compatible floating-point conventions.
      // Reject other ISAs, PIC/FDPIC and unknown flag bits.
      if (h.e_flags != EF_SH4 && h.e_flags != EF_SH4_NOFPU)
        ErrAlways(ctx) << f << ": unsupported SH e_flags: 0x"
                       << utohexstr(h.e_flags)
                       << "; expected SH4 (0x9) or SH4_NOFPU (0x10)";
      else if (flags && h.e_flags != flags)
        ErrAlways(ctx)
            << f << ": SH4 and SH4_NOFPU machine profiles cannot be mixed";
      else
        flags = h.e_flags;
      if ((h.e_ident[EI_OSABI] != ELFOSABI_NONE &&
           h.e_ident[EI_OSABI] != ELFOSABI_GNU) ||
          h.e_ident[EI_ABIVERSION] != 0)
        ErrAlways(ctx) << f << ": unsupported SH OSABI or ABI version";
      for (const auto &sec : CHECK2(obj.sections(), f))
        if (sec.sh_type == SHT_REL || sec.sh_type == SHT_CREL)
          ErrAlways(ctx) << f << ": SH requires SHT_RELA relocations";
    };
    if (ctx.arg.isLE)
      checkFile(cast<ObjFile<ELF32LE>>(f)->getObj());
    else
      checkFile(cast<ObjFile<ELF32BE>>(f)->getObj());
    for (InputSectionBase *sec : f->getSections())
      if (sec && sec != &InputSection::discarded && (sec->flags & SHF_TLS))
        ErrAlways(ctx) << f << ": SH TLS is not supported";
  }
  return flags ? flags : EF_SH4;
}

RelExpr SH::getRelExpr(RelType type, const Symbol &s,
                       const uint8_t *loc) const {
  // The assembler's numeric short PC-relative operands retain a compile-only
  // round-trip convention. They can carry an absolute target in r_addend,
  // which a partial-in-place relocation cannot represent. Do not silently
  // ignore that target when linking; code/data labels use the native format.
  if ((type == R_SH_DIR8WPN || type == R_SH_DIR8WPL || type == R_SH_DIR8WPZ) &&
      s.isUndefined() && s.getName().empty()) {
    Err(ctx) << getErrorLoc(ctx, loc)
             << "absolute SH short PC-relative targets are not supported; "
                "use a code or data label";
    return R_NONE;
  }
  if (s.isGnuIFunc()) {
    Err(ctx) << getErrorLoc(ctx, loc) << "SH IFUNC symbols are not supported";
    return R_NONE;
  }
  switch (type) {
  case R_SH_NONE:
    return R_NONE;
  case R_SH_DIR16:
  case R_SH_DIR32:
    return R_ABS;
  case R_SH_REL32:
  case R_SH_IND12W:
  case R_SH_DIR8WPN:
  case R_SH_DIR8WPZ:
    return R_PC;
  case R_SH_DIR8WPL:
    return RE_SH_PCA;
  default:
    Err(ctx) << getErrorLoc(ctx, loc) << "unsupported SH relocation " << type
             << " against symbol " << &s;
    return R_NONE;
  }
}

bool SH::usesOnlyImplicitAddend(RelType type) const {
  switch (type) {
  case R_SH_DIR32:
  case R_SH_REL32:
  case R_SH_DIR8WPN:
  case R_SH_DIR8WPL:
  case R_SH_DIR8WPZ:
    return true;
  default:
    return false;
  }
}

int64_t SH::getImplicitAddend(const uint8_t *buf, RelType type) const {
  switch (type) {
  case R_SH_NONE:
    return 0;
  case R_SH_DIR32:
  case R_SH_REL32:
    return SignExtend64<32>(read32(ctx, buf));
  case R_SH_DIR8WPN:
    return SignExtend64<8>(read16(ctx, buf) & 0xff) * 2;
  case R_SH_DIR8WPL:
    return (read16(ctx, buf) & 0xff) * 4;
  case R_SH_DIR8WPZ:
    return (read16(ctx, buf) & 0xff) * 2;
  default:
    Err(ctx) << getErrorLoc(ctx, buf)
             << "SH requires an explicit RELA addend for " << type;
    return 0;
  }
}

template <class ELFT, class RelTy>
void SH::scanSectionImpl(InputSectionBase &sec, Relocs<RelTy> rels,
                         unsigned shard) {
  RelocScan rs(ctx, &sec, shard);
  sec.relocations.reserve(rels.size());
  for (auto it = rels.begin(); it != rels.end(); ++it) {
    auto type = it->getType(false);
    if (type != R_SH_NONE)
      rs.scan<ELFT, RelTy>(it, type, rs.getAddend<ELFT>(*it, type));
  }
}

void SH::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const {
  switch (rel.type) {
  case R_SH_NONE:
    return;
  case R_SH_DIR16:
    checkIntUInt(ctx, loc, val, 16, rel);
    write16(ctx, loc, val);
    return;
  case R_SH_DIR32:
    checkIntUInt(ctx, loc, val, 32, rel);
    write32(ctx, loc, val);
    return;
  case R_SH_REL32:
    checkInt(ctx, loc, val, 32, rel);
    write32(ctx, loc, val);
    return;
  case R_SH_IND12W:
    // Unlike the eight-bit PC-relative forms, IND12W carries the -4 PC bias
    // in r_addend and does not read the old displacement field.
    checkAlignment(ctx, loc, val, 2, rel);
    checkInt(ctx, loc, val, 13, rel);
    write16(ctx, loc, (read16(ctx, loc) & 0xf000) | ((val >> 1) & 0xfff));
    return;
  case R_SH_DIR8WPN:
  case R_SH_DIR8WPZ:
  case R_SH_DIR8WPL: {
    // -r updates in-place section-symbol addends without applying a PC bias.
    if (!ctx.arg.relocatable)
      val -= 4;
    unsigned shift = rel.type == R_SH_DIR8WPL ? 2 : 1;
    checkAlignment(ctx, loc, val, 1 << shift, rel);
    if (rel.type == R_SH_DIR8WPN)
      checkInt(ctx, loc, val, 8 + shift, rel);
    else
      checkUInt(ctx, loc, val, 8 + shift, rel);
    write16(ctx, loc, (read16(ctx, loc) & 0xff00) | ((val >> shift) & 0xff));
    return;
  }
  default:
    llvm_unreachable("unsupported SH relocation");
  }
}

void elf::setSHTargetInfo(Ctx &ctx) { ctx.target = std::make_unique<SH>(ctx); }
