//===-- SHDisassembler.cpp - Disassembler for SH -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/SHBaseInfo.h"
#include "MCTargetDesc/SHDSPInfo.h"
#include "MCTargetDesc/SHMCTargetDesc.h"
#include "TargetInfo/SHTargetInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDecoder.h"
#include "llvm/MC/MCDecoderOps.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/TargetParser/Triple.h"

#define DEBUG_TYPE "sh-disassembler"

using namespace llvm;
using namespace llvm::MCD;

using DecodeStatus = MCDisassembler::DecodeStatus;

namespace {

class SHDisassembler : public MCDisassembler {
public:
  SHDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx)
      : MCDisassembler(STI, Ctx) {}

  DecodeStatus getInstruction(MCInst &MI, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t Address,
                              raw_ostream &CStream) const override;
};

} // namespace

static const MCRegister GPRDecoderTable[] = {
    SH::R0, SH::R1, SH::R2,  SH::R3,  SH::R4,  SH::R5,  SH::R6,  SH::R7,
    SH::R8, SH::R9, SH::R10, SH::R11, SH::R12, SH::R13, SH::R14, SH::R15};

static const MCRegister BankedGPRDecoderTable[] = {
    SH::R0_BANK, SH::R1_BANK, SH::R2_BANK, SH::R3_BANK,
    SH::R4_BANK, SH::R5_BANK, SH::R6_BANK, SH::R7_BANK};

static const MCRegister FRDecoderTable[] = {
    SH::FR0,  SH::FR1,  SH::FR2,  SH::FR3, SH::FR4,  SH::FR5,
    SH::FR6,  SH::FR7,  SH::FR8,  SH::FR9, SH::FR10, SH::FR11,
    SH::FR12, SH::FR13, SH::FR14, SH::FR15};

static const MCRegister DRDecoderTable[] = {
    SH::DR0, SH::DR2, SH::DR4, SH::DR6, SH::DR8, SH::DR10, SH::DR12, SH::DR14};

static const MCRegister FVDecoderTable[] = {SH::FV0, SH::FV4, SH::FV8,
                                            SH::FV12};

static MCDisassembler::DecodeStatus
DecodeGPRRegisterClass(MCInst &MI, uint64_t RegNo, uint64_t Address,
                       const MCDisassembler *Decoder) {
  if (RegNo >= std::size(GPRDecoderTable))
    return MCDisassembler::Fail;
  MI.addOperand(MCOperand::createReg(GPRDecoderTable[RegNo]));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus
DecodeBankedGPRRegisterClass(MCInst &MI, uint64_t RegNo, uint64_t Address,
                             const MCDisassembler *Decoder) {
  if (RegNo >= std::size(BankedGPRDecoderTable))
    return MCDisassembler::Fail;
  MI.addOperand(MCOperand::createReg(BankedGPRDecoderTable[RegNo]));
  return MCDisassembler::Success;
}

template <size_t N>
static MCDisassembler::DecodeStatus
decodeFPURegisterClass(MCInst &MI, uint64_t RegNo,
                       const MCRegister (&Table)[N]) {
  if (RegNo >= N)
    return MCDisassembler::Fail;
  MI.addOperand(MCOperand::createReg(Table[RegNo]));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus
DecodeFRRegisterClass(MCInst &MI, uint64_t RegNo, uint64_t Address,
                      const MCDisassembler *Decoder) {
  return decodeFPURegisterClass(MI, RegNo, FRDecoderTable);
}

static MCDisassembler::DecodeStatus
DecodeDRRegisterClass(MCInst &MI, uint64_t RegNo, uint64_t Address,
                      const MCDisassembler *Decoder) {
  return decodeFPURegisterClass(MI, RegNo, DRDecoderTable);
}

static MCDisassembler::DecodeStatus
DecodeFVRegisterClass(MCInst &MI, uint64_t RegNo, uint64_t Address,
                      const MCDisassembler *Decoder) {
  return decodeFPURegisterClass(MI, RegNo, FVDecoderTable);
}

static MCDisassembler::DecodeStatus
decodeR0RegisterClass(MCInst &MI, uint64_t RegNo, uint64_t Address,
                      const MCDisassembler *Decoder) {
  MI.addOperand(MCOperand::createReg(SH::R0));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus
decodeSHDSPDDT(MCInst &MI, uint64_t Insn, uint64_t Address,
               const MCDisassembler *Decoder) {
  if (!SH::isValidDSPInstruction(0xf000 | (Insn & 0x3ff), 2))
    return MCDisassembler::Fail;
  MI.addOperand(MCOperand::createImm(Insn & 0x3ff));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus
decodeSHDSPSDT(MCInst &MI, uint64_t Insn, uint64_t Address,
               const MCDisassembler *Decoder) {
  if (!SH::isValidDSPInstruction(0xf400 | (Insn & 0x3ff), 2))
    return MCDisassembler::Fail;
  MI.addOperand(MCOperand::createImm(Insn & 0x3ff));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus
decodeSHDSPPPI(MCInst &MI, uint64_t Insn, uint64_t Address,
               const MCDisassembler *Decoder) {
  if (!SH::isValidDSPInstruction(0xf8000000 | (Insn & 0x3ffffff), 4))
    return MCDisassembler::Fail;
  MI.addOperand(MCOperand::createImm(Insn & 0x3ffffff));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus
decodeSH2AMovm(MCInst &MI, uint64_t Insn, uint64_t Address,
               const MCDisassembler *Decoder) {
  if (DecodeGPRRegisterClass(MI, fieldFromInstruction(Insn, 8, 4), Address,
                             Decoder) == MCDisassembler::Fail)
    return MCDisassembler::Fail;
  MI.addOperand(MCOperand::createReg(SH::R15));
  return MCDisassembler::Success;
}

static void applySH2APublicFPMode(MCInst &MI, const MCSubtargetInfo &STI) {
  if (!STI.getFeatureBits()[SH::FeatureFPUSingleMode])
    return;
  if (MI.getOpcode() == SH::FMOV_D_LOAD_DISP12)
    MI.setOpcode(SH::FMOV_S_LOAD_DISP12);
  else if (MI.getOpcode() == SH::FMOV_D_STORE_DISP12)
    MI.setOpcode(SH::FMOV_S_STORE_DISP12);
  else
    return;
  MCRegister Register = MI.getOperand(0).getReg();
  for (unsigned Index = 0; Index != std::size(DRDecoderTable); ++Index)
    if (DRDecoderTable[Index] == Register) {
      MI.getOperand(0).setReg(FRDecoderTable[Index * 2]);
      return;
    }
}

static MCDisassembler::DecodeStatus
decodeGBRRegisterClass(MCInst &MI, uint64_t RegNo, uint64_t Address,
                       const MCDisassembler *Decoder) {
  MI.addOperand(MCOperand::createReg(SH::GBR));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus decodeSImm8(MCInst &MI, uint64_t Imm,
                                                uint64_t Address,
                                                const MCDisassembler *Decoder) {
  MI.addOperand(MCOperand::createImm(SignExtend64<8>(Imm)));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus
decodeSImm20(MCInst &MI, uint64_t Imm, uint64_t Address,
             const MCDisassembler *Decoder) {
  MI.addOperand(MCOperand::createImm(SignExtend64<20>(Imm)));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus
decodeSImm20By8(MCInst &MI, uint64_t Imm, uint64_t Address,
                const MCDisassembler *Decoder) {
  MI.addOperand(MCOperand::createImm(SignExtend64<20>(Imm) * 256));
  return MCDisassembler::Success;
}

template <unsigned Width>
static MCDisassembler::DecodeStatus
decodeBranchTarget(MCInst &MI, uint64_t Value, uint64_t Address,
                   const MCDisassembler *Decoder) {
  int64_t ByteDisp = SignExtend64<Width>(Value) * SH::BranchDisplacementScale;
  uint64_t Target = Address + SH::PCRelativeBias + ByteDisp;
  if (!Decoder->tryAddingSymbolicOperand(MI, Target, Address, true, 0,
                                         SH::BranchInstructionSize,
                                         SH::BranchInstructionSize))
    MI.addOperand(MCOperand::createImm(ByteDisp));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus
decodeBranchTarget8(MCInst &MI, uint64_t Value, uint64_t Address,
                    const MCDisassembler *Decoder) {
  return decodeBranchTarget<8>(MI, Value, Address, Decoder);
}

static MCDisassembler::DecodeStatus
decodeBranchTarget12(MCInst &MI, uint64_t Value, uint64_t Address,
                     const MCDisassembler *Decoder) {
  return decodeBranchTarget<12>(MI, Value, Address, Decoder);
}

static MCDisassembler::DecodeStatus
decodePCLiteral(MCInst &MI, uint64_t Value, uint64_t Address,
                const MCDisassembler *Decoder) {
  uint64_t ByteDisp = Value * SH::PCRelativeLongScale;
  uint64_t Target = SH::getPCRelativeLongBase(Address) + ByteDisp;
  if (!Decoder->tryAddingSymbolicOperand(MI, Target, Address, false, 0,
                                         SH::PCRelativeLiteralInstructionSize,
                                         SH::PCRelativeLiteralInstructionSize))
    MI.addOperand(MCOperand::createImm(ByteDisp));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus
decodePCWordLiteral(MCInst &MI, uint64_t Value, uint64_t Address,
                    const MCDisassembler *Decoder) {
  uint64_t ByteDisp = Value * SH::PCRelativeWordScale;
  uint64_t Target = Address + SH::PCRelativeBias + ByteDisp;
  if (!Decoder->tryAddingSymbolicOperand(MI, Target, Address, false, 0,
                                         SH::PCRelativeLiteralInstructionSize,
                                         SH::PCRelativeLiteralInstructionSize))
    MI.addOperand(MCOperand::createImm(ByteDisp));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus
decodeLongDispMemOperand(MCInst &MI, uint64_t Value, uint64_t Address,
                         const MCDisassembler *Decoder) {
  if (DecodeGPRRegisterClass(MI, Value >> 4, Address, Decoder) ==
      MCDisassembler::Fail)
    return MCDisassembler::Fail;
  MI.addOperand(MCOperand::createImm((Value & 0xf) * 4));
  return MCDisassembler::Success;
}

template <unsigned Scale>
static MCDisassembler::DecodeStatus
decodeNarrowDispMemOperand(MCInst &MI, uint64_t Value, uint64_t Address,
                           const MCDisassembler *Decoder) {
  if (DecodeGPRRegisterClass(MI, Value >> 4, Address, Decoder) ==
      MCDisassembler::Fail)
    return MCDisassembler::Fail;
  MI.addOperand(MCOperand::createImm((Value & 0xf) * Scale));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus
decodeByteDispMemOperand(MCInst &MI, uint64_t Value, uint64_t Address,
                         const MCDisassembler *Decoder) {
  return decodeNarrowDispMemOperand<1>(MI, Value, Address, Decoder);
}

static MCDisassembler::DecodeStatus
decodeWordDispMemOperand(MCInst &MI, uint64_t Value, uint64_t Address,
                         const MCDisassembler *Decoder) {
  return decodeNarrowDispMemOperand<2>(MI, Value, Address, Decoder);
}

template <unsigned Scale>
static MCDisassembler::DecodeStatus
decodeExtendedMemDispOperand(MCInst &MI, uint64_t Value, uint64_t Address,
                             const MCDisassembler *Decoder) {
  if (DecodeGPRRegisterClass(MI, Value >> 12, Address, Decoder) ==
      MCDisassembler::Fail)
    return MCDisassembler::Fail;
  MI.addOperand(MCOperand::createImm((Value & 0xfff) * Scale));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus
decodeExtendedByteMemDispOperand(MCInst &MI, uint64_t Value, uint64_t Address,
                                 const MCDisassembler *Decoder) {
  return decodeExtendedMemDispOperand<1>(MI, Value, Address, Decoder);
}

static MCDisassembler::DecodeStatus
decodeExtendedWordMemDispOperand(MCInst &MI, uint64_t Value, uint64_t Address,
                                 const MCDisassembler *Decoder) {
  return decodeExtendedMemDispOperand<2>(MI, Value, Address, Decoder);
}

static MCDisassembler::DecodeStatus
decodeExtendedLongMemDispOperand(MCInst &MI, uint64_t Value, uint64_t Address,
                                 const MCDisassembler *Decoder) {
  return decodeExtendedMemDispOperand<4>(MI, Value, Address, Decoder);
}

static MCDisassembler::DecodeStatus
decodeExtendedDoubleMemDispOperand(MCInst &MI, uint64_t Value, uint64_t Address,
                                   const MCDisassembler *Decoder) {
  return decodeExtendedMemDispOperand<8>(MI, Value, Address, Decoder);
}

static MCDisassembler::DecodeStatus
decodeTBRDisp(MCInst &MI, uint64_t Value, uint64_t Address,
              const MCDisassembler *Decoder) {
  MI.addOperand(MCOperand::createImm(Value * 4));
  return MCDisassembler::Success;
}

template <unsigned Scale>
static MCDisassembler::DecodeStatus
decodeGBRDispOperand(MCInst &MI, uint64_t Value, uint64_t Address,
                     const MCDisassembler *Decoder) {
  MI.addOperand(MCOperand::createImm(Value * Scale));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus
decodeGBRByteDispOperand(MCInst &MI, uint64_t Value, uint64_t Address,
                         const MCDisassembler *Decoder) {
  return decodeGBRDispOperand<1>(MI, Value, Address, Decoder);
}

static MCDisassembler::DecodeStatus
decodeGBRWordDispOperand(MCInst &MI, uint64_t Value, uint64_t Address,
                         const MCDisassembler *Decoder) {
  return decodeGBRDispOperand<2>(MI, Value, Address, Decoder);
}

static MCDisassembler::DecodeStatus
decodeGBRLongDispOperand(MCInst &MI, uint64_t Value, uint64_t Address,
                         const MCDisassembler *Decoder) {
  return decodeGBRDispOperand<4>(MI, Value, Address, Decoder);
}

#include "SHGenDisassemblerTables.inc"

MCDisassembler::DecodeStatus
SHDisassembler::getInstruction(MCInst &MI, uint64_t &Size,
                               ArrayRef<uint8_t> Bytes, uint64_t Address,
                               raw_ostream &CStream) const {
  if (Bytes.size() < SH::InstructionAlignment) {
    Size = 0;
    return Fail;
  }

  bool IsLittleEndian = getSubtargetInfo().getTargetTriple().isLittleEndian();
  uint16_t First = IsLittleEndian ? support::endian::read16le(Bytes.data())
                                  : support::endian::read16be(Bytes.data());
  Size = SH::InstructionAlignment;
  DecodeStatus Result = Fail;
  if (getSubtargetInfo().getFeatureBits()[SH::FeatureDSP])
    Result = decodeInstruction(DecoderTableSHDSP16, MI, First, Address, this,
                               getSubtargetInfo());
  if (Result == Fail) {
    MI.clear();
    Result = decodeInstruction(DecoderTable16, MI, First, Address, this,
                               getSubtargetInfo());
  }
  if (Result != Fail)
    return Result;
  MI.clear();
  if (Bytes.size() < 4)
    return Fail;
  uint16_t Second = IsLittleEndian
                        ? support::endian::read16le(Bytes.data() + 2)
                        : support::endian::read16be(Bytes.data() + 2);
  uint32_t Insn = (static_cast<uint32_t>(First) << 16) | Second;
  if (getSubtargetInfo().getFeatureBits()[SH::FeatureDSP])
    Result = decodeInstruction(DecoderTableSHDSP32, MI, Insn, Address, this,
                               getSubtargetInfo());
  if (Result == Fail) {
    MI.clear();
    Result = decodeInstruction(DecoderTable32, MI, Insn, Address, this,
                               getSubtargetInfo());
  }
  if (Result != Fail) {
    applySH2APublicFPMode(MI, getSubtargetInfo());
    Size = 4;
  } else
    MI.clear();
  return Result;
}

static MCDisassembler *createSHDisassembler(const Target &T,
                                            const MCSubtargetInfo &STI,
                                            MCContext &Ctx) {
  return new SHDisassembler(STI, Ctx);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeSHDisassembler() {
  TargetRegistry::RegisterMCDisassembler(getTheSHTarget(),
                                         createSHDisassembler);
  TargetRegistry::RegisterMCDisassembler(getTheSHLETarget(),
                                         createSHDisassembler);
}
