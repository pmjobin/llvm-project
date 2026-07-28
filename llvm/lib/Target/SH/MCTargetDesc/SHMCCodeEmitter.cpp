//===-- SHMCCodeEmitter.cpp - Convert SH instructions to machine code -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHFixupKinds.h"
#include "SHMCTargetDesc.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

namespace {

class SHMCCodeEmitter : public MCCodeEmitter {
  MCContext &Ctx;
  const MCInstrInfo &MCII;

  uint64_t getBinaryCodeForInstr(const MCInst &MI,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  unsigned getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;
  unsigned getBranchTargetOpValue(const MCInst &MI, unsigned OpNo,
                                  SmallVectorImpl<MCFixup> &Fixups,
                                  const MCSubtargetInfo &STI) const;
  unsigned getLongDispMemOpValue(const MCInst &MI, unsigned OpNo,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;
  unsigned getNarrowDispMemOpValue(const MCInst &MI, unsigned OpNo,
                                   SmallVectorImpl<MCFixup> &Fixups,
                                   const MCSubtargetInfo &STI) const;

public:
  SHMCCodeEmitter(const MCInstrInfo &MCII, MCContext &Ctx)
      : Ctx(Ctx), MCII(MCII) {}

  void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &CB,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;
};

} // namespace

void SHMCCodeEmitter::encodeInstruction(const MCInst &MI,
                                        SmallVectorImpl<char> &CB,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        const MCSubtargetInfo &STI) const {
  assert(MCII.get(MI.getOpcode()).getSize() == 2 &&
         "unexpected SH instruction size");
  uint16_t Bits = getBinaryCodeForInstr(MI, Fixups, STI);
  support::endian::write(CB, Bits,
                         STI.getTargetTriple().isLittleEndian()
                             ? endianness::little
                             : endianness::big);
}

unsigned SHMCCodeEmitter::getMachineOpValue(const MCInst &MI,
                                            const MCOperand &MO,
                                            SmallVectorImpl<MCFixup> &Fixups,
                                            const MCSubtargetInfo &STI) const {
  if (MO.isReg())
    return Ctx.getRegisterInfo()->getEncodingValue(MO.getReg());
  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());

  Ctx.reportError(MI.getLoc(), "relocatable expressions are not supported");
  return 0;
}

unsigned
SHMCCodeEmitter::getBranchTargetOpValue(const MCInst &MI, unsigned OpNo,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpNo);
  if (MO.isImm()) {
    int64_t ByteDisp = MO.getImm();
    if (ByteDisp % 2 != 0) {
      Ctx.reportError(MI.getLoc(), "SH branch target must be two-byte aligned");
      return 0;
    }
    unsigned Width =
        MI.getOpcode() == SH::BRA || MI.getOpcode() == SH::BSR ? 12 : 8;
    if (!isIntN(Width, ByteDisp / 2)) {
      Ctx.reportError(MI.getLoc(), "SH branch target is out of range");
      return 0;
    }
    return static_cast<unsigned>(ByteDisp / 2);
  }

  assert(MO.isExpr() && "expected SH branch target expression");
  MCFixupKind Kind = MI.getOpcode() == SH::BRA || MI.getOpcode() == SH::BSR
                         ? SH::fixup_SH_PCREL12_2
                         : SH::fixup_SH_PCREL8_2;
  Fixups.push_back(MCFixup::create(0, MO.getExpr(), Kind, true));
  return 0;
}

unsigned
SHMCCodeEmitter::getLongDispMemOpValue(const MCInst &MI, unsigned OpNo,
                                       SmallVectorImpl<MCFixup> &Fixups,
                                       const MCSubtargetInfo &STI) const {
  const MCOperand &Base = MI.getOperand(OpNo);
  const MCOperand &Disp = MI.getOperand(OpNo + 1);
  if (!Base.isReg() || !Disp.isImm()) {
    Ctx.reportError(MI.getLoc(),
                    "expected register and integer longword displacement");
    return 0;
  }

  int64_t ByteDisp = Disp.getImm();
  if (ByteDisp < 0 || ByteDisp > 60 || ByteDisp % 4 != 0) {
    Ctx.reportError(MI.getLoc(),
                    "longword displacement must be a multiple of 4 in the "
                    "range [0, 60]");
    return 0;
  }

  unsigned Reg = Ctx.getRegisterInfo()->getEncodingValue(Base.getReg());
  return (Reg << 4) | static_cast<unsigned>(ByteDisp / 4);
}

unsigned
SHMCCodeEmitter::getNarrowDispMemOpValue(const MCInst &MI, unsigned OpNo,
                                         SmallVectorImpl<MCFixup> &Fixups,
                                         const MCSubtargetInfo &STI) const {
  const MCOperand &FixedReg = MI.getOperand(0);
  const MCOperand &Base = MI.getOperand(OpNo);
  const MCOperand &Disp = MI.getOperand(OpNo + 1);
  if (!FixedReg.isReg() || FixedReg.getReg() != SH::R0) {
    Ctx.reportError(MI.getLoc(),
                    "byte/word displacement data register must be r0");
    return 0;
  }
  if (!Base.isReg() || !Disp.isImm()) {
    Ctx.reportError(MI.getLoc(),
                    "expected register and integer byte/word displacement");
    return 0;
  }

  int64_t ByteDisp = Disp.getImm();
  bool IsWord = MI.getOpcode() == SH::MOVW_load_disp ||
                MI.getOpcode() == SH::MOVW_store_disp;
  if (IsWord && (ByteDisp < 0 || ByteDisp > 30 || ByteDisp % 2 != 0)) {
    Ctx.reportError(MI.getLoc(),
                    "word displacement must be an even byte offset in the "
                    "range [0, 30]");
    return 0;
  }
  if (!IsWord && (ByteDisp < 0 || ByteDisp > 15)) {
    Ctx.reportError(MI.getLoc(),
                    "byte displacement must be in the range [0, 15]");
    return 0;
  }

  unsigned Reg = Ctx.getRegisterInfo()->getEncodingValue(Base.getReg());
  unsigned EncodedDisp =
      static_cast<unsigned>(IsWord ? ByteDisp / 2 : ByteDisp);
  return (Reg << 4) | EncodedDisp;
}

#include "SHGenMCCodeEmitter.inc"

MCCodeEmitter *llvm::createSHMCCodeEmitter(const MCInstrInfo &MCII,
                                           MCContext &Ctx) {
  return new SHMCCodeEmitter(MCII, Ctx);
}
