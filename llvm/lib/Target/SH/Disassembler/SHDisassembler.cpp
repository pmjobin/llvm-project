//===-- SHDisassembler.cpp - Disassembler for SH -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/SHMCTargetDesc.h"
#include "TargetInfo/SHTargetInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDecoder.h"
#include "llvm/MC/MCDecoderOps.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
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

static MCDisassembler::DecodeStatus
DecodeGPRRegisterClass(MCInst &MI, uint64_t RegNo, uint64_t Address,
                       const MCDisassembler *Decoder) {
  if (RegNo >= std::size(GPRDecoderTable))
    return MCDisassembler::Fail;
  MI.addOperand(MCOperand::createReg(GPRDecoderTable[RegNo]));
  return MCDisassembler::Success;
}

static MCDisassembler::DecodeStatus decodeSImm8(MCInst &MI, uint64_t Imm,
                                                uint64_t Address,
                                                const MCDisassembler *Decoder) {
  MI.addOperand(MCOperand::createImm(SignExtend64<8>(Imm)));
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

#include "SHGenDisassemblerTables.inc"

MCDisassembler::DecodeStatus
SHDisassembler::getInstruction(MCInst &MI, uint64_t &Size,
                               ArrayRef<uint8_t> Bytes, uint64_t Address,
                               raw_ostream &CStream) const {
  if (Bytes.size() < 2) {
    Size = 0;
    return Fail;
  }

  Size = 2;
  uint16_t Insn = getSubtargetInfo().getTargetTriple().isLittleEndian()
                      ? support::endian::read16le(Bytes.data())
                      : support::endian::read16be(Bytes.data());
  return decodeInstruction(DecoderTable16, MI, Insn, Address, this,
                           getSubtargetInfo());
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
