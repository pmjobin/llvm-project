//===-- SHInstrInfo.td - SuperH instruction definitions ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "SHInstrFormats.td"

def BranchTargetAsmOperand : AsmOperandClass {
  let Name = "BranchTarget";
  let PredicateMethod = "isBranchTarget";
  let RenderMethod = "addBranchTargetOperands";
  let ParserMethod = "parseBranchTarget";
}

def brtarget8 : Operand<OtherVT> {
  let ParserMatchClass = BranchTargetAsmOperand;
  let PrintMethod = "printBranchTarget";
  let EncoderMethod = "getBranchTargetOpValue";
  let DecoderMethod = "decodeBranchTarget8";
  let OperandType = "OPERAND_PCREL";
}

def brtarget12 : Operand<OtherVT> {
  let ParserMatchClass = BranchTargetAsmOperand;
  let PrintMethod = "printBranchTarget";
  let EncoderMethod = "getBranchTargetOpValue";
  let DecoderMethod = "decodeBranchTarget12";
  let OperandType = "OPERAND_PCREL";
}

def calltarget12 : Operand<i32> {
  let ParserMatchClass = BranchTargetAsmOperand;
  let PrintMethod = "printBranchTarget";
  let EncoderMethod = "getBranchTargetOpValue";
  let DecoderMethod = "decodeBranchTarget12";
  let OperandType = "OPERAND_PCREL";
}

def TBRDispAsmOperand : AsmOperandClass {
	let Name = "TBRDisp";
	let PredicateMethod = "isTBRDisp";
	let RenderMethod = "addTBRDispOperands";
	let ParserMethod = "parseTBRDisp";
	let DiagnosticType = "InvalidTBRDisp";
}

def tbrdisp : Operand<i32> {
	let ParserMatchClass = TBRDispAsmOperand;
	let PrintMethod = "printTBRDisp";
	let EncoderMethod = "getTBRDispOpValue";
	let DecoderMethod = "decodeTBRDisp";
}

def PCLiteralAsmOperand : AsmOperandClass {
  let Name = "PCLiteral";
  let PredicateMethod = "isPCLiteral";
  let RenderMethod = "addPCLiteralOperands";
  let ParserMethod = "parsePCLiteral";
  let DiagnosticType = "InvalidPCLiteral";
}

def PCWordLiteralAsmOperand : AsmOperandClass {
	let Name = "PCWordLiteral";
	let PredicateMethod = "isPCWordLiteral";
	let RenderMethod = "addPCLiteralOperands";
	let ParserMethod = "parsePCWordLiteral";
	let DiagnosticType = "InvalidPCWordLiteral";
}

def pcliteral : Operand<iPTR> {
  let ParserMatchClass = PCLiteralAsmOperand;
  let PrintMethod = "printPCLiteral";
  let EncoderMethod = "getPCLiteralOpValue";
  let DecoderMethod = "decodePCLiteral";
  let OperandType = "OPERAND_PCREL";
}

def pcwordliteral : Operand<iPTR> {
	let ParserMatchClass = PCWordLiteralAsmOperand;
	let PrintMethod = "printPCLiteral";
	let EncoderMethod = "getPCWordLiteralOpValue";
	let DecoderMethod = "decodePCWordLiteral";
	let OperandType = "OPERAND_PCREL";
}

def island_instance : OperandWithDefaultOps<i32, (ops (i32 -1))>;

def cpinst_operand : Operand<i32>;

def SImm8AsmOperand : AsmOperandClass {
  let Name = "SImm8";
  let PredicateMethod = "isSImm8";
  let RenderMethod = "addImmOperands";
  let ParserMethod = "parseSImm8";
  let DiagnosticType = "InvalidSImm8";
}

def UImm8AsmOperand : AsmOperandClass {
	let Name = "UImm8";
	let PredicateMethod = "isUImm8";
	let RenderMethod = "addImmOperands";
	let ParserMethod = "parseUImm8";
	let DiagnosticType = "InvalidUImm8";
}

def simm8 : Operand<i32>, ImmLeaf<i32, [{ return isInt<8>(Imm); }]> {
  let ParserMatchClass = SImm8AsmOperand;
  let PrintMethod = "printSImm8";
  let DecoderMethod = "decodeSImm8";
}

def uimm8 : Operand<i32>, ImmLeaf<i32, [{ return isUInt<8>(Imm); }]> {
	let ParserMatchClass = UImm8AsmOperand;
	let PrintMethod = "printUImm8";
}

def DSPEncodingAsmOperand : AsmOperandClass {
	let Name = "DSPEncoding";
	let PredicateMethod = "isImm";
	let RenderMethod = "addImmOperands";
}

def dspencoding : Operand<i32> {
	let ParserMatchClass = DSPEncodingAsmOperand;
}

def dsploopbounds : Operand<i32> {
	let EncoderMethod = "getDSPLoopBoundsOpValue";
	let MIOperandInfo = (ops brtarget8:$start, brtarget8:$end);
}

def SImm20AsmOperand : AsmOperandClass {
	let Name = "SImm20";
	let PredicateMethod = "isSImm20";
	let RenderMethod = "addSImm20Operands";
	let ParserMethod = "parseSImm20";
	let DiagnosticType = "InvalidSImm20";
}

def SImm20By8AsmOperand : AsmOperandClass {
	let Name = "SImm20By8";
	let PredicateMethod = "isSImm20By8";
	let RenderMethod = "addImmOperands";
	let ParserMethod = "parseSImm20By8";
	let DiagnosticType = "InvalidSImm20By8";
}

def UImm3AsmOperand : AsmOperandClass {
	let Name = "UImm3";
	let PredicateMethod = "isUImm3";
	let RenderMethod = "addImmOperands";
	let ParserMethod = "parseUImm3";
	let DiagnosticType = "InvalidUImm3";
}

def simm20 : Operand<i32>, ImmLeaf<i32, [{ return isInt<20>(Imm); }]> {
	let ParserMatchClass = SImm20AsmOperand;
	let PrintMethod = "printSImm20";
	let EncoderMethod = "getSImm20OpValue";
	let DecoderMethod = "decodeSImm20";
}

def simm20by8 : Operand<i32> {
	let ParserMatchClass = SImm20By8AsmOperand;
	let PrintMethod = "printSImm20";
	let EncoderMethod = "getSImm20By8OpValue";
	let DecoderMethod = "decodeSImm20By8";
}

def uimm3 : Operand<i32>, ImmLeaf<i32, [{ return isUInt<3>(Imm); }]> {
	let ParserMatchClass = UImm3AsmOperand;
	let PrintMethod = "printUImm3";
}

def LongMemRegAsmOperand : AsmOperandClass {
  let Name = "LongMemReg";
  let PredicateMethod = "isLongMemReg";
  let RenderMethod = "addLongMemRegOperands";
}

def TasMemRegAsmOperand : AsmOperandClass {
  let Name = "TasMemReg";
  let PredicateMethod = "isTasMemReg";
  let RenderMethod = "addLongMemRegOperands";
}

def LongMemDispAsmOperand : AsmOperandClass {
  let Name = "LongMemDisp";
  let PredicateMethod = "isLongMemDisp";
  let RenderMethod = "addLongMemDispOperands";
}

def ByteMemDispAsmOperand : AsmOperandClass {
  let Name = "ByteMemDisp";
  let PredicateMethod = "isByteMemDisp";
  let RenderMethod = "addNarrowMemDispOperands";
}

def WordMemDispAsmOperand : AsmOperandClass {
  let Name = "WordMemDisp";
  let PredicateMethod = "isWordMemDisp";
  let RenderMethod = "addNarrowMemDispOperands";
}

def ExtendedByteMemDispAsmOperand : AsmOperandClass {
	let Name = "ExtendedByteMemDisp";
	let PredicateMethod = "isExtendedByteMemDisp";
	let RenderMethod = "addExtendedMemDispOperands";
}

def ExtendedWordMemDispAsmOperand : AsmOperandClass {
	let Name = "ExtendedWordMemDisp";
	let PredicateMethod = "isExtendedWordMemDisp";
	let RenderMethod = "addExtendedMemDispOperands";
}

def ExtendedLongMemDispAsmOperand : AsmOperandClass {
	let Name = "ExtendedLongMemDisp";
	let PredicateMethod = "isExtendedLongMemDisp";
	let RenderMethod = "addExtendedMemDispOperands";
}

def ExtendedDoubleMemDispAsmOperand : AsmOperandClass {
	let Name = "ExtendedDoubleMemDisp";
	let PredicateMethod = "isExtendedDoubleMemDisp";
	let RenderMethod = "addExtendedMemDispOperands";
}

def R0AsmOperand : AsmOperandClass {
	let Name = "R0Operand";
  let PredicateMethod = "isR0";
  let RenderMethod = "addRegOperands";
  let DiagnosticType = "InvalidR0";
}

def PreDecGPRAsmOperand : AsmOperandClass {
  let Name = "PreDecGPR";
  let PredicateMethod = "isPreDecGPR";
  let RenderMethod = "addPreDecGPROperands";
}

def PostIncGPRAsmOperand : AsmOperandClass {
  let Name = "PostIncGPR";
  let PredicateMethod = "isPostIncGPR";
  let RenderMethod = "addPostIncGPROperands";
}

def PreDecR15AsmOperand : AsmOperandClass {
	let Name = "PreDecR15";
	let PredicateMethod = "isPreDecR15";
	let RenderMethod = "addPreDecGPROperands";
}

def PostIncR15AsmOperand : AsmOperandClass {
	let Name = "PostIncR15";
	let PredicateMethod = "isPostIncR15";
	let RenderMethod = "addPostIncGPROperands";
}

def longmemreg : Operand<iPTR> {
  let MIOperandInfo = (ops GPR);
  let ParserMatchClass = LongMemRegAsmOperand;
  let PrintMethod = "printLongMemReg";
  let DecoderMethod = "DecodeGPRRegisterClass";
}

def IndexedMemAsmOperand : AsmOperandClass {
  let Name = "IndexedMem";
  let PredicateMethod = "isIndexedMem";
  let RenderMethod = "addIndexedMemOperands";
  let DiagnosticType = "InvalidIndexedMem";
}

def R0GBRAsmOperand : AsmOperandClass {
	let Name = "R0GBR";
	let PredicateMethod = "isR0GBR";
	let RenderMethod = "addRegOperands";
	let DiagnosticType = "InvalidR0GBR";
}

def GBRByteDispAsmOperand : AsmOperandClass {
	let Name = "GBRByteDisp";
	let PredicateMethod = "isGBRByteDisp";
	let RenderMethod = "addGBRDispOperands";
}

def GBRWordDispAsmOperand : AsmOperandClass {
	let Name = "GBRWordDisp";
	let PredicateMethod = "isGBRWordDisp";
	let RenderMethod = "addGBRDispOperands";
}

def GBRLongDispAsmOperand : AsmOperandClass {
	let Name = "GBRLongDisp";
	let PredicateMethod = "isGBRLongDisp";
	let RenderMethod = "addGBRDispOperands";
}

def indexedmem : Operand<iPTR> {
  let ParserMatchClass = IndexedMemAsmOperand;
  let PrintMethod = "printIndexedMem";
  let DecoderMethod = "DecodeGPRRegisterClass";
}

def r0gbr : RegisterOperand<GBRReg> {
	let ParserMatchClass = R0GBRAsmOperand;
	let PrintMethod = "printR0GBR";
	let DecoderMethod = "decodeGBRRegisterClass";
}

def gbrbytedisp : Operand<iPTR> {
	let MIOperandInfo = (ops i32imm);
	let ParserMatchClass = GBRByteDispAsmOperand;
	let PrintMethod = "printGBRDisp";
	let EncoderMethod = "getGBRDispOpValue";
	let DecoderMethod = "decodeGBRByteDispOperand";
}

def gbrworddisp : Operand<iPTR> {
	let MIOperandInfo = (ops i32imm);
	let ParserMatchClass = GBRWordDispAsmOperand;
	let PrintMethod = "printGBRDisp";
	let EncoderMethod = "getGBRDispOpValue";
	let DecoderMethod = "decodeGBRWordDispOperand";
}

def gbrlongdisp : Operand<iPTR> {
	let MIOperandInfo = (ops i32imm);
	let ParserMatchClass = GBRLongDispAsmOperand;
	let PrintMethod = "printGBRDisp";
	let EncoderMethod = "getGBRDispOpValue";
	let DecoderMethod = "decodeGBRLongDispOperand";
}

def tasmemreg : Operand<iPTR> {
  let MIOperandInfo = (ops GPR);
  let ParserMatchClass = TasMemRegAsmOperand;
  let PrintMethod = "printLongMemReg";
  let DecoderMethod = "DecodeGPRRegisterClass";
}

def longmemdisp : Operand<iPTR> {
  let MIOperandInfo = (ops GPR, i32imm);
  let ParserMatchClass = LongMemDispAsmOperand;
  let PrintMethod = "printLongMemDisp";
  let EncoderMethod = "getLongDispMemOpValue";
  let DecoderMethod = "decodeLongDispMemOperand";
}

def bytememdisp : Operand<iPTR> {
  let MIOperandInfo = (ops GPR, i32imm);
  let ParserMatchClass = ByteMemDispAsmOperand;
  let PrintMethod = "printNarrowMemDisp";
  let EncoderMethod = "getNarrowDispMemOpValue";
  let DecoderMethod = "decodeByteDispMemOperand";
}

def wordmemdisp : Operand<iPTR> {
  let MIOperandInfo = (ops GPR, i32imm);
  let ParserMatchClass = WordMemDispAsmOperand;
  let PrintMethod = "printNarrowMemDisp";
  let EncoderMethod = "getNarrowDispMemOpValue";
  let DecoderMethod = "decodeWordDispMemOperand";
}

def extbytememdisp : Operand<iPTR> {
	let MIOperandInfo = (ops GPR, i32imm);
	let ParserMatchClass = ExtendedByteMemDispAsmOperand;
	let PrintMethod = "printExtendedMemDisp";
	let EncoderMethod = "getExtendedMemDispOpValue";
	let DecoderMethod = "decodeExtendedByteMemDispOperand";
}

def extwordmemdisp : Operand<iPTR> {
	let MIOperandInfo = (ops GPR, i32imm);
	let ParserMatchClass = ExtendedWordMemDispAsmOperand;
	let PrintMethod = "printExtendedMemDisp";
	let EncoderMethod = "getExtendedMemDispOpValue";
	let DecoderMethod = "decodeExtendedWordMemDispOperand";
}

def extlongmemdisp : Operand<iPTR> {
	let MIOperandInfo = (ops GPR, i32imm);
	let ParserMatchClass = ExtendedLongMemDispAsmOperand;
	let PrintMethod = "printExtendedMemDisp";
	let EncoderMethod = "getExtendedMemDispOpValue";
	let DecoderMethod = "decodeExtendedLongMemDispOperand";
}

def extdoublememdisp : Operand<iPTR> {
	let MIOperandInfo = (ops GPR, i32imm);
	let ParserMatchClass = ExtendedDoubleMemDispAsmOperand;
	let PrintMethod = "printExtendedMemDisp";
	let EncoderMethod = "getExtendedMemDispOpValue";
	let DecoderMethod = "decodeExtendedDoubleMemDispOperand";
}

def r0reg : RegisterOperand<R0Reg> {
  let ParserMatchClass = R0AsmOperand;
  let DecoderMethod = "decodeR0RegisterClass";
}

def predecgpr : Operand<iPTR> {
  let MIOperandInfo = (ops GPR);
  let ParserMatchClass = PreDecGPRAsmOperand;
  let PrintMethod = "printPreDecGPR";
  let DecoderMethod = "DecodeGPRRegisterClass";
}

def postincgpr : Operand<iPTR> {
  let MIOperandInfo = (ops GPR);
  let ParserMatchClass = PostIncGPRAsmOperand;
  let PrintMethod = "printPostIncGPR";
  let DecoderMethod = "DecodeGPRRegisterClass";
}

def predecr15 : Operand<iPTR> {
	let MIOperandInfo = (ops GPR);
	let ParserMatchClass = PreDecR15AsmOperand;
	let PrintMethod = "printPreDecGPR";
}

def postincr15 : Operand<iPTR> {
	let MIOperandInfo = (ops GPR);
	let ParserMatchClass = PostIncR15AsmOperand;
	let PrintMethod = "printPostIncGPR";
}

def bankedgpr : RegisterOperand<BankedGPR> {
	let DecoderMethod = "DecodeBankedGPRRegisterClass";
}

def addr_reg : ComplexPattern<iPTR, 1, "SelectAddrReg">;
def addr_disp : ComplexPattern<iPTR, 2, "SelectAddrDisp", [add, frameindex]>;
def addr_fpu_reg : ComplexPattern<iPTR, 1, "SelectFPUAddrReg">;
def addr_fpu_frame : ComplexPattern<iPTR, 2, "SelectFPUFrameAddr", [add, frameindex]>;
def addr_narrow_reg : ComplexPattern<iPTR, 1, "SelectNarrowAddrReg">;
def addr_byte_frame
    : ComplexPattern<iPTR, 2, "SelectByteFrameAddr", [add, frameindex]>;
def addr_word_frame
    : ComplexPattern<iPTR, 2, "SelectWordFrameAddr", [add, frameindex]>;

def SDT_SHCmp : SDTypeProfile<0, 2, [SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;
def SDT_SHUnaryCmp : SDTypeProfile<0, 1, [SDTCisVT<0, i32>]>;
def SDT_SHBrcond : SDTypeProfile<0, 1, [SDTCisVT<0, OtherVT>]>;
def SDT_SHCall : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;
def SDT_SHDivRem
    : SDTypeProfile<2, 2, [SDTCisVT<0, i32>, SDTCisSameAs<0, 1>,
                           SDTCisSameAs<0, 2>, SDTCisSameAs<0, 3>]>;
def SDT_SHMulLoHi
	: SDTypeProfile<2, 2, [SDTCisVT<0, i32>, SDTCisSameAs<0, 1>,
							SDTCisSameAs<0, 2>, SDTCisSameAs<0, 3>]>;
def SDT_SHShiftParts
    : SDTypeProfile<2, 3, [SDTCisVT<0, i32>, SDTCisSameAs<0, 1>,
                           SDTCisSameAs<0, 2>, SDTCisSameAs<0, 3>,
                           SDTCisSameAs<0, 4>]>;
def SDT_SHMovT : SDTypeProfile<1, 0, [SDTCisVT<0, i32>]>;
def SDT_SHUnaryI32
	: SDTypeProfile<1, 1, [SDTCisVT<0, i32>, SDTCisSameAs<0, 1>]>;
def SDT_SHBinaryI32
	: SDTypeProfile<1, 2, [SDTCisVT<0, i32>, SDTCisSameAs<0, 1>,
							 SDTCisSameAs<0, 2>]>;
def SDT_SHSelect
	: SDTypeProfile<1, 3, [SDTCisVT<0, i32>, SDTCisSameAs<0, 1>,
						   SDTCisSameAs<0, 2>, SDTCisSameAs<0, 3>]>;
def SDT_SHSelectCC
	: SDTypeProfile<1, 5, [SDTCisVT<0, i32>, SDTCisSameAs<0, 1>,
						   SDTCisSameAs<0, 2>, SDTCisSameAs<0, 3>,
						   SDTCisSameAs<0, 4>, SDTCisSameAs<0, 5>]>;
def SDT_SHSelectF32
	: SDTypeProfile<1, 3, [SDTCisVT<0, f32>, SDTCisVT<1, i32>,
						   SDTCisSameAs<0, 2>, SDTCisSameAs<0, 3>]>;
def SDT_SHSelectF64
	: SDTypeProfile<1, 3, [SDTCisVT<0, f64>, SDTCisVT<1, i32>,
						   SDTCisSameAs<0, 2>, SDTCisSameAs<0, 3>]>;
def SDT_SHCallSeqStart
    : SDCallSeqStart<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;
def SDT_SHCallSeqEnd
    : SDCallSeqEnd<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;
def SDT_SHBrJT
    : SDTypeProfile<0, 3, [SDTCisVT<0, i32>, SDTCisVT<1, i32>,
                           SDTCisVT<2, i32>]>;
def SDT_SHPICAddress : SDTypeProfile<1, 1, [SDTCisVT<0, i32>,
                                            SDTCisVT<1, i32>]>;
def SDT_SHEHReturn : SDTypeProfile<0, 1, [SDTCisPtrTy<0>]>;
def SDT_SHTrapExit : SDTypeProfile<0, 1, [SDTCisVT<0, i32>]>;
def SDT_SHGPRToFR : SDTypeProfile<1, 1, [SDTCisVT<0, f32>, SDTCisVT<1, i32>]>;
def SDT_SHFRToGPR : SDTypeProfile<1, 1, [SDTCisVT<0, i32>, SDTCisVT<1, f32>]>;
def SDT_SHGPRPairToDR : SDTypeProfile<1, 2, [SDTCisVT<0, f64>, SDTCisVT<1, i32>, SDTCisSameAs<1, 2>]>;
def SDT_SHDRToGPRPair : SDTypeProfile<2, 1, [SDTCisVT<0, i32>, SDTCisSameAs<0, 1>, SDTCisVT<2, f64>]>;
def SDT_SHI32ToFP : SDTypeProfile<1, 1, [SDTCisFP<0>, SDTCisVT<1, i32>]>;
def SDT_SHFPToI32 : SDTypeProfile<1, 1, [SDTCisVT<0, i32>, SDTCisFP<1>]>;
def SDT_SHF32ToF64 : SDTypeProfile<1, 1, [SDTCisVT<0, f64>, SDTCisVT<1, f32>]>;
def SDT_SHF64ToF32 : SDTypeProfile<1, 1, [SDTCisVT<0, f32>, SDTCisVT<1, f64>]>;
def SDT_SHFCmp32 : SDTypeProfile<0, 2, [SDTCisVT<0, f32>, SDTCisSameAs<0, 1>]>;
def SDT_SHFCmp64 : SDTypeProfile<0, 2, [SDTCisVT<0, f64>, SDTCisSameAs<0, 1>]>;
def SDT_SHF32Constant : SDTypeProfile<1, 0, [SDTCisVT<0, f32>]>;

def SHret : SDNode<"SHISD::RET_GLUE", SDTNone,
                   [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def SHrte : SDNode<"SHISD::RTE_GLUE", SDTNone,
	[SDNPHasChain, SDNPOptInGlue]>;
def SHtrap_exit : SDNode<"SHISD::TRAP_EXIT", SDT_SHTrapExit,
	[SDNPHasChain, SDNPOptInGlue]>;
def SHehret : SDNode<"SHISD::EH_RETURN", SDT_SHEHReturn,
	[SDNPHasChain, SDNPOptInGlue]>;
def SHcall : SDNode<"SHISD::CALL", SDT_SHCall,
                    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def SHcall_pcrel : SDNode<"SHISD::CALL_PCREL", SDT_SHCall,
	[SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def SHtailcall : SDNode<"SHISD::TAIL_CALL", SDT_SHCall,
	[SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def SHcallseq_start : SDNode<"ISD::CALLSEQ_START", SDT_SHCallSeqStart,
                            [SDNPHasChain, SDNPOutGlue]>;
def SHcallseq_end : SDNode<"ISD::CALLSEQ_END", SDT_SHCallSeqEnd,
                          [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;
def SHsynco : SDNode<"SHISD::SYNCO", SDTNone, [SDNPHasChain]>;
def SHcmpeq : SDNode<"SHISD::CMP_EQ", SDT_SHCmp, [SDNPOutGlue]>;
def SHcmphs : SDNode<"SHISD::CMP_HS", SDT_SHCmp, [SDNPOutGlue]>;
def SHcmpge : SDNode<"SHISD::CMP_GE", SDT_SHCmp, [SDNPOutGlue]>;
def SHcmphi : SDNode<"SHISD::CMP_HI", SDT_SHCmp, [SDNPOutGlue]>;
def SHcmpgt : SDNode<"SHISD::CMP_GT", SDT_SHCmp, [SDNPOutGlue]>;
def SHcmppl : SDNode<"SHISD::CMP_PL", SDT_SHUnaryCmp, [SDNPOutGlue]>;
def SHcmppz : SDNode<"SHISD::CMP_PZ", SDT_SHUnaryCmp, [SDNPOutGlue]>;
def SHtst : SDNode<"SHISD::TST", SDT_SHCmp, [SDNPOutGlue]>;
def SHbt : SDNode<"SHISD::BT", SDT_SHBrcond, [SDNPHasChain, SDNPInGlue]>;
def SHbf : SDNode<"SHISD::BF", SDT_SHBrcond, [SDNPHasChain, SDNPInGlue]>;
def SHudiv : SDNode<"SHISD::UDIV", SDTIntBinOp>;
// Unsigned i32 quotient with a proven nonzero divisor that fits in 16 bits.
def SHudivnarrow : SDNode<"SHISD::UDIV_NARROW", SDTIntBinOp>;
def SHurem : SDNode<"SHISD::UREM", SDTIntBinOp>;
def SHudivrem : SDNode<"SHISD::UDIVREM", SDT_SHDivRem>;
def SHsdiv : SDNode<"SHISD::SDIV", SDTIntBinOp>;
def SHsrem : SDNode<"SHISD::SREM", SDTIntBinOp>;
def SHsdivrem : SDNode<"SHISD::SDIVREM", SDT_SHDivRem>;
def SHmulsw : SDNode<"SHISD::MULS_WORD", SDTIntBinOp>;
def SHmuluw : SDNode<"SHISD::MULU_WORD", SDTIntBinOp>;
def SHsmullohi : SDNode<"SHISD::SMUL_LOHI", SDT_SHMulLoHi>;
def SHumullohi : SDNode<"SHISD::UMUL_LOHI", SDT_SHMulLoHi>;
def SHmulhs : SDNode<"SHISD::MUL_HIGH_SIGNED", SDTIntBinOp>;
def SHmulhu : SDNode<"SHISD::MUL_HIGH_UNSIGNED", SDTIntBinOp>;
def SHshlparts : SDNode<"SHISD::SHL_PARTS", SDT_SHShiftParts>;
def SHsrlparts : SDNode<"SHISD::SRL_PARTS", SDT_SHShiftParts>;
def SHsraparts : SDNode<"SHISD::SRA_PARTS", SDT_SHShiftParts>;
def SHshl : SDNode<"SHISD::SHL", SDT_SHBinaryI32>;
def SDT_SHSmallShift : SDTypeProfile<1, 4, [SDTCisVT<0, i32>, SDTCisSameAs<0, 1>, SDTCisSameAs<0, 2>, SDTCisSameAs<0, 3>, SDTCisSameAs<0, 4>]>;
def SHshlsmall : SDNode<"SHISD::SHL_SMALL", SDT_SHSmallShift>;
def SHsrlsmall : SDNode<"SHISD::SRL_SMALL", SDT_SHSmallShift>;
def SHmovt : SDNode<"SHISD::MOVT", SDT_SHMovT, [SDNPInGlue]>;
def SHrotl1 : SDNode<"SHISD::ROTL1", SDT_SHUnaryI32>;
def SHrotr1 : SDNode<"SHISD::ROTR1", SDT_SHUnaryI32>;
def SHrotl16 : SDNode<"SHISD::ROTL16", SDT_SHUnaryI32>;
def SHrotl : SDNode<"SHISD::ROTL", SDT_SHBinaryI32>;
def SHrotr : SDNode<"SHISD::ROTR", SDT_SHBinaryI32>;
def SHrotlshort : SDNode<"SHISD::ROTL_SHORT", SDT_SHBinaryI32>;
def SHrotrshort : SDNode<"SHISD::ROTR_SHORT", SDT_SHBinaryI32>;
def SHaddv : SDNode<"SHISD::ADDV", SDT_SHBinaryI32, [SDNPOutGlue]>;
def SHsubv : SDNode<"SHISD::SUBV", SDT_SHBinaryI32, [SDNPOutGlue]>;
def SHboolsext : SDNode<"SHISD::BOOL_SEXT", SDT_SHUnaryI32>;
def SHselect : SDNode<"SHISD::SELECT", SDT_SHSelect>;
def SHselectcc : SDNode<"SHISD::SELECT_CC", SDT_SHSelectCC>;
def SHselectf32 : SDNode<"SHISD::SELECT_F32", SDT_SHSelectF32>;
def SHselectf64 : SDNode<"SHISD::SELECT_F64", SDT_SHSelectF64>;
def SHbrjt
    : SDNode<"SHISD::BR_JT", SDT_SHBrJT,
             [SDNPHasChain, SDNPMayLoad, SDNPMemOperand]>;
def SHpicaddr
    : SDNode<"SHISD::PIC_ADDRESS", SDT_SHPICAddress,
             [SDNPHasChain, SDNPMayLoad, SDNPMemOperand]>;
def SHpicgotoff
    : SDNode<"SHISD::PIC_GOTOFF", SDT_SHPICAddress,
             [SDNPHasChain, SDNPMayLoad, SDNPMemOperand]>;
def SHthreadptr : SDNode<"SHISD::THREAD_POINTER",
                         SDTypeProfile<1, 0, [SDTCisVT<0, i32>]>>;
def SHgprtofr : SDNode<"SHISD::GPR_TO_FR", SDT_SHGPRToFR>;
def SHfrtogpr : SDNode<"SHISD::FR_TO_GPR", SDT_SHFRToGPR>;
def SHgprpairtodr : SDNode<"SHISD::GPR_PAIR_TO_DR", SDT_SHGPRPairToDR>;
def SHdrtogprpair : SDNode<"SHISD::DR_TO_GPR_PAIR", SDT_SHDRToGPRPair>;
def SHi32tof32 : SDNode<"SHISD::I32_TO_F32", SDT_SHI32ToFP>;
def SHi32tof64 : SDNode<"SHISD::I32_TO_F64", SDT_SHI32ToFP>;
def SHf32toi32 : SDNode<"SHISD::F32_TO_I32", SDT_SHFPToI32>;
def SHf64toi32 : SDNode<"SHISD::F64_TO_I32", SDT_SHFPToI32>;
def SDT_SHFPToU32 : SDTypeProfile<1, 2, [SDTCisVT<0, i32>, SDTCisFP<1>, SDTCisSameAs<1, 2>]>;
def SHf32tou32 : SDNode<"SHISD::F32_TO_U32", SDT_SHFPToU32>;
def SHf64tou32 : SDNode<"SHISD::F64_TO_U32", SDT_SHFPToU32>;
def SHf32tof64 : SDNode<"SHISD::F32_TO_F64", SDT_SHF32ToF64>;
def SHf64tof32 : SDNode<"SHISD::F64_TO_F32", SDT_SHF64ToF32>;
def SHfconst0 : SDNode<"SHISD::FCONST_0", SDT_SHF32Constant>;
def SHfconst1 : SDNode<"SHISD::FCONST_1", SDT_SHF32Constant>;
def SHfcmpeq32 : SDNode<"SHISD::FCMP_EQ_F32", SDT_SHFCmp32, [SDNPOutGlue]>;
def SHfcmpgt32 : SDNode<"SHISD::FCMP_GT_F32", SDT_SHFCmp32, [SDNPOutGlue]>;
def SHfcmpeq64 : SDNode<"SHISD::FCMP_EQ_F64", SDT_SHFCmp64, [SDNPOutGlue]>;
def SHfcmpgt64 : SDNode<"SHISD::FCMP_GT_F64", SDT_SHFCmp64, [SDNPOutGlue]>;
def SHtlscall
    : SDNode<"SHISD::TLS_CALL", SDT_SHCall,
             [SDNPHasChain, SDNPOutGlue, SDNPVariadic, SDNPMayLoad,
              SDNPMemOperand]>;
def SHtlsie
    : SDNode<"SHISD::TLS_IE", SDT_SHPICAddress,
             [SDNPHasChain, SDNPMayLoad, SDNPMemOperand]>;

def non_simm8 : ImmLeaf<i32, [{ return !isInt<8>(Imm); }]>;
def u5imm : ImmLeaf<i32, [{ return isUInt<5>(Imm); }]>;

// System and control-state instructions.
def NOP : SHFixed<0x0009, "nop">;

let Defs = [SBit], Predicates = [HasSBitControl] in
def CLRS : SHFixed<0x0048, "clrs">;

let Defs = [SBit], Predicates = [HasSBitControl] in
def SETS : SHFixed<0x0058, "sets">;

let hasSideEffects = 1, Predicates = [HasMMU] in
def LDTLB : SHFixed<0x0038, "ldtlb">;

let Defs = [TBit] in
def CLRT : SHFixed<0x0008, "clrt">;

let Defs = [TBit] in
def SETT : SHFixed<0x0018, "sett">;

let Defs = [MACH, MACL] in
def CLRMAC : SHFixed<0x0028, "clrmac">;

let hasSideEffects = 1 in
def SLEEP : SHFixed<0x001b, "sleep">;

let isReturn = 1, isTerminator = 1, isBarrier = 1, hasDelaySlot = 1,
		mayLoad = 1, Uses = [R15], Defs = [R15, SR, SBit, MBit, QBit, TBit] in
def RTE : SHFixed<0x002b, "rte">;

let isCall = 1, hasSideEffects = 1, mayLoad = 1, mayStore = 1,
		Uses = [R15, SR, SBit, MBit, QBit, TBit, VBR], Defs = [R15] in
def TRAPA : SHR0Imm<0xc3, (outs), (ins uimm8:$imm), "trapa\t$imm">;

let Defs = [TBit], mayLoad = 1, mayStore = 1 in
def TAS_B
    : SHOneReg<0x1b, (outs), (ins tasmemreg:$dst), "tas.b\t$dst">;

let Uses = [TBit] in
def MOVT
    : SHSpecialToReg<0x29, (outs GPR:$dst), (ins), "movt\t$dst"> {
	let Pattern = [(set GPR:$dst, (SHmovt))];
}

// Code-generation pseudos and returns.
let isCodeGenOnly = 1, Uses = [R15], Defs = [R15] in {
  def ADJCALLSTACKDOWN
      : SHPseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2)> {
    let Pattern = [(SHcallseq_start timm:$amt1, timm:$amt2)];
  }
  def ADJCALLSTACKUP
      : SHPseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2)> {
    let Pattern = [(SHcallseq_end timm:$amt1, timm:$amt2)];
  }
}

let isReturn = 1, isTerminator = 1, isBarrier = 1, hasDelaySlot = 1,
    Uses = [PR] in
def RTS : SHFixed<0x000b, "rts"> {
  let Pattern = [(SHret)];
}

let isReturn = 1, isTerminator = 1, isBarrier = 1, hasDelaySlot = 1,
	mayLoad = 1, Uses = [R15], Defs = [R15, SR, SBit, MBit, QBit, TBit] in
def RTE_RETURN : SHPseudo<(outs), (ins)> {
	let Pattern = [(SHrte)];
}

let isReturn = 1, isTerminator = 1, isBarrier = 1,
	hasSideEffects = 1,
	Uses = [R15, SR, SBit, MBit, QBit, TBit, VBR],
	Defs = [R15] in
def TRAP_EXIT_RETURN : SHR0Imm<0xc3, (outs), (ins uimm8:$imm),
	"trapa\t$imm"> {
	let isCodeGenOnly = 1;
	let Pattern = [(SHtrap_exit uimm8:$imm)];
}

let isReturn = 1, isTerminator = 1, isBarrier = 1, Uses = [R3] in
def EH_RETURN : SHPseudo<(outs), (ins GPR:$handler)> {
	let Pattern = [(SHehret GPR:$handler)];
}

// Register transfer, arithmetic, and logical instructions.
let hasExtraSrcRegAllocReq = 0, hasExtraDefRegAllocReq = 0 in
def MOVrr : SHRegReg<0x6, 0x3, (outs GPR:$dst), (ins GPR:$src),
                     "mov\t$src,$dst">;

def SWAP_B : SHRegReg<0x6, 0x8, (outs GPR:$dst), (ins GPR:$src),
											"swap.b\t$src,$dst">;
def SWAP_W : SHRegReg<0x6, 0x9, (outs GPR:$dst), (ins GPR:$src),
											"swap.w\t$src,$dst",
											[(set GPR:$dst, (SHrotl16 GPR:$src))]>;

let Constraints = "$dst = $dst_in" in
def XTRCT : SHRegReg<0x2, 0xd, (outs GPR:$dst),
										 (ins GPR:$dst_in, GPR:$src), "xtrct\t$src,$dst">;

let Defs = [PR] in
def LDS_PR : SHRegToSpecial<0x2a, (outs), (ins GPR:$src), "lds\t$src,pr">;

let Defs = [MACH] in
def LDS_MACH : SHRegToSpecial<0x0a, (outs), (ins GPR:$src), "lds\t$src,mach">;

let Defs = [MACL] in
def LDS_MACL : SHRegToSpecial<0x1a, (outs), (ins GPR:$src), "lds\t$src,macl">;

let Defs = [SR, SBit, MBit, QBit, TBit] in
def LDC_SR : SHRegToSpecial<0x0e, (outs), (ins GPR:$src), "ldc\t$src,sr">;

let Defs = [GBR], hasSideEffects = 1 in
def LDC_GBR : SHRegToSpecial<0x1e, (outs), (ins GPR:$src), "ldc\t$src,gbr">;

let Defs = [VBR] in
def LDC_VBR : SHRegToSpecial<0x2e, (outs), (ins GPR:$src), "ldc\t$src,vbr">;

let Defs = [SSR], Predicates = [HasExceptionState] in
def LDC_SSR : SHRegToSpecial<0x3e, (outs), (ins GPR:$src), "ldc\t$src,ssr">;

let Defs = [SPC], Predicates = [HasExceptionState] in
def LDC_SPC : SHRegToSpecial<0x4e, (outs), (ins GPR:$src), "ldc\t$src,spc">;

let Defs = [DBR], Predicates = [HasSH4DebugState] in
def LDC_DBR : SHRegToSpecial<0xfa, (outs), (ins GPR:$src), "ldc\t$src,dbr">;

let Predicates = [HasBankedGPRs] in
def LDC_BANK : SHBankLoad<(outs bankedgpr:$bank_dst), (ins GPR:$src),
	"ldc\t$src,$bank_dst">;

let Constraints = "$dst = $dst_in", isCommutable = 1 in
def ADDrr : SHRegReg<0x3, 0xc, (outs GPR:$dst),
                     (ins GPR:$dst_in, GPR:$src), "add\t$src,$dst",
                     [(set GPR:$dst, (add GPR:$dst_in, GPR:$src))]>;

let Constraints = "$dst = $dst_in" in
def SUBrr : SHRegReg<0x3, 0x8, (outs GPR:$dst),
                     (ins GPR:$dst_in, GPR:$src), "sub\t$src,$dst",
                     [(set GPR:$dst, (sub GPR:$dst_in, GPR:$src))]>;

def NEG : SHRegReg<0x6, 0xb, (outs GPR:$dst), (ins GPR:$src),
                   "neg\t$src,$dst",
                   [(set GPR:$dst, (ineg GPR:$src))]>;

def : Pat<(SHboolsext GPR:$src), (NEG GPR:$src)>;

let Constraints = "$dst = $dst_in", isCommutable = 1 in {
  def ANDrr : SHRegReg<0x2, 0x9, (outs GPR:$dst),
                       (ins GPR:$dst_in, GPR:$src), "and\t$src,$dst",
                       [(set GPR:$dst, (and GPR:$dst_in, GPR:$src))]>;
  def ORrr : SHRegReg<0x2, 0xb, (outs GPR:$dst),
                      (ins GPR:$dst_in, GPR:$src), "or\t$src,$dst",
                      [(set GPR:$dst, (or GPR:$dst_in, GPR:$src))]>;
  def XORrr : SHRegReg<0x2, 0xa, (outs GPR:$dst),
                       (ins GPR:$dst_in, GPR:$src), "xor\t$src,$dst",
                       [(set GPR:$dst, (xor GPR:$dst_in, GPR:$src))]>;
}

def NOT : SHRegReg<0x6, 0x7, (outs GPR:$dst), (ins GPR:$src),
                   "not\t$src,$dst",
                   [(set GPR:$dst, (not GPR:$src))]>;

let Defs = [TBit], isCompare = 1 in
def TST : SHRegReg<0x2, 0x8, (outs), (ins GPR:$src, GPR:$dst),
                   "tst\t$src,$dst", [(SHtst GPR:$dst, GPR:$src)]>;

let Defs = [TBit], isCompare = 1, Uses = [R0] in {
	def CMP_EQri : SHR0Imm<0x88, (outs), (ins simm8:$imm),
													"cmp/eq\t$imm,r0">;
	def TSTri : SHR0Imm<0xc8, (outs), (ins uimm8:$imm),
											"tst\t$imm,r0">;
}

let Defs = [TBit, R0], isCompare = 1 in
def CMP_EQri_VALUE_PSEUDO : SHPseudo<(outs), (ins GPR:$src, simm8:$imm)>;

let Defs = [TBit, R0], isCompare = 1 in
def TSTri_PSEUDO : SHPseudo<(outs), (ins GPR:$src, uimm8:$imm)>;

let Defs = [TBit], isCompare = 1 in {
	def CMP_PL : SHOneReg<0x15, (outs), (ins GPR:$dst), "cmp/pl\t$dst",
		[(SHcmppl GPR:$dst)]>;
	def CMP_PZ : SHOneReg<0x11, (outs), (ins GPR:$dst), "cmp/pz\t$dst",
		[(SHcmppz GPR:$dst)]>;
	def CMP_STR : SHRegReg<0x2, 0xc, (outs), (ins GPR:$src, GPR:$dst),
												 "cmp/str\t$src,$dst">;
}

let Constraints = "$dst = $dst_in", Defs = [TBit] in {
	let Predicates = [HasSH2] in
  def DT : SHOneReg<0x10, (outs GPR:$dst), (ins GPR:$dst_in), "dt\t$dst">;
  def SHLL
      : SHOneReg<0x00, (outs GPR:$dst), (ins GPR:$dst_in), "shll\t$dst">;
  def SHLR
      : SHOneReg<0x01, (outs GPR:$dst), (ins GPR:$dst_in), "shlr\t$dst">;
  def SHAR
      : SHOneReg<0x21, (outs GPR:$dst), (ins GPR:$dst_in), "shar\t$dst">;
	def SHAL
			: SHOneReg<0x20, (outs GPR:$dst), (ins GPR:$dst_in), "shal\t$dst">;
}

let Constraints = "$dst = $dst_in", Predicates = [HasDynamicShift] in {
	def SHAD : SHRegReg<0x4, 0xc, (outs GPR:$dst),
		(ins GPR:$dst_in, GPR:$src), "shad\t$src,$dst">;
	def SHLD : SHRegReg<0x4, 0xd, (outs GPR:$dst),
		(ins GPR:$dst_in, GPR:$src), "shld\t$src,$dst">;
}

let Constraints = "$dst = $dst_in" in {
  def SHLL2
      : SHOneReg<0x08, (outs GPR:$dst), (ins GPR:$dst_in), "shll2\t$dst">;
  def SHLR2
      : SHOneReg<0x09, (outs GPR:$dst), (ins GPR:$dst_in), "shlr2\t$dst">;
  def SHLL8
      : SHOneReg<0x18, (outs GPR:$dst), (ins GPR:$dst_in), "shll8\t$dst">;
  def SHLR8
      : SHOneReg<0x19, (outs GPR:$dst), (ins GPR:$dst_in), "shlr8\t$dst">;
  def SHLL16
      : SHOneReg<0x28, (outs GPR:$dst), (ins GPR:$dst_in), "shll16\t$dst">;
  def SHLR16
      : SHOneReg<0x29, (outs GPR:$dst), (ins GPR:$dst_in), "shlr16\t$dst">;
}

let isReMaterializable = 1, isAsCheapAsAMove = 1 in
def MOVri : SHRegImm<0xe, (outs GPR:$dst), (ins simm8:$imm),
                     "mov\t$imm,$dst", [(set GPR:$dst, simm8:$imm)]>;

def LEA_FI : SHPseudo<(outs GPR:$dst), (ins i32imm:$fi, i32imm:$disp)>;

let Constraints = "$dst = $dst_in" in
def ADDri : SHRegImm<0x7, (outs GPR:$dst),
                     (ins GPR:$dst_in, simm8:$imm), "add\t$imm,$dst",
                     [(set GPR:$dst, (add GPR:$dst_in, simm8:$imm))]>;

let Uses = [R0], Defs = [R0] in {
	def ANDri : SHR0Imm<0xc9, (outs), (ins uimm8:$imm), "and\t$imm,r0">;
	def ORri : SHR0Imm<0xcb, (outs), (ins uimm8:$imm), "or\t$imm,r0">;
	def XORri : SHR0Imm<0xca, (outs), (ins uimm8:$imm), "xor\t$imm,r0">;
}

let usesCustomInserter = 1 in {
  def ADDC_LO_PSEUDO
      : SHPseudo<(outs GPR:$dst), (ins GPR:$lhs, GPR:$rhs)> {
    let Pattern = [(set GPR:$dst, (addc GPR:$lhs, GPR:$rhs))];
    let Defs = [TBit];
  }
  def SUBC_LO_PSEUDO
      : SHPseudo<(outs GPR:$dst), (ins GPR:$lhs, GPR:$rhs)> {
    let Pattern = [(set GPR:$dst, (subc GPR:$lhs, GPR:$rhs))];
    let Defs = [TBit];
  }
	def ROTLrr
		: SHPseudo<(outs GPR:$dst), (ins GPR:$value, GPR:$count)> {
		let Pattern = [(set GPR:$dst, (SHrotl GPR:$value, GPR:$count))];
	}
	def ROTRrr
		: SHPseudo<(outs GPR:$dst), (ins GPR:$value, GPR:$count)> {
		let Pattern = [(set GPR:$dst, (SHrotr GPR:$value, GPR:$count))];
	}
	def ROTL_SHORT_PSEUDO
		: SHPseudo<(outs GPR:$dst), (ins GPR:$value, GPR:$count)> {
		let Pattern = [(set GPR:$dst, (SHrotlshort GPR:$value, GPR:$count))];
	}
	def ROTR_SHORT_PSEUDO
		: SHPseudo<(outs GPR:$dst), (ins GPR:$value, GPR:$count)> {
		let Pattern = [(set GPR:$dst, (SHrotrshort GPR:$value, GPR:$count))];
	}
	let isCommutable = 1, Predicates = [HasSH2] in
  def MUL32_PSEUDO
      : SHPseudo<(outs GPR:$dst), (ins GPR:$lhs, GPR:$rhs)> {
    let Pattern = [(set GPR:$dst, (mul GPR:$lhs, GPR:$rhs))];
  }
	let isCommutable = 1 in {
		def MULS_WORD_PSEUDO
			: SHPseudo<(outs GPR:$dst), (ins GPR:$lhs, GPR:$rhs)> {
			let Pattern = [(set GPR:$dst, (SHmulsw GPR:$lhs, GPR:$rhs))];
		}
		def MULU_WORD_PSEUDO
			: SHPseudo<(outs GPR:$dst), (ins GPR:$lhs, GPR:$rhs)> {
			let Pattern = [(set GPR:$dst, (SHmuluw GPR:$lhs, GPR:$rhs))];
		}
		def DMULS_LOHI_PSEUDO
			: SHPseudo<(outs GPR:$low, GPR:$high),
						(ins GPR:$lhs, GPR:$rhs)> {
			let Pattern = [(set GPR:$low, GPR:$high,
				(SHsmullohi GPR:$lhs, GPR:$rhs))];
			let Predicates = [HasSH2];
		}
		def DMULU_LOHI_PSEUDO
			: SHPseudo<(outs GPR:$low, GPR:$high),
						(ins GPR:$lhs, GPR:$rhs)> {
			let Pattern = [(set GPR:$low, GPR:$high,
				(SHumullohi GPR:$lhs, GPR:$rhs))];
			let Predicates = [HasSH2];
		}
		def DMULS_HIGH_PSEUDO
			: SHPseudo<(outs GPR:$high), (ins GPR:$lhs, GPR:$rhs)> {
			let Pattern = [(set GPR:$high, (SHmulhs GPR:$lhs, GPR:$rhs))];
			let Predicates = [HasSH2];
		}
		def DMULU_HIGH_PSEUDO
			: SHPseudo<(outs GPR:$high), (ins GPR:$lhs, GPR:$rhs)> {
			let Pattern = [(set GPR:$high, (SHmulhu GPR:$lhs, GPR:$rhs))];
			let Predicates = [HasSH2];
		}
	}
	def UDIV_NARROW_PSEUDO : SHPseudo<(outs GPR:$quotient), (ins GPR:$dividend, GPR:$divisor)> {
		let Pattern = [(set GPR:$quotient, (SHudivnarrow GPR:$dividend, GPR:$divisor))];
		let Defs = [MBit, QBit, TBit];
	}
  def UDIV32_PSEUDO
      : SHPseudo<(outs GPR:$quotient),
                 (ins GPR:$dividend, GPR:$divisor)> {
    let Pattern =
        [(set GPR:$quotient, (SHudiv GPR:$dividend, GPR:$divisor))];
  }
  def UREM32_PSEUDO
      : SHPseudo<(outs GPR:$remainder),
                 (ins GPR:$dividend, GPR:$divisor)> {
    let Pattern =
        [(set GPR:$remainder, (SHurem GPR:$dividend, GPR:$divisor))];
  }
  def UDIVREM32_PSEUDO
      : SHPseudo<(outs GPR:$quotient, GPR:$remainder),
                 (ins GPR:$dividend, GPR:$divisor)> {
    let Pattern =
        [(set GPR:$quotient, GPR:$remainder,
              (SHudivrem GPR:$dividend, GPR:$divisor))];
  }
  def SDIV32_PSEUDO
      : SHPseudo<(outs GPR:$quotient),
                 (ins GPR:$dividend, GPR:$divisor)> {
    let Pattern =
        [(set GPR:$quotient, (SHsdiv GPR:$dividend, GPR:$divisor))];
  }
  def SREM32_PSEUDO
      : SHPseudo<(outs GPR:$remainder),
                 (ins GPR:$dividend, GPR:$divisor)> {
    let Pattern =
        [(set GPR:$remainder, (SHsrem GPR:$dividend, GPR:$divisor))];
  }
  def SDIVREM32_PSEUDO
      : SHPseudo<(outs GPR:$quotient, GPR:$remainder),
                 (ins GPR:$dividend, GPR:$divisor)> {
    let Pattern =
        [(set GPR:$quotient, GPR:$remainder,
              (SHsdivrem GPR:$dividend, GPR:$divisor))];
  }
  def MOVi32 : SHPseudo<(outs GPR:$dst), (ins i32imm:$imm)> {
    let Pattern = [(set GPR:$dst, non_simm8:$imm)];
  }
  def SHLri : SHPseudo<(outs GPR:$dst), (ins GPR:$src, i32imm:$amount)> {
    let Pattern = [(set GPR:$dst, (shl GPR:$src, u5imm:$amount))];
  }
	let Defs = [TBit] in {
		def SHL_SMALL_PSEUDO : SHPseudo<(outs GPR:$dst), (ins GPR:$src, GPR:$count, i32imm:$base, i32imm:$mask)> {
			let Pattern = [(set GPR:$dst, (SHshlsmall GPR:$src, GPR:$count, timm:$base, timm:$mask))];
		}
		def SRL_SMALL_PSEUDO : SHPseudo<(outs GPR:$dst), (ins GPR:$src, GPR:$count, i32imm:$base, i32imm:$mask)> {
			let Pattern = [(set GPR:$dst, (SHsrlsmall GPR:$src, GPR:$count, timm:$base, timm:$mask))];
		}
	}
  def SRLri : SHPseudo<(outs GPR:$dst), (ins GPR:$src, i32imm:$amount)> {
    let Pattern = [(set GPR:$dst, (srl GPR:$src, u5imm:$amount))];
  }
  def SRAri : SHPseudo<(outs GPR:$dst), (ins GPR:$src, i32imm:$amount)> {
    let Pattern = [(set GPR:$dst, (sra GPR:$src, u5imm:$amount))];
  }
  def SHLrr : SHPseudo<(outs GPR:$dst), (ins GPR:$src, GPR:$amount)> {
    let Pattern = [(set GPR:$dst, (shl GPR:$src, GPR:$amount))];
  }
  def SRLrr : SHPseudo<(outs GPR:$dst), (ins GPR:$src, GPR:$amount)> {
    let Pattern = [(set GPR:$dst, (srl GPR:$src, GPR:$amount))];
  }
  def SRArr : SHPseudo<(outs GPR:$dst), (ins GPR:$src, GPR:$amount)> {
    let Pattern = [(set GPR:$dst, (sra GPR:$src, GPR:$amount))];
  }
  def SHL64_PSEUDO
      : SHPseudo<(outs GPR:$lo_dst, GPR:$hi_dst),
                 (ins GPR:$lo, GPR:$hi, GPR:$amount)> {
    let Pattern =
        [(set GPR:$lo_dst, GPR:$hi_dst,
              (SHshlparts GPR:$lo, GPR:$hi, GPR:$amount))];
  }
  def SRL64_PSEUDO
      : SHPseudo<(outs GPR:$lo_dst, GPR:$hi_dst),
                 (ins GPR:$lo, GPR:$hi, GPR:$amount)> {
    let Pattern =
        [(set GPR:$lo_dst, GPR:$hi_dst,
              (SHsrlparts GPR:$lo, GPR:$hi, GPR:$amount))];
  }
  def SRA64_PSEUDO
      : SHPseudo<(outs GPR:$lo_dst, GPR:$hi_dst),
                 (ins GPR:$lo, GPR:$hi, GPR:$amount)> {
    let Pattern =
        [(set GPR:$lo_dst, GPR:$hi_dst,
              (SHsraparts GPR:$lo, GPR:$hi, GPR:$amount))];
  }
	def SELECT_PSEUDO
		: SHPseudo<(outs GPR:$dst),
					 (ins GPR:$condition, GPR:$true_value, GPR:$false_value)> {
		let Pattern =
			[(set GPR:$dst,
				  (SHselect GPR:$condition, GPR:$true_value, GPR:$false_value))];
	}
	def SELECT_CC_PSEUDO
		: SHPseudo<(outs GPR:$dst),
			(ins GPR:$lhs, GPR:$rhs, GPR:$true_value, GPR:$false_value,
				i32imm:$cc)>;
}

def : Pat<(SHshl GPR:$src, GPR:$amount),
	(SHLrr GPR:$src, GPR:$amount)>;

let usesCustomInserter = 1, isBranch = 1, isTerminator = 1 in {
	def BR_FP_OR_F32_PSEUDO
		: SHPseudo<(outs),
			(ins FR:$lhs, FR:$rhs, i32imm:$branch_on_true,
				brtarget8:$target)>;
	def BR_FP_OR_F64_PSEUDO
		: SHPseudo<(outs),
			(ins DR:$lhs, DR:$rhs, i32imm:$branch_on_true,
				brtarget8:$target)>;
}

// Multiply, divide, and condition-state instructions.
let isCommutable = 1, Defs = [MACL], Predicates = [HasSH2] in
def MUL_L : SHRegReg<0x0, 0x7, (outs), (ins GPR:$src, GPR:$dst),
                     "mul.l\t$src,$dst">;

let isCommutable = 1, Defs = [MACH, MACL], Predicates = [HasSH2] in {
	def DMULS_L : SHRegReg<0x3, 0xd, (outs), (ins GPR:$src, GPR:$dst),
												 "dmuls.l\t$src,$dst">;
	def DMULU_L : SHRegReg<0x3, 0x5, (outs), (ins GPR:$src, GPR:$dst),
												 "dmulu.l\t$src,$dst">;
}

let isCommutable = 1, Defs = [MACL] in {
	def MULS_W : SHRegReg<0x2, 0xf, (outs), (ins GPR:$src, GPR:$dst),
												"muls.w\t$src,$dst">;
	def MULU_W : SHRegReg<0x2, 0xe, (outs), (ins GPR:$src, GPR:$dst),
												"mulu.w\t$src,$dst">;
}

let mayLoad = 1, Uses = [SR, SBit, MACH, MACL], Defs = [MACH, MACL],
		Constraints = "$lhs_wb = $lhs, $rhs_wb = $rhs" in {
	let Predicates = [HasSH2] in
	def MAC_L : SHMAC<0x0, 0xf,
										(outs postincgpr:$lhs_wb, postincgpr:$rhs_wb),
										(ins GPR:$lhs, GPR:$rhs),
										"mac.l\t$lhs_wb,$rhs_wb">;
	def MAC_W : SHMAC<0x4, 0xf,
										(outs postincgpr:$lhs_wb, postincgpr:$rhs_wb),
										(ins GPR:$lhs, GPR:$rhs),
										"mac.w\t$lhs_wb,$rhs_wb">;
}

let Uses = [MACL] in
def STS_MACL
    : SHSpecialToReg<0x1a, (outs GPR:$dst), (ins), "sts\tmacl,$dst">;

let Uses = [GBR], hasSideEffects = 1 in
def STC_GBR
    : SHSpecialToReg<0x12, (outs GPR:$dst), (ins), "stc\tgbr,$dst"> {
  let Pattern = [(set GPR:$dst, (SHthreadptr))];
}

let Uses = [MACH] in
def STS_MACH : SHSpecialToReg<0x0a, (outs GPR:$dst), (ins),
															"sts\tmach,$dst">;

let Uses = [PR] in
def STS_PR : SHSpecialToReg<0x2a, (outs GPR:$dst), (ins),
														"sts\tpr,$dst">;

let Uses = [SR, SBit, MBit, QBit, TBit] in
def STC_SR : SHSpecialToReg<0x02, (outs GPR:$dst), (ins),
												"stc\tsr,$dst">;

let Uses = [VBR] in
def STC_VBR : SHSpecialToReg<0x22, (outs GPR:$dst), (ins),
												 "stc\tvbr,$dst">;

let Uses = [SSR], Predicates = [HasExceptionState] in
def STC_SSR : SHSpecialToReg<0x32, (outs GPR:$dst), (ins), "stc\tssr,$dst">;

let Uses = [SPC], Predicates = [HasExceptionState] in
def STC_SPC : SHSpecialToReg<0x42, (outs GPR:$dst), (ins), "stc\tspc,$dst">;

let Uses = [SGR], Predicates = [HasSH4DebugState] in
def STC_SGR : SHSpecialToReg<0x3a, (outs GPR:$dst), (ins), "stc\tsgr,$dst">;

let Uses = [DBR], Predicates = [HasSH4DebugState] in
def STC_DBR : SHSpecialToReg<0xfa, (outs GPR:$dst), (ins), "stc\tdbr,$dst">;

let Predicates = [HasBankedGPRs] in
def STC_BANK : SHBankStore<(outs GPR:$dst), (ins bankedgpr:$bank_src),
	"stc\t$bank_src,$dst">;

let Defs = [MBit, QBit, TBit] in
def DIV0U : SHFixed<0x0019, "div0u">;

let Defs = [MBit, QBit, TBit] in
def DIV0S : SHRegReg<0x2, 0x7, (outs), (ins GPR:$src, GPR:$dst),
                     "div0s\t$src,$dst">;

let Constraints = "$dst = $dst_in", Uses = [MBit, QBit, TBit],
    Defs = [QBit, TBit] in
def DIV1 : SHRegReg<0x3, 0x4, (outs GPR:$dst),
                    (ins GPR:$dst_in, GPR:$src), "div1\t$src,$dst">;

let Constraints = "$dst = $dst_in", Uses = [TBit], Defs = [TBit] in
def ROTCL
    : SHOneReg<0x24, (outs GPR:$dst), (ins GPR:$dst_in), "rotcl\t$dst">;

let Constraints = "$dst = $dst_in", Uses = [TBit], Defs = [TBit] in
def ROTCR
    : SHOneReg<0x25, (outs GPR:$dst), (ins GPR:$dst_in), "rotcr\t$dst">;

let Constraints = "$dst = $dst_in", Defs = [TBit] in {
	def ROTL
		: SHOneReg<0x04, (outs GPR:$dst), (ins GPR:$dst_in), "rotl\t$dst",
					 [(set GPR:$dst, (SHrotl1 GPR:$dst_in))]>;
	def ROTR
		: SHOneReg<0x05, (outs GPR:$dst), (ins GPR:$dst_in), "rotr\t$dst",
					 [(set GPR:$dst, (SHrotr1 GPR:$dst_in))]>;
}

let Constraints = "$dst = $dst_in", Uses = [TBit], Defs = [TBit] in {
  def ADDC : SHRegReg<0x3, 0xe, (outs GPR:$dst),
                      (ins GPR:$dst_in, GPR:$src), "addc\t$src,$dst",
                      [(set GPR:$dst, (adde GPR:$dst_in, GPR:$src))]>;
  def SUBC : SHRegReg<0x3, 0xa, (outs GPR:$dst),
                      (ins GPR:$dst_in, GPR:$src), "subc\t$src,$dst",
                      [(set GPR:$dst, (sube GPR:$dst_in, GPR:$src))]>;
}

let Constraints = "$dst = $dst_in", Uses = [TBit], Defs = [TBit] in
def NEGC : SHRegReg<0x6, 0xa, (outs GPR:$dst),
										(ins GPR:$dst_in, GPR:$src), "negc\t$src,$dst">;

let Constraints = "$dst = $dst_in", Defs = [TBit] in {
	def ADDV : SHRegReg<0x3, 0xf, (outs GPR:$dst),
							 (ins GPR:$dst_in, GPR:$src), "addv\t$src,$dst",
							 [(set GPR:$dst, (SHaddv GPR:$dst_in, GPR:$src))]>;
	def SUBV : SHRegReg<0x3, 0xb, (outs GPR:$dst),
							 (ins GPR:$dst_in, GPR:$src), "subv\t$src,$dst",
							 [(set GPR:$dst, (SHsubv GPR:$dst_in, GPR:$src))]>;
}

let Defs = [TBit], isCompare = 1 in {
  def CMP_EQ : SHRegReg<0x3, 0x0, (outs), (ins GPR:$src, GPR:$dst),
                        "cmp/eq\t$src,$dst",
                        [(SHcmpeq GPR:$dst, GPR:$src)]>;
  def CMP_HS : SHRegReg<0x3, 0x2, (outs), (ins GPR:$src, GPR:$dst),
                        "cmp/hs\t$src,$dst",
                        [(SHcmphs GPR:$dst, GPR:$src)]>;
  def CMP_GE : SHRegReg<0x3, 0x3, (outs), (ins GPR:$src, GPR:$dst),
                        "cmp/ge\t$src,$dst",
                        [(SHcmpge GPR:$dst, GPR:$src)]>;
  def CMP_HI : SHRegReg<0x3, 0x6, (outs), (ins GPR:$src, GPR:$dst),
                        "cmp/hi\t$src,$dst",
                        [(SHcmphi GPR:$dst, GPR:$src)]>;
  def CMP_GT : SHRegReg<0x3, 0x7, (outs), (ins GPR:$src, GPR:$dst),
                        "cmp/gt\t$src,$dst",
                        [(SHcmpgt GPR:$dst, GPR:$src)]>;
}

// Control-transfer instructions.
let isBranch = 1, isTerminator = 1, Uses = [TBit] in {
  def BT : SHBranch8<0x89, (ins brtarget8:$target), "bt\t$target",
                     [(SHbt bb:$target)]>;
  def BF : SHBranch8<0x8b, (ins brtarget8:$target), "bf\t$target",
                     [(SHbf bb:$target)]>;
}

let isBranch = 1, isTerminator = 1, Uses = [TBit], hasDelaySlot = 1,
	Predicates = [HasSH2] in {
	def BT_S : SHBranch8<0x8d, (ins brtarget8:$target), "bt/s\t$target">;
	def BF_S : SHBranch8<0x8f, (ins brtarget8:$target), "bf/s\t$target">;
}

let isBranch = 1, isTerminator = 1, isBarrier = 1, hasDelaySlot = 1 in
def BRA : SHBranch12<0xa, (ins brtarget12:$target), "bra\t$target",
                     [(br bb:$target)]>;

let isBranch = 1, isIndirectBranch = 1, isTerminator = 1, isBarrier = 1,
    hasDelaySlot = 1 in
def JMP : SHIndirectBranch<(ins GPR:$target, variable_ops), "jmp\t@$target",
                           [(brind GPR:$target)]>;

let isBranch = 1, isIndirectBranch = 1, isTerminator = 1, isBarrier = 1,
	hasDelaySlot = 1, Predicates = [HasSH2] in
def BRAF : SHRegRelative<0x23, (outs), (ins GPR:$target, variable_ops),
												 "braf\t$target">;

let isBranch = 1, isTerminator = 1, isBarrier = 1, mayLoad = 1,
    Defs = [R0] in
def SH_LONG_BRANCH
    : SHPseudo<(outs GPRNoR0:$scratch),
               (ins brtarget12:$target, i32imm:$restore_cfi)>;

let isBranch = 1, isTerminator = 1, isBarrier = 1, mayLoad = 1 in
def SH_REL16_BRANCH
	: SHPseudo<(outs GPR:$scratch),
		     (ins brtarget12:$target, i32imm:$restore_cfi)>;

let usesCustomInserter = 1, isBranch = 1, isIndirectBranch = 1,
    isTerminator = 1, isBarrier = 1, mayLoad = 1 in
def SH_JT_DISPATCH
    : SHPseudo<(outs), (ins GPR:$index, GPR:$base, i32imm:$jti)> {
  let Pattern = [(SHbrjt GPR:$index, GPR:$base, tjumptable:$jti)];
}

let isCall = 1, hasDelaySlot = 1, Defs = [PR] in {
  def BSR : SHBranch12<0xb, (ins calltarget12:$target), "bsr\t$target",
                       [(SHcall tglobaladdr:$target)]>;
	def JSR : SHIndirectCall<(ins GPR:$callee), "jsr\t@$callee",
                           [(SHcall GPR:$callee)]>;
	let Predicates = [HasSH2], isNotDuplicable = 1 in
	let Pattern = [(SHcall_pcrel GPR:$target)] in
	def BSRF : SHRegRelative<0x03, (outs), (ins GPR:$target, variable_ops),
		"bsrf\t$target">;
}

let isCodeGenOnly = 1, isCall = 1, isReturn = 1, isBranch = 1,
	isIndirectBranch = 1, isTerminator = 1, isBarrier = 1,
	hasDelaySlot = 1 in
def TAIL_JMP : SHIndirectBranch<(ins GPRTailCall:$target, variable_ops),
	"jmp\t@$target", [(SHtailcall GPRTailCall:$target)]>;

// Special-register and ordinary memory transfers.
let mayStore = 1, Uses = [PR], Constraints = "$base_wb = $base" in
def STS_L_PR
    : SHPRStack<0x2, (outs predecgpr:$base_wb), (ins GPR:$base),
                "sts.l\tpr,$base_wb">;

let mayLoad = 1, Defs = [PR], Constraints = "$base_wb = $base" in
def LDS_L_PR
    : SHPRStack<0x6, (outs postincgpr:$base_wb), (ins GPR:$base),
                "lds.l\t$base_wb,pr">;

let mayLoad = 1, Constraints = "$base_wb = $base" in {
	let Defs = [SR, SBit, MBit, QBit, TBit] in
	def LDC_L_SR : SHRegStack<0x0, 0x7, (outs postincgpr:$base_wb),
														(ins GPR:$base), "ldc.l\t$base_wb,sr">;
	let Defs = [GBR] in
	def LDC_L_GBR : SHRegStack<0x1, 0x7, (outs postincgpr:$base_wb),
														 (ins GPR:$base), "ldc.l\t$base_wb,gbr">;
	let Defs = [VBR] in
	def LDC_L_VBR : SHRegStack<0x2, 0x7, (outs postincgpr:$base_wb),
														 (ins GPR:$base), "ldc.l\t$base_wb,vbr">;
	let Defs = [MACH] in
	def LDS_L_MACH : SHRegStack<0x0, 0x6, (outs postincgpr:$base_wb),
															(ins GPR:$base), "lds.l\t$base_wb,mach">;
	let Defs = [MACL] in
	def LDS_L_MACL : SHRegStack<0x1, 0x6, (outs postincgpr:$base_wb),
														(ins GPR:$base), "lds.l\t$base_wb,macl">;
	let Defs = [SSR], Predicates = [HasExceptionState] in
	def LDC_L_SSR : SHRegStack<0x3, 0x7, (outs postincgpr:$base_wb),
		(ins GPR:$base), "ldc.l\t$base_wb,ssr">;
	let Defs = [SPC], Predicates = [HasExceptionState] in
	def LDC_L_SPC : SHRegStack<0x4, 0x7, (outs postincgpr:$base_wb),
		(ins GPR:$base), "ldc.l\t$base_wb,spc">;
	let Defs = [DBR], Predicates = [HasSH4DebugState] in
	def LDC_L_DBR : SHRegStack<0xf, 0x6, (outs postincgpr:$base_wb),
		(ins GPR:$base), "ldc.l\t$base_wb,dbr">;
}

let mayLoad = 1, Constraints = "$base_wb = $base",
	Predicates = [HasBankedGPRs] in
def LDC_L_BANK : SHBankLoadStack<
	(outs postincgpr:$base_wb, bankedgpr:$bank_dst), (ins GPR:$base),
	"ldc.l\t$base_wb,$bank_dst">;

let mayStore = 1, Constraints = "$base_wb = $base" in {
	let Uses = [SR, SBit, MBit, QBit, TBit] in
	def STC_L_SR : SHRegStack<0x0, 0x3, (outs predecgpr:$base_wb),
														(ins GPR:$base), "stc.l\tsr,$base_wb">;
	let Uses = [GBR] in
	def STC_L_GBR : SHRegStack<0x1, 0x3, (outs predecgpr:$base_wb),
														 (ins GPR:$base), "stc.l\tgbr,$base_wb">;
	let Uses = [VBR] in
	def STC_L_VBR : SHRegStack<0x2, 0x3, (outs predecgpr:$base_wb),
														 (ins GPR:$base), "stc.l\tvbr,$base_wb">;
	let Uses = [MACH] in
	def STS_L_MACH : SHRegStack<0x0, 0x2, (outs predecgpr:$base_wb),
															(ins GPR:$base), "sts.l\tmach,$base_wb">;
	let Uses = [MACL] in
	def STS_L_MACL : SHRegStack<0x1, 0x2, (outs predecgpr:$base_wb),
														(ins GPR:$base), "sts.l\tmacl,$base_wb">;
	let Uses = [SSR], Predicates = [HasExceptionState] in
	def STC_L_SSR : SHRegStack<0x3, 0x3, (outs predecgpr:$base_wb),
		(ins GPR:$base), "stc.l\tssr,$base_wb">;
	let Uses = [SPC], Predicates = [HasExceptionState] in
	def STC_L_SPC : SHRegStack<0x4, 0x3, (outs predecgpr:$base_wb),
		(ins GPR:$base), "stc.l\tspc,$base_wb">;
	let Uses = [SGR], Predicates = [HasSH4DebugState] in
	def STC_L_SGR : SHRegStack<0x3, 0x2, (outs predecgpr:$base_wb),
		(ins GPR:$base), "stc.l\tsgr,$base_wb">;
	let Uses = [DBR], Predicates = [HasSH4DebugState] in
	def STC_L_DBR : SHRegStack<0xf, 0x2, (outs predecgpr:$base_wb),
		(ins GPR:$base), "stc.l\tdbr,$base_wb">;
}

let mayStore = 1, Constraints = "$base_wb = $base",
	Predicates = [HasBankedGPRs] in
def STC_L_BANK : SHBankStoreStack<(outs predecgpr:$base_wb),
	(ins bankedgpr:$bank_src, GPR:$base), "stc.l\t$bank_src,$base_wb">;

let hasSideEffects = 1, Predicates = [HasPrefetch] in
def PREF : SHSystemMemory<0x83, (outs), (ins longmemreg:$addr),
	"pref\t$addr">;

let hasSideEffects = 1, IsDelaySlotIllegal = 1,
	Predicates = [HasSH4ACommon], SchedRW = [SH4WriteLS] in
def ICBI : SHSystemMemory<0xe3, (outs), (ins longmemreg:$addr),
	"icbi\t$addr">;

let hasSideEffects = 1, Predicates = [HasSH4ACommon],
	SchedRW = [SH4WriteLS] in
def SYNCO : SHFixed<0x00ab, "synco"> {
	let Pattern = [(SHsynco)];
}

let mayStore = 1, hasSideEffects = 1, Uses = [R0],
	Predicates = [HasSH4Cache], SchedRW = [SH4WriteStore] in
def MOVCA_L : SHSystemMemory<0xc3, (outs), (ins longmemreg:$addr),
	"movca.l\tr0,$addr">;

let hasSideEffects = 1, Predicates = [HasSH4Cache],
	SchedRW = [SH4WriteStore] in
def OCBI : SHSystemMemory<0x93, (outs), (ins longmemreg:$addr),
	"ocbi\t$addr">;

let mayLoad = 1, mayStore = 1, hasSideEffects = 1,
	Predicates = [HasSH4Cache], SchedRW = [SH4WriteStore] in {
	def OCBP : SHSystemMemory<0xa3, (outs), (ins longmemreg:$addr),
		"ocbp\t$addr">;
	def OCBWB : SHSystemMemory<0xb3, (outs), (ins longmemreg:$addr),
		"ocbwb\t$addr">;
}

let mayLoad = 1, Constraints = "$base_wb = $base,@earlyclobber $dst" in {
	def MOVB_load_postinc : SHPostIncLoad<0x4,
			(outs GPR:$dst, postincgpr:$base_wb), (ins GPR:$base),
			"mov.b\t$base_wb,$dst">;
	def MOVW_load_postinc : SHPostIncLoad<0x5,
			(outs GPR:$dst, postincgpr:$base_wb), (ins GPR:$base),
			"mov.w\t$base_wb,$dst">;
	def MOVL_load_postinc : SHPostIncLoad<0x6,
			(outs GPR:$dst, postincgpr:$base_wb), (ins GPR:$base),
			"mov.l\t$base_wb,$dst">;
}

let mayStore = 1, Constraints = "$base_wb = $base,@earlyclobber $base_wb" in {
	def MOVB_store_predec : SHPreDecStore<0x4, (outs predecgpr:$base_wb),
			(ins GPR:$src, GPR:$base), "mov.b\t$src,$base_wb">;
	def MOVW_store_predec : SHPreDecStore<0x5, (outs predecgpr:$base_wb),
			(ins GPR:$src, GPR:$base), "mov.w\t$src,$base_wb">;
	def MOVL_store_predec : SHPreDecStore<0x6, (outs predecgpr:$base_wb),
			(ins GPR:$src, GPR:$base), "mov.l\t$src,$base_wb">;
}

let mayLoad = 1, hasExtraDefRegAllocReq = 0 in
def MOVL_load_reg
    : SHLongLoadReg<(outs GPR:$dst), (ins longmemreg:$addr),
                    "mov.l\t$addr,$dst",
                    [(set GPR:$dst, (load addr_reg:$addr))]>;

let mayLoad = 1, Uses = [R0], hasExtraDefRegAllocReq = 0 in {
	def MOVB_load_indexed : SHIndexedLoad<0xc, (outs GPR:$dst),
			(ins indexedmem:$base), "mov.b\t$base,$dst">;
	def MOVW_load_indexed : SHIndexedLoad<0xd, (outs GPR:$dst),
			(ins indexedmem:$base), "mov.w\t$base,$dst">;
}

let mayStore = 1, Uses = [R0] in {
	def MOVB_store_indexed : SHIndexedStore<0x4, (outs),
			(ins GPR:$src, indexedmem:$base), "mov.b\t$src,$base">;
	def MOVW_store_indexed : SHIndexedStore<0x5, (outs),
			(ins GPR:$src, indexedmem:$base), "mov.w\t$src,$base">;
	def MOVL_store_indexed : SHIndexedStore<0x6, (outs),
			(ins GPR:$src, indexedmem:$base), "mov.l\t$src,$base">;
}

let mayLoad = 1, Uses = [R0], hasExtraDefRegAllocReq = 0 in
def MOVL_load_indexed
    : SHLongLoadIndexed<(outs GPR:$dst), (ins indexedmem:$base),
                        "mov.l\t$base,$dst">;

let mayLoad = 1 in
def MOVL_load_pc
    : SHLongLoadPC<(outs GPR:$dst), (ins pcliteral:$target),
                   "mov.l\t$target,$dst",
                   []>;

let mayLoad = 1 in
def MOVW_load_pc : SHPCLoad<0x9, (outs GPR:$dst),
														(ins pcwordliteral:$target),
														"mov.w\t$target,$dst">;

def MOVA
    : SHMOVA<(outs r0reg:$dst), (ins pcliteral:$target),
             "mova\t$target,$dst">;

let Uses = [GBR] in {
	let mayLoad = 1 in {
		def MOVB_load_gbr : SHGBRDisp<0xc4, (outs r0reg:$dst),
																	(ins gbrbytedisp:$addr),
																	"mov.b\t$addr,$dst">;
		def MOVW_load_gbr : SHGBRDisp<0xc5, (outs r0reg:$dst),
																	(ins gbrworddisp:$addr),
																	"mov.w\t$addr,$dst">;
		def MOVL_load_gbr : SHGBRDisp<0xc6, (outs r0reg:$dst),
																	(ins gbrlongdisp:$addr),
																	"mov.l\t$addr,$dst">;
	}
	let mayStore = 1 in {
		def MOVB_store_gbr : SHGBRDisp<0xc0, (outs),
				(ins r0reg:$src, gbrbytedisp:$addr), "mov.b\t$src,$addr">;
		def MOVW_store_gbr : SHGBRDisp<0xc1, (outs),
				(ins r0reg:$src, gbrworddisp:$addr), "mov.w\t$src,$addr">;
		def MOVL_store_gbr : SHGBRDisp<0xc2, (outs),
				(ins r0reg:$src, gbrlongdisp:$addr), "mov.l\t$src,$addr">;
	}
}

let Uses = [R0, GBR], mayLoad = 1, mayStore = 1 in {
	def AND_B_GBR : SHGBRImm<0xcd, (outs),
			(ins uimm8:$imm, r0gbr:$addr), "and.b\t$imm,$addr">;
	def OR_B_GBR : SHGBRImm<0xcf, (outs),
			(ins uimm8:$imm, r0gbr:$addr), "or.b\t$imm,$addr">;
	def XOR_B_GBR : SHGBRImm<0xce, (outs),
			(ins uimm8:$imm, r0gbr:$addr), "xor.b\t$imm,$addr">;
}

let Uses = [R0, GBR], Defs = [TBit], mayLoad = 1, isCompare = 1 in
def TST_B_GBR : SHGBRImm<0xcc, (outs),
		(ins uimm8:$imm, r0gbr:$addr), "tst.b\t$imm,$addr">;

// Literal-island, PIC, and TLS pseudos.
let mayLoad = 1, isReMaterializable = 1 in
def MOVL_load_pc_island
    : SHPseudo<(outs GPR:$dst),
               (ins pcliteral:$target, island_instance:$island)> {
  let Pattern = [(set GPR:$dst, (load tconstpool:$target))];
}

let mayLoad = 1, isReMaterializable = 1 in
def MOVW_load_pc_island
	: SHPseudo<(outs GPR:$dst),
		(ins pcwordliteral:$target, island_instance:$island)>;

let mayLoad = 1, Defs = [R0, R12] in
def SH_PIC_SETUP
    : SHPseudo<(outs),
               (ins pcliteral:$target, island_instance:$island)>;

let mayLoad = 1, Defs = [R0] in
def SH_PIC_ADDRESS
    : SHPseudo<(outs GPRNoR0:$dst),
               (ins pcliteral:$target, island_instance:$island)> {
  let Pattern = [(set GPRNoR0:$dst, (SHpicaddr tconstpool:$target))];
}

let mayLoad = 1, Uses = [R12] in
def SH_PIC_GOTOFF
    : SHPseudo<(outs GPR:$dst),
               (ins pcliteral:$target, island_instance:$island)> {
  let Pattern = [(set GPR:$dst, (SHpicgotoff tconstpool:$target))];
}

let isCall = 1, mayLoad = 1, Defs = [R0, R1, R4, PR], Uses = [R12] in
def SH_TLS_CALL
    : SHPseudo<(outs),
               (ins pcliteral:$descriptor,
                    island_instance:$descriptor_island,
                    pcliteral:$resolver,
                    island_instance:$resolver_island,
                    variable_ops)>;

let mayLoad = 1, Defs = [R0], Uses = [R12, GBR] in
def SH_TLS_IE
    : SHPseudo<(outs GPRNoR0:$dst),
               (ins pcliteral:$target, island_instance:$island)>;

let hasSideEffects = 0, isNotDuplicable = 1 in
def SH_CONSTPOOL_ENTRY
    : SHPseudo<(outs),
               (ins cpinst_operand:$cpi, i32imm:$instance, i32imm:$size,
                    i32imm:$alignment)>;

let hasSideEffects = 0, isNotDuplicable = 1 in
def SH_JUMPTABLE
    : SHPseudo<(outs),
               (ins i32imm:$jti, pcliteral:$table, pcliteral:$base,
                    i32imm:$entry_size)>;

// Frame-index memory pseudos and extension instructions.
let mayStore = 1 in
def MOVL_store_reg
    : SHLongStoreReg<(outs), (ins GPR:$src, longmemreg:$addr),
                     "mov.l\t$src,$addr",
                     [(store GPR:$src, addr_reg:$addr)]>;

let mayLoad = 1, hasExtraDefRegAllocReq = 0 in
def MOVL_load_disp
    : SHLongLoadDisp<(outs GPR:$dst), (ins longmemdisp:$addr),
                     "mov.l\t$addr,$dst",
                     [(set GPR:$dst, (load addr_disp:$addr))]>;

let mayStore = 1 in
def MOVL_store_disp
    : SHLongStoreDisp<(outs), (ins GPR:$src, longmemdisp:$addr),
                      "mov.l\t$src,$addr",
                      [(store GPR:$src, addr_disp:$addr)]>;

let mayStore = 1 in {
  def MOVB_store_reg
      : SHNarrowStoreReg<0x0, (outs), (ins GPR:$src, longmemreg:$addr),
                         "mov.b\t$src,$addr",
                         [(truncstorei8 GPR:$src,
                                          addr_narrow_reg:$addr)]>;
  def MOVW_store_reg
      : SHNarrowStoreReg<0x1, (outs), (ins GPR:$src, longmemreg:$addr),
                         "mov.w\t$src,$addr",
                         [(truncstorei16 GPR:$src,
                                           addr_narrow_reg:$addr)]>;
}

let mayLoad = 1, hasExtraDefRegAllocReq = 0 in {
  def MOVB_load_reg
      : SHNarrowLoadReg<0x0, (outs GPR:$dst), (ins longmemreg:$addr),
                        "mov.b\t$addr,$dst",
                        [(set GPR:$dst,
                              (sextloadi8 addr_narrow_reg:$addr))]>;
  def MOVW_load_reg
      : SHNarrowLoadReg<0x1, (outs GPR:$dst), (ins longmemreg:$addr),
                        "mov.w\t$addr,$dst",
                        [(set GPR:$dst,
                              (sextloadi16 addr_narrow_reg:$addr))]>;
}

let mayStore = 1 in {
  def MOVB_store_disp
      : SHNarrowStoreDisp<0x80, (outs),
                          (ins r0reg:$src, bytememdisp:$addr),
                          "mov.b\t$src,$addr">;
  def MOVW_store_disp
      : SHNarrowStoreDisp<0x81, (outs),
                          (ins r0reg:$src, wordmemdisp:$addr),
                          "mov.w\t$src,$addr">;
}

let mayLoad = 1 in {
  def MOVB_load_disp
      : SHNarrowLoadDisp<0x84, (outs r0reg:$dst),
                         (ins bytememdisp:$addr), "mov.b\t$addr,$dst">;
  def MOVW_load_disp
      : SHNarrowLoadDisp<0x85, (outs r0reg:$dst),
                         (ins wordmemdisp:$addr), "mov.w\t$addr,$dst">;
}

// These extensions have independent, fully described GPR operands.
let hasExtraSrcRegAllocReq = 0, hasExtraDefRegAllocReq = 0 in {
def EXTUB : SHRegReg<0x6, 0xc, (outs GPR:$dst), (ins GPR:$src),
                     "extu.b\t$src,$dst",
                     [(set GPR:$dst, (and GPR:$src, 0xff))]>;
def EXTUW : SHRegReg<0x6, 0xd, (outs GPR:$dst), (ins GPR:$src),
                     "extu.w\t$src,$dst",
                     [(set GPR:$dst, (and GPR:$src, 0xffff))]>;
def EXTSB : SHRegReg<0x6, 0xe, (outs GPR:$dst), (ins GPR:$src),
                     "exts.b\t$src,$dst",
                     [(set GPR:$dst, (sext_inreg GPR:$src, i8))]>;
def EXTSW : SHRegReg<0x6, 0xf, (outs GPR:$dst), (ins GPR:$src),
                     "exts.w\t$src,$dst",
                     [(set GPR:$dst, (sext_inreg GPR:$src, i16))]>;
}

let mayLoad = 1, Defs = [R0] in {
  def MOVB_load_frame
      : SHPseudo<(outs GPR:$dst), (ins bytememdisp:$addr)> {
    let Pattern =
        [(set GPR:$dst, (sextloadi8 addr_byte_frame:$addr))];
  }
  def MOVW_load_frame
      : SHPseudo<(outs GPR:$dst), (ins wordmemdisp:$addr)> {
    let Pattern =
        [(set GPR:$dst, (sextloadi16 addr_word_frame:$addr))];
  }
}

let mayStore = 1, Defs = [R0], hasPostISelHook = 1 in {
  def MOVB_store_frame
      : SHPseudo<(outs), (ins GPR:$src, bytememdisp:$addr)> {
    let Pattern =
        [(truncstorei8 GPR:$src, addr_byte_frame:$addr)];
  }
  def MOVW_store_frame
      : SHPseudo<(outs), (ins GPR:$src, wordmemdisp:$addr)> {
    let Pattern =
        [(truncstorei16 GPR:$src, addr_word_frame:$addr)];
  }
}

def : Pat<(i32 (extloadi8 addr_narrow_reg:$addr)),
          (MOVB_load_reg addr_narrow_reg:$addr)>;
def : Pat<(i32 (extloadi16 addr_narrow_reg:$addr)),
          (MOVW_load_reg addr_narrow_reg:$addr)>;
def : Pat<(i32 (zextloadi8 addr_narrow_reg:$addr)),
          (EXTUB (MOVB_load_reg addr_narrow_reg:$addr))>;
def : Pat<(i32 (zextloadi16 addr_narrow_reg:$addr)),
          (EXTUW (MOVW_load_reg addr_narrow_reg:$addr))>;

def : Pat<(i32 (extloadi8 addr_byte_frame:$addr)),
          (MOVB_load_frame addr_byte_frame:$addr)>;
def : Pat<(i32 (extloadi16 addr_word_frame:$addr)),
          (MOVW_load_frame addr_word_frame:$addr)>;
def : Pat<(i32 (zextloadi8 addr_byte_frame:$addr)),
          (EXTUB (MOVB_load_frame addr_byte_frame:$addr))>;
def : Pat<(i32 (zextloadi16 addr_word_frame:$addr)),
          (EXTUW (MOVW_load_frame addr_word_frame:$addr))>;

include "SH2AInstrInfo.td"
include "SH4AInstrInfo.td"
include "SHDSPInstrInfo.td"
include "SHFPUInstrInfo.td"

let SchedModel = SH4AModel in {
	def : InstRW<[SH4AWriteAtomic], (instrs MOVCO_L, MOVLI_L)>;
	def : InstRW<[SH4AWriteMOVUA], (instrs MOVUA_L, MOVUA_L_POSTINC)>;
	def : InstRW<[SH4AWriteICBI], (instrs ICBI)>;
	def : InstRW<[SH4AWritePREFI], (instrs PREFI)>;
	def : InstRW<[SH4AWriteSYNCO], (instrs SYNCO)>;
	def : InstRW<[SH4AWriteFSRRA], (instrs FSRRA)>;
	def : InstRW<[SH4AWriteFSCA], (instrs FSCA)>;
	def : InstRW<[SH4AWriteFPChange], (instrs FPCHG)>;
}

let SchedModel = SH4ALDSPModel in {
	def : InstRW<[SH4WriteMT], (instrs MOVrr, MOVri, NOP)>;
	def : InstRW<[SH4WriteEX], (instrs
		TST, TSTri, CMP_EQri, CMP_PL, CMP_PZ, CMP_STR,
		CMP_EQ, CMP_HS, CMP_GE, CMP_HI, CMP_GT,
		ADDrr, ADDri, ADDC, ADDV, SUBrr, SUBC, SUBV,
		NEG, NEGC, ANDrr, ORrr, XORrr, NOT, ANDri, ORri, XORri,
		MOVA, DT, DIV0S, DIV0U, DIV1, EXTSB, EXTSW, EXTUB, EXTUW,
		SHLL, SHLR, SHAR, SHAL, SHLL2, SHLR2, SHLL8, SHLR8,
		SHLL16, SHLR16, ROTCL, ROTCR, ROTL, ROTR, SWAP_B, SWAP_W,
		XTRCT)>;
	def : InstRW<[SH4WriteEX, ReadDefault, SH4ReadDynamicShift],
		(instrs SHAD, SHLD)>;
	def : InstRW<[SH4WriteLoad], (instrs
		MOVB_load_reg, MOVW_load_reg, MOVL_load_reg,
		MOVB_load_postinc, MOVW_load_postinc, MOVL_load_postinc,
		MOVB_load_disp, MOVW_load_disp, MOVL_load_disp,
		MOVB_load_indexed, MOVW_load_indexed, MOVL_load_indexed,
		MOVW_load_pc, MOVL_load_pc,
		MOVB_load_gbr, MOVW_load_gbr, MOVL_load_gbr)>;
	def : InstRW<[SH4WriteStore], (instrs
		MOVB_store_reg, MOVW_store_reg, MOVL_store_reg,
		MOVB_store_predec, MOVW_store_predec, MOVL_store_predec,
		MOVB_store_disp, MOVW_store_disp, MOVL_store_disp,
		MOVB_store_indexed, MOVW_store_indexed, MOVL_store_indexed,
		MOVB_store_gbr, MOVW_store_gbr, MOVL_store_gbr,
		PREF, OCBI, OCBP, OCBWB)>;
	def : InstRW<[SH4WriteBR], (instrs BT, BF, BT_S, BF_S, BRA, BSR)>;
	def : InstRW<[SH4WriteControl3], (instrs JMP, BRAF, JSR, BSRF, RTS)>;
	def : InstRW<[SH4WriteRTE5], (instrs RTE)>;
	def : InstRW<[SH4ALDSPWriteMul1], (instrs MULS_W, MULU_W)>;
	def : InstRW<[SH4ALDSPWriteMul2], (instrs MUL_L, DMULS_L, DMULU_L)>;
	def : InstRW<[SH4ALDSPWriteMACW], (instrs MAC_W)>;
	def : InstRW<[SH4ALDSPWriteMACL], (instrs MAC_L)>;
	def : InstRW<[SH4WriteCO3], (instrs
		LDC_GBR, LDC_VBR, LDC_SSR, LDC_SPC, LDC_DBR,
		LDS_PR, LDS_MACH, LDS_MACL, STC_SGR, STC_DBR)>;
	def : InstRW<[SH4WriteCO4], (instrs LDC_SR)>;
	def : InstRW<[SH4WriteTrap7], (instrs TRAPA)>;
	def : InstRW<[SH4AWriteAtomic], (instrs MOVCO_L, MOVLI_L)>;
	def : InstRW<[SH4AWriteMOVUA], (instrs MOVUA_L, MOVUA_L_POSTINC)>;
	def : InstRW<[SH4AWriteICBI], (instrs ICBI)>;
	def : InstRW<[SH4AWritePREFI], (instrs PREFI)>;
	def : InstRW<[SH4AWriteSYNCO], (instrs SYNCO)>;
}

let SchedModel = SH4Model in {
	def : InstRW<[SH4WriteMT], (instrs MOVrr, NOP)>;
	def : InstRW<[SH4WriteEX], (instrs MOVri, MOVA, SHAD, SHLD)>;
	def : InstRW<[SH4WriteLoad], (instrs
		MOVB_load_reg, MOVW_load_reg, MOVL_load_reg,
		MOVB_load_postinc, MOVW_load_postinc, MOVL_load_postinc,
		MOVB_load_disp, MOVW_load_disp, MOVL_load_disp,
		MOVB_load_indexed, MOVW_load_indexed, MOVL_load_indexed,
		MOVW_load_pc, MOVL_load_pc,
		MOVB_load_gbr, MOVW_load_gbr, MOVL_load_gbr)>;
	def : InstRW<[SH4WriteStore], (instrs
		MOVB_store_reg, MOVW_store_reg, MOVL_store_reg,
		MOVB_store_predec, MOVW_store_predec, MOVL_store_predec,
		MOVB_store_disp, MOVW_store_disp, MOVL_store_disp,
		MOVB_store_indexed, MOVW_store_indexed, MOVL_store_indexed,
		MOVB_store_gbr, MOVW_store_gbr, MOVL_store_gbr,
		PREF)>;
	def : InstRW<[SH4WriteBR], (instrs BT, BF, BT_S, BF_S, BRA, BSR)>;
	def : InstRW<[SH4WriteControl3], (instrs JMP, BRAF, JSR, BSRF, RTS)>;
	def : InstRW<[SH4WriteRTE5], (instrs RTE)>;
	def : InstRW<[SH4WriteMul4], (instrs
		MUL_L, MULS_W, MULU_W, DMULS_L, DMULU_L, MAC_L)>;
	def : InstRW<[SH4WriteCO2], (instrs MAC_W)>;
	def : InstRW<[SH4WriteCO3], (instrs STS_MACL, STS_MACH)>;
	def : InstRW<[SH4WriteCO3], (instrs
		LDC_GBR, LDC_VBR, LDC_SSR, LDC_SPC, LDC_DBR,
		LDS_PR, LDS_MACH, LDS_MACL,
		STC_SGR, STC_DBR)>;
	def : InstRW<[SH4WriteCO4], (instrs LDC_SR)>;
	def : InstRW<[SH4WriteTrap7], (instrs TRAPA)>;
}

multiclass ST40InstrSchedule<SchedMachineModel Model> {
	let SchedModel = Model in {
		def : InstRW<[SH4WriteMT], (instrs
			MOVrr, NOP)>;
		def : InstRW<[SH4WriteMT1], (instrs
			TST, TSTri,
			CMP_EQri, CMP_PL, CMP_PZ, CMP_STR,
			CMP_EQ, CMP_HS, CMP_GE, CMP_HI, CMP_GT)>;
		def : InstRW<[SH4WriteEX], (instrs
			ADDrr, ADDri, SUBrr, NEG, ANDrr, ORrr, XORrr, NOT,
			ANDri, ORri, XORri, MOVri, MOVA,
			DT, SHLL, SHLR, SHAR, SHAL,
			SHLL2, SHLR2, SHLL8, SHLR8, SHLL16, SHLR16,
			ROTCL, ROTCR, ROTL, ROTR)>;
		def : InstRW<[SH4WriteEX, ReadDefault, SH4ReadDynamicShift],
			(instrs SHAD, SHLD)>;
		def : InstRW<[SH4WriteLoad], (instrs
			MOVB_load_reg, MOVW_load_reg, MOVL_load_reg,
			MOVB_load_postinc, MOVW_load_postinc, MOVL_load_postinc,
			MOVB_load_disp, MOVW_load_disp, MOVL_load_disp,
			MOVB_load_indexed, MOVW_load_indexed, MOVL_load_indexed,
			MOVW_load_pc, MOVL_load_pc,
			MOVB_load_gbr, MOVW_load_gbr, MOVL_load_gbr)>;
		def : InstRW<[SH4WriteStore], (instrs
			MOVB_store_reg, MOVW_store_reg, MOVL_store_reg,
			MOVB_store_predec, MOVW_store_predec, MOVL_store_predec,
			MOVB_store_disp, MOVW_store_disp, MOVL_store_disp,
			MOVB_store_indexed, MOVW_store_indexed, MOVL_store_indexed,
			MOVB_store_gbr, MOVW_store_gbr, MOVL_store_gbr,
			PREF)>;
		def : InstRW<[SH4WriteBR], (instrs BT, BF, BT_S, BF_S, BRA, BSR)>;
		def : InstRW<[SH4WriteControl3], (instrs JMP, BRAF, JSR, BSRF, RTS)>;
		def : InstRW<[SH4WriteRTE5], (instrs RTE)>;
		def : InstRW<[SH4WriteMul4], (instrs
			MUL_L, MULS_W, MULU_W, DMULS_L, DMULU_L, MAC_L, MAC_W)>;
		def : InstRW<[SH4WriteCO3], (instrs
			LDC_GBR, LDC_VBR, LDC_SSR, LDC_SPC, LDC_DBR,
			LDS_PR, LDS_MACH, LDS_MACL,
			STC_SGR, STC_DBR)>;
		def : InstRW<[SH4WriteCO4], (instrs LDC_SR)>;
		def : InstRW<[SH4WriteTrap7], (instrs TRAPA)>;
	}
}

defm : ST40InstrSchedule<ST40100Model>;
defm : ST40InstrSchedule<ST40200Model>;

let SchedModel = ST40300Model in {
	def : InstRW<[ST40300WriteMove], (instrs MOVrr, NOP)>;
	def : InstRW<[ST40300WriteALU], (instrs
		TST, TSTri, CMP_EQri, CMP_PL, CMP_PZ, CMP_STR,
		CMP_EQ, CMP_HS, CMP_GE, CMP_HI, CMP_GT,
		ADDrr, ADDri, SUBrr, NEG, ANDrr, ORrr, XORrr, NOT,
		ANDri, ORri, XORri, MOVri, MOVA, DT,
		SHLL, SHLR, SHAR, SHAL, SHLL2, SHLR2, SHLL8, SHLR8,
		SHLL16, SHLR16, ROTCL, ROTCR, ROTL, ROTR, SHAD, SHLD)>;
	def : InstRW<[ST40300WriteLoad], (instrs
		MOVB_load_reg, MOVW_load_reg, MOVL_load_reg,
		MOVB_load_postinc, MOVW_load_postinc, MOVL_load_postinc,
		MOVB_load_disp, MOVW_load_disp, MOVL_load_disp,
		MOVB_load_indexed, MOVW_load_indexed, MOVL_load_indexed,
		MOVW_load_pc, MOVL_load_pc,
		MOVB_load_gbr, MOVW_load_gbr, MOVL_load_gbr)>;
	def : InstRW<[ST40300WriteStore], (instrs
		MOVB_store_reg, MOVW_store_reg, MOVL_store_reg,
		MOVB_store_predec, MOVW_store_predec, MOVL_store_predec,
		MOVB_store_disp, MOVW_store_disp, MOVL_store_disp,
		MOVB_store_indexed, MOVW_store_indexed, MOVL_store_indexed,
		MOVB_store_gbr, MOVW_store_gbr, MOVL_store_gbr, PREF)>;
	def : InstRW<[ST40300WriteBranch], (instrs BT, BF, BT_S, BF_S, BRA, BSR)>;
	def : InstRW<[ST40300WriteControl], (instrs JMP, BRAF, JSR, BSRF, RTS)>;
	def : InstRW<[ST40300WriteRTE], (instrs RTE)>;
	def : InstRW<[ST40300WriteMul], (instrs MUL_L, MULS_W, MULU_W)>;
	def : InstRW<[ST40300WriteDMul], (instrs DMULS_L, DMULU_L)>;
	def : InstRW<[ST40300WriteMACW], (instrs MAC_W)>;
	def : InstRW<[ST40300WriteMACL], (instrs MAC_L)>;
	def : InstRW<[ST40300WriteICBI], (instrs ICBI)>;
	def : InstRW<[ST40300WriteSYNCO], (instrs SYNCO)>;

	def : InstRW<[ST40300WriteFPT], (instrs
		FMOV_FR, FMOV_DR_DR, FMOV_DR_XD, FMOV_XD_DR, FMOV_XD_XD,
		FLDI0, FLDI1, FLDS, FSTS, FABS_FR, FNEG_FR, FABS_DR, FNEG_DR,
		LDS_FPUL, STS_FPUL)>;
	def : InstRW<[ST40300WriteFPLoad], (instrs
		FMOV_S_LOAD, FMOV_S_LOAD_POSTINC, FMOV_S_LOAD_INDEXED,
		FMOV_S_LOAD_ALIAS, FMOV_S_LOAD_POSTINC_ALIAS,
		FMOV_S_LOAD_INDEXED_ALIAS,
		FMOV_D_LOAD_DR, FMOV_D_LOAD_POSTINC_DR, FMOV_D_LOAD_INDEXED_DR,
		FMOV_D_LOAD_XD, FMOV_D_LOAD_POSTINC_XD, FMOV_D_LOAD_INDEXED_XD)>;
	def : InstRW<[ST40300WriteFPStore], (instrs
		FMOV_S_STORE, FMOV_S_STORE_PREDEC, FMOV_S_STORE_INDEXED,
		FMOV_S_STORE_ALIAS, FMOV_S_STORE_PREDEC_ALIAS,
		FMOV_S_STORE_INDEXED_ALIAS,
		FMOV_D_STORE_DR, FMOV_D_STORE_PREDEC_DR, FMOV_D_STORE_INDEXED_DR,
		FMOV_D_STORE_XD, FMOV_D_STORE_PREDEC_XD, FMOV_D_STORE_INDEXED_XD)>;
	def : InstRW<[ST40300WriteFPAdd], (instrs
		FADD_FR, FSUB_FR, FMAC_FR, FADD_DR, FSUB_DR)>;
	def : InstRW<[ST40300WriteFPCmp], (instrs
		FCMP_EQ_FR, FCMP_GT_FR, FCMP_EQ_DR, FCMP_GT_DR)>;
	def : InstRW<[ST40300WriteFPMul], (instrs FMUL_FR)>;
	def : InstRW<[ST40300WriteFPDiv], (instrs FDIV_FR, FSQRT_FR)>;
	def : InstRW<[ST40300WriteFPMulD], (instrs FMUL_DR)>;
	def : InstRW<[ST40300WriteFPDivD], (instrs FDIV_DR, FSQRT_DR)>;
	def : InstRW<[ST40300WriteFPVector], (instrs FIPR)>;
	def : InstRW<[ST40300WriteFPMatrix], (instrs FTRV)>;
	def : InstRW<[ST40300WriteFPConvert], (instrs
		FLOAT_FR, FTRC_FR, FLOAT_DR, FTRC_DR, FCNVDS, FCNVSD)>;
	def : InstRW<[ST40300WriteFPChange], (instrs FPCHG, FRCHG, FSCHG)>;
}
