//===-- SHDSPInstrInfo.td - SH4AL-DSP instructions -------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class SHDSPRegToControl<bits<8> Suffix, Register Control, string Name>
	: SHRegToSpecial<Suffix, (outs), (ins GPR:$src), !strconcat("ldc\t$src,", Name)> {
	let Defs = [Control];
	let SchedRW = [SH4WriteLS];
	let Predicates = [HasDSP];
	let DecoderNamespace = "SHDSP";
}

class SHDSPControlToReg<bits<8> Suffix, Register Control, string Name>
	: SHSpecialToReg<Suffix, (outs GPR:$dst), (ins), !strconcat("stc\t", Name, ",$dst")> {
	let Uses = [Control];
	let SchedRW = [SH4WriteLS];
	let Predicates = [HasDSP];
	let DecoderNamespace = "SHDSP";
}

class SHDSPControlLoad<bits<4> Special, Register Control, string Name>
	: SHRegStack<Special, 0x7, (outs postincgpr:$base_wb), (ins GPR:$base),
		!strconcat("ldc.l\t$base_wb,", Name)> {
	let Defs = [Control];
	let Constraints = "$base_wb = $base";
	let mayLoad = 1;
	let SchedRW = [SH4WriteLoad];
	let Predicates = [HasDSP];
	let DecoderNamespace = "SHDSP";
}

class SHDSPControlStore<bits<4> Special, Register Control, string Name>
	: SHRegStack<Special, 0x3, (outs predecgpr:$base_wb), (ins GPR:$base),
		!strconcat("stc.l\t", Name, ",$base_wb")> {
	let Uses = [Control];
	let Constraints = "$base_wb = $base";
	let mayStore = 1;
	let SchedRW = [SH4WriteStore];
	let Predicates = [HasDSP];
	let DecoderNamespace = "SHDSP";
}

class SHDSPRegToData<bits<8> Suffix, Register Data, string Name>
	: SHRegToSpecial<Suffix, (outs), (ins GPR:$src), !strconcat("lds\t$src,", Name)> {
	let Defs = [Data];
	let SchedRW = [SH4WriteLS];
	let Predicates = [HasDSP];
	let DecoderNamespace = "SHDSP";
}

class SHDSPDataToReg<bits<8> Suffix, Register Data, string Name>
	: SHSpecialToReg<Suffix, (outs GPR:$dst), (ins), !strconcat("sts\t", Name, ",$dst")> {
	let Uses = [Data];
	let SchedRW = [SH4WriteLS];
	let Predicates = [HasDSP];
	let DecoderNamespace = "SHDSP";
}

class SHDSPDataLoad<bits<4> Special, Register Data, string Name>
	: SHRegStack<Special, 0x6, (outs postincgpr:$base_wb), (ins GPR:$base),
		!strconcat("lds.l\t$base_wb,", Name)> {
	let Defs = [Data];
	let Constraints = "$base_wb = $base";
	let mayLoad = 1;
	let SchedRW = [SH4WriteLoad];
	let Predicates = [HasDSP];
	let DecoderNamespace = "SHDSP";
}

class SHDSPDataStore<bits<4> Special, Register Data, string Name>
	: SHRegStack<Special, 0x2, (outs predecgpr:$base_wb), (ins GPR:$base),
		!strconcat("sts.l\t", Name, ",$base_wb")> {
	let Uses = [Data];
	let Constraints = "$base_wb = $base";
	let mayStore = 1;
	let SchedRW = [SH4WriteStore];
	let Predicates = [HasDSP];
	let DecoderNamespace = "SHDSP";
}

let Predicates = [HasSH4ALDSP] in
def CLRMXY : SHFixed<0x0088, "clrdmxy"> {
	let Defs = [SR];
	let Uses = [SR];
	let SchedRW = [SH4WriteEX];
	let DecoderNamespace = "SHDSP";
}

def LDC_MOD : SHDSPRegToControl<0x5e, MOD, "mod">;
def LDC_RE : SHDSPRegToControl<0x7e, RE, "re">;
def LDC_RS : SHDSPRegToControl<0x6e, RS, "rs">;
def LDC_L_MOD : SHDSPControlLoad<0x5, MOD, "mod">;
def LDC_L_RE : SHDSPControlLoad<0x7, RE, "re">;
def LDC_L_RS : SHDSPControlLoad<0x6, RS, "rs">;

let Predicates = [HasDSP] in {
	def LDRC_REG : SHOneReg<0x34, (outs), (ins GPR:$dst), "ldrc\t$dst"> {
		let Defs = [SR, RE];
		let Uses = [SR, RS, RE];
		let DecoderNamespace = "SHDSP";
	}
	def LDRC_IMM : SHBranch8<0x8a, (ins uimm8:$target), "ldrc\t$target"> {
		let Defs = [SR, RE];
		let Uses = [SR, RS, RE];
		let DecoderNamespace = "SHDSP";
	}
	def LDRE : SHBranch8<0x8e, (ins pcwordliteral:$target), "ldre\t$target"> {
		let Defs = [RE];
		let IsDelaySlotIllegal = 1;
		let SchedRW = [SH4WriteLS];
		let DecoderNamespace = "SHDSP";
	}
	def LDRS : SHBranch8<0x8c, (ins pcwordliteral:$target), "ldrs\t$target"> {
		let Defs = [RS];
		let IsDelaySlotIllegal = 1;
		let SchedRW = [SH4WriteLS];
		let DecoderNamespace = "SHDSP";
	}
	def DSP_LOOP_START : SHInst<(outs), (ins dsploopbounds:$bounds), ""> {
		bits<8> bounds;
		let Inst{15-8} = 0x8c;
		let Inst{7-0} = bounds;
		let Defs = [RS];
		let IsDelaySlotIllegal = 1;
		let isAsmParserOnly = 1;
		let isCodeGenOnly = 1;
	}
	def DSP_LOOP_END : SHInst<(outs), (ins dsploopbounds:$bounds), ""> {
		bits<8> bounds;
		let Inst{15-8} = 0x8e;
		let Inst{7-0} = bounds;
		let Defs = [RE];
		let IsDelaySlotIllegal = 1;
		let isAsmParserOnly = 1;
		let isCodeGenOnly = 1;
	}
}

def LDS_DSR : SHDSPRegToData<0x6a, DSR, "dsr">;
def LDS_A0 : SHDSPRegToData<0x7a, A0, "a0">;
def LDS_X0 : SHDSPRegToData<0x8a, X0, "x0">;
def LDS_X1 : SHDSPRegToData<0x9a, X1, "x1">;
def LDS_Y0 : SHDSPRegToData<0xaa, Y0, "y0">;
def LDS_Y1 : SHDSPRegToData<0xba, Y1, "y1">;
def LDS_L_DSR : SHDSPDataLoad<0x6, DSR, "dsr">;
def LDS_L_A0 : SHDSPDataLoad<0x7, A0, "a0">;
def LDS_L_X0 : SHDSPDataLoad<0x8, X0, "x0">;
def LDS_L_X1 : SHDSPDataLoad<0x9, X1, "x1">;
def LDS_L_Y0 : SHDSPDataLoad<0xa, Y0, "y0">;
def LDS_L_Y1 : SHDSPDataLoad<0xb, Y1, "y1">;

let Predicates = [HasDSP] in {
	def SETDMX : SHFixed<0x0098, "setdmx"> {
		let Defs = [SR];
		let Uses = [SR];
		let SchedRW = [SH4WriteEX];
		let DecoderNamespace = "SHDSP";
	}
	def SETDMY : SHFixed<0x00c8, "setdmy"> {
		let Defs = [SR];
		let Uses = [SR];
		let SchedRW = [SH4WriteEX];
		let DecoderNamespace = "SHDSP";
	}
	def SETRC_REG : SHOneReg<0x14, (outs), (ins GPR:$dst), "setrc\t$dst"> {
		let Defs = [SR];
		let Uses = [SR, RS, RE];
		let DecoderNamespace = "SHDSP";
	}
	def SETRC_IMM : SHBranch8<0x82, (ins uimm8:$target), "setrc\t$target"> {
		let Defs = [SR];
		let Uses = [SR, RS, RE];
		let DecoderNamespace = "SHDSP";
	}
}

def STC_MOD : SHDSPControlToReg<0x52, MOD, "mod">;
def STC_RE : SHDSPControlToReg<0x72, RE, "re">;
def STC_RS : SHDSPControlToReg<0x62, RS, "rs">;
def STC_L_MOD : SHDSPControlStore<0x5, MOD, "mod">;
def STC_L_RE : SHDSPControlStore<0x7, RE, "re">;
def STC_L_RS : SHDSPControlStore<0x6, RS, "rs">;
def STS_DSR : SHDSPDataToReg<0x6a, DSR, "dsr">;
def STS_A0 : SHDSPDataToReg<0x7a, A0, "a0">;
def STS_X0 : SHDSPDataToReg<0x8a, X0, "x0">;
def STS_X1 : SHDSPDataToReg<0x9a, X1, "x1">;
def STS_Y0 : SHDSPDataToReg<0xaa, Y0, "y0">;
def STS_Y1 : SHDSPDataToReg<0xba, Y1, "y1">;
def STS_L_DSR : SHDSPDataStore<0x6, DSR, "dsr">;
def STS_L_A0 : SHDSPDataStore<0x7, A0, "a0">;
def STS_L_X0 : SHDSPDataStore<0x8, X0, "x0">;
def STS_L_X1 : SHDSPDataStore<0x9, X1, "x1">;
def STS_L_Y0 : SHDSPDataStore<0xa, Y0, "y0">;
def STS_L_Y1 : SHDSPDataStore<0xb, Y1, "y1">;

let Predicates = [HasDSP], hasSideEffects = 1,
	mayLoad = 1, mayStore = 1,
	Uses = [R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, A0, A1, X0, X1,
		Y0, Y1, M0, M1, SR, DSR, RS, RE, MOD, MACH, MACL],
	Defs = [R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, A0, A1, X0, X1,
		Y0, Y1, M0, M1, DSR, RS, RE, MOD, MACH, MACL] in {
	def DSP_DDT : SHDSPTransferInst<(outs), (ins dspencoding:$encoding),
		"dsp.ddt\t$encoding"> {
		let DecoderMethod = "decodeSHDSPDDT";
		let DecoderNamespace = "SHDSP";
	}
	def DSP_SDT : SHDSPSingleTransferInst<(outs), (ins dspencoding:$encoding),
		"dsp.sdt\t$encoding"> {
		let DecoderMethod = "decodeSHDSPSDT";
		let DecoderNamespace = "SHDSP";
	}
}

let Predicates = [HasDSP], hasSideEffects = 1,
	mayLoad = 1, mayStore = 1,
	Uses = [R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, A0, A1, X0, X1,
		Y0, Y1, M0, M1, SR, DSR, RS, RE, MOD, MACH, MACL, TBit],
	Defs = [R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, A0, A1, X0, X1,
		Y0, Y1, M0, M1, DSR, RS, RE, MOD, MACH, MACL, TBit] in
def DSP_PPI : SHDSPParallelInst<(outs), (ins dspencoding:$encoding),
	"dsp.ppi\t$encoding"> {
	let IsDelaySlotIllegal = 0;
	let DecoderMethod = "decodeSHDSPPPI";
	let DecoderNamespace = "SHDSP";
}

let SchedModel = SH4ALDSPModel in {
	def : InstRW<[SH4ALDSPWriteControl2],
		(instrs LDRC_REG, LDRC_IMM, SETRC_REG, SETRC_IMM)>;
	def : InstRW<[SH4ALDSPWriteTransfer], (instrs DSP_DDT, DSP_SDT)>;
	def : InstRW<[SH4ALDSPWriteOperation], (instrs DSP_PPI)>;
}
