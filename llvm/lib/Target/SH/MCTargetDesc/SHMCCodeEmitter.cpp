//===-- SHMCCodeEmitter.cpp - Convert SH instructions to machine code -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHBaseInfo.h"
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
  unsigned getPCLiteralOpValue(const MCInst &MI, unsigned OpNo,
                               SmallVectorImpl<MCFixup> &Fixups,
                               const MCSubtargetInfo &STI) const;
  unsigned getPCWordLiteralOpValue(const MCInst &MI, unsigned OpNo,
                                   SmallVectorImpl<MCFixup> &Fixups,
                                   const MCSubtargetInfo &STI) const;
  unsigned getLongDispMemOpValue(const MCInst &MI, unsigned OpNo,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;
  unsigned getNarrowDispMemOpValue(const MCInst &MI, unsigned OpNo,
                                   SmallVectorImpl<MCFixup> &Fixups,
                                   const MCSubtargetInfo &STI) const;
  unsigned getGBRDispOpValue(const MCInst &MI, unsigned OpNo,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;
  unsigned getExtendedMemDispOpValue(const MCInst &MI, unsigned OpNo,
                                     SmallVectorImpl<MCFixup> &Fixups,
                                     const MCSubtargetInfo &STI) const;
  unsigned getSImm20By8OpValue(const MCInst &MI, unsigned OpNo,
                               SmallVectorImpl<MCFixup> &Fixups,
                               const MCSubtargetInfo &STI) const;
  unsigned getSImm20OpValue(const MCInst &MI, unsigned OpNo,
                            SmallVectorImpl<MCFixup> &Fixups,
                            const MCSubtargetInfo &STI) const;
  unsigned getTBRDispOpValue(const MCInst &MI, unsigned OpNo,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;
  unsigned getDSPLoopBoundsOpValue(const MCInst &MI, unsigned OpNo,
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
  unsigned Size = MCII.get(MI.getOpcode()).getSize();
  assert((Size == 2 || Size == 4) && "unexpected SH instruction size");
  uint64_t Bits = getBinaryCodeForInstr(MI, Fixups, STI);
  endianness Endian = STI.getTargetTriple().isLittleEndian()
                          ? endianness::little
                          : endianness::big;
  if (Size == 2) {
    support::endian::write(CB, static_cast<uint16_t>(Bits), Endian);
    return;
  }
  // SH-2A preserves architectural halfword order in both endian modes. Each
  // halfword is byte-swapped independently for little-endian objects.
  support::endian::write(CB, static_cast<uint16_t>(Bits >> 16), Endian);
  support::endian::write(CB, static_cast<uint16_t>(Bits), Endian);
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
    if (ByteDisp % SH::BranchDisplacementScale != 0) {
      Ctx.reportError(MI.getLoc(), "SH branch target must be two-byte aligned");
      return 0;
    }
    unsigned Width = MI.getOpcode() == SH::BRA || MI.getOpcode() == SH::BSR
                         ? SH::UnconditionalBranchDisplacementBits
                         : SH::ConditionalBranchDisplacementBits;
    if (!SH::isBranchDisplacementInRange(ByteDisp, Width)) {
      Ctx.reportError(MI.getLoc(), "SH branch target is out of range");
      return 0;
    }
    return static_cast<unsigned>(ByteDisp / SH::BranchDisplacementScale);
  }

  assert(MO.isExpr() && "expected SH branch target expression");
  MCFixupKind Kind;
  if (MI.getOpcode() == SH::BSR)
    Kind = SH::fixup_SH_BSR12_2;
  else if (MI.getOpcode() == SH::BRA)
    Kind = SH::fixup_SH_PCREL12_2;
  else
    Kind = SH::fixup_SH_PCREL8_2;
  Fixups.push_back(MCFixup::create(0, MO.getExpr(), Kind, true));
  return 0;
}

unsigned
SHMCCodeEmitter::getPCLiteralOpValue(const MCInst &MI, unsigned OpNo,
                                     SmallVectorImpl<MCFixup> &Fixups,
                                     const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpNo);
  if (MO.isImm()) {
    int64_t ByteDisp = MO.getImm();
    if (ByteDisp < 0) {
      Ctx.reportError(
          MI.getLoc(),
          "SH PC-relative literal target is behind the instruction");
      return 0;
    }
    if (ByteDisp % SH::PCRelativeLongScale != 0) {
      Ctx.reportError(
          MI.getLoc(),
          "SH PC-relative literal target must be four-byte aligned");
      return 0;
    }
    if (ByteDisp > SH::PCRelativeLongMaxDisplacement) {
      Ctx.reportError(MI.getLoc(),
                      "SH PC-relative literal target is out of range");
      return 0;
    }
    return static_cast<unsigned>(ByteDisp / SH::PCRelativeLongScale);
  }

  assert(MO.isExpr() && "expected SH PC-relative literal expression");
  Fixups.push_back(
      MCFixup::create(0, MO.getExpr(), SH::fixup_SH_PCREL8_4, true));
  return 0;
}

unsigned
SHMCCodeEmitter::getPCWordLiteralOpValue(const MCInst &MI, unsigned OpNo,
                                         SmallVectorImpl<MCFixup> &Fixups,
                                         const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpNo);
  if (MO.isImm()) {
    int64_t ByteDisp = MO.getImm();
    if (!SH::isPCRelativeWordDisplacementInRange(ByteDisp)) {
      Ctx.reportError(MI.getLoc(),
                      "SH PC-relative word displacement must be an even byte "
                      "offset in the range [0, 510]");
      return 0;
    }
    return static_cast<unsigned>(ByteDisp / SH::PCRelativeWordScale);
  }

  assert(MO.isExpr() && "expected SH PC-relative word expression");
  Fixups.push_back(
      MCFixup::create(0, MO.getExpr(), SH::fixup_SH_PCREL8_2_LITERAL, true));
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

unsigned SHMCCodeEmitter::getGBRDispOpValue(const MCInst &MI, unsigned OpNo,
                                            SmallVectorImpl<MCFixup> &Fixups,
                                            const MCSubtargetInfo &STI) const {
  const MCOperand &Disp = MI.getOperand(OpNo);
  if (!Disp.isImm()) {
    Ctx.reportError(MI.getLoc(), "expected integer GBR displacement");
    return 0;
  }

  int64_t ByteDisp = Disp.getImm();
  unsigned Scale = 1;
  switch (MI.getOpcode()) {
  case SH::MOVW_load_gbr:
  case SH::MOVW_store_gbr:
    Scale = 2;
    break;
  case SH::MOVL_load_gbr:
  case SH::MOVL_store_gbr:
    Scale = 4;
    break;
  default:
    break;
  }
  if (ByteDisp < 0 || ByteDisp > 255 * Scale || ByteDisp % Scale != 0) {
    Ctx.reportError(MI.getLoc(), "invalid GBR displacement");
    return 0;
  }
  return static_cast<unsigned>(ByteDisp / Scale);
}

unsigned
SHMCCodeEmitter::getExtendedMemDispOpValue(const MCInst &MI, unsigned OpNo,
                                           SmallVectorImpl<MCFixup> &Fixups,
                                           const MCSubtargetInfo &STI) const {
  const MCOperand &Base = MI.getOperand(OpNo);
  const MCOperand &Disp = MI.getOperand(OpNo + 1);
  if (!Base.isReg() || !Disp.isImm()) {
    Ctx.reportError(MI.getLoc(),
                    "expected register and integer SH-2A displacement");
    return 0;
  }
  unsigned Scale;
  switch (MI.getOpcode()) {
  case SH::MOVW_LOAD_DISP12:
  case SH::MOVUW_LOAD_DISP12:
  case SH::MOVW_STORE_DISP12:
    Scale = 2;
    break;
  case SH::MOVL_LOAD_DISP12:
  case SH::MOVL_STORE_DISP12:
  case SH::FMOV_S_LOAD_DISP12:
  case SH::FMOV_S_STORE_DISP12:
    Scale = 4;
    break;
  case SH::FMOV_D_LOAD_DISP12:
  case SH::FMOV_D_STORE_DISP12:
    Scale = 8;
    break;
  default:
    Scale = 1;
    break;
  }
  int64_t ByteDisp = Disp.getImm();
  if (ByteDisp < 0 || ByteDisp > 4095 * Scale || ByteDisp % Scale != 0) {
    Ctx.reportError(MI.getLoc(), "invalid SH-2A 12-bit displacement");
    return 0;
  }
  unsigned Reg = Ctx.getRegisterInfo()->getEncodingValue(Base.getReg());
  return (Reg << 12) | static_cast<unsigned>(ByteDisp / Scale);
}

unsigned
SHMCCodeEmitter::getSImm20By8OpValue(const MCInst &MI, unsigned OpNo,
                                     SmallVectorImpl<MCFixup> &Fixups,
                                     const MCSubtargetInfo &STI) const {
  const MCOperand &Operand = MI.getOperand(OpNo);
  if (!Operand.isImm() || !isInt<28>(Operand.getImm()) ||
      Operand.getImm() % 256 != 0) {
    Ctx.reportError(
        MI.getLoc(),
        "movi20s immediate must be a signed 28-bit multiple of 256");
    return 0;
  }
  return static_cast<unsigned>(Operand.getImm() >> 8) & 0xfffff;
}

unsigned SHMCCodeEmitter::getSImm20OpValue(const MCInst &MI, unsigned OpNo,
                                           SmallVectorImpl<MCFixup> &Fixups,
                                           const MCSubtargetInfo &STI) const {
  const MCOperand &Operand = MI.getOperand(OpNo);
  if (Operand.isImm())
    return static_cast<unsigned>(Operand.getImm()) & 0xfffff;
  assert(Operand.isExpr() && "expected SH movi20 expression");
  Fixups.push_back(MCFixup::create(0, Operand.getExpr(), SH::fixup_SH_MOVI20));
  return 0;
}

unsigned SHMCCodeEmitter::getTBRDispOpValue(const MCInst &MI, unsigned OpNo,
                                            SmallVectorImpl<MCFixup> &Fixups,
                                            const MCSubtargetInfo &STI) const {
  const MCOperand &Operand = MI.getOperand(OpNo);
  if (!Operand.isImm() || Operand.getImm() < 0 || Operand.getImm() > 1020 ||
      Operand.getImm() % 4 != 0) {
    Ctx.reportError(
        MI.getLoc(),
        "TBR displacement must be a multiple of 4 in the range [0, 1020]");
    return 0;
  }
  return static_cast<unsigned>(Operand.getImm() / 4);
}

unsigned
SHMCCodeEmitter::getDSPLoopBoundsOpValue(const MCInst &MI, unsigned OpNo,
                                         SmallVectorImpl<MCFixup> &Fixups,
                                         const MCSubtargetInfo &STI) const {
  const MCOperand &Start = MI.getOperand(OpNo);
  const MCOperand &End = MI.getOperand(OpNo + 1);
  assert(Start.isExpr() && End.isExpr() &&
         "expected DSP loop-bound expressions");
  Fixups.push_back(
      MCFixup::create(0, Start.getExpr(), SH::fixup_SH_LOOP_START, false));
  Fixups.push_back(
      MCFixup::create(0, End.getExpr(), SH::fixup_SH_LOOP_END, false));
  return MI.getOpcode() == SH::DSP_LOOP_START ? 0x8c : 0x8e;
}

#include "SHGenMCCodeEmitter.inc"

MCCodeEmitter *llvm::createSHMCCodeEmitter(const MCInstrInfo &MCII,
                                           MCContext &Ctx) {
  return new SHMCCodeEmitter(MCII, Ctx);
}
