//===-- SHFPUInstrInfo.td - SuperH FPU instruction definitions ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Scalar CodeGen uses the canonical primary FR bank while the
// public calling convention continues to transport floating values as GPR and
// stack words.  Alternate-bank and vector/matrix instructions remain MC-only.

let Predicates = [HasSingleFPU], Uses = [FPSCR], HasFPURequirement = 1,
	FPURequirement = SHFPUReqScalarMove in {
	let SchedRW = [SH4WriteLS] in
	def FMOV_FR : SHFRegReg<0xc, (outs FR:$dst), (ins FR:$src),
		"fmov\t$src,$dst">;

	let mayLoad = 1, SchedRW = [SH4WriteLoad] in
	def FMOV_S_LOAD : SHFLoad<0x8, (outs FR:$dst), (ins longmemreg:$addr),
		"fmov\t$addr,$dst">;

	let mayStore = 1, SchedRW = [SH4WriteStore] in
	def FMOV_S_STORE : SHFStore<0xa, (outs),
		(ins FR:$src, longmemreg:$addr), "fmov\t$src,$addr">;

	let mayLoad = 1, Constraints = "$base_wb = $base",
		SchedRW = [SH4WriteLoad] in
	def FMOV_S_LOAD_POSTINC : SHFLoadPostInc<
		(outs FR:$dst, postincgpr:$base_wb), (ins GPR:$base),
		"fmov\t$base_wb,$dst">;

	let mayStore = 1, Constraints = "$base_wb = $base",
		SchedRW = [SH4WriteStore] in
	def FMOV_S_STORE_PREDEC : SHFStorePreDec<(outs predecgpr:$base_wb),
		(ins FR:$src, GPR:$base), "fmov\t$src,$base_wb">;

	let mayLoad = 1, Uses = [R0, FPSCR], SchedRW = [SH4WriteLoad] in
	def FMOV_S_LOAD_INDEXED : SHFLoad<0x6, (outs FR:$dst),
		(ins indexedmem:$addr), "fmov\t$addr,$dst">;

	let mayStore = 1, Uses = [R0, FPSCR], SchedRW = [SH4WriteStore] in
	def FMOV_S_STORE_INDEXED : SHFStore<0x7, (outs),
		(ins FR:$src, indexedmem:$addr), "fmov\t$src,$addr">;
}

let Predicates = [HasSingleFPU], Uses = [FPSCR], isAsmParserOnly = 1,
	HasFPURequirement = 1 in {
	let mayLoad = 1, SchedRW = [SH4WriteLoad] in
	def FMOV_S_LOAD_ALIAS : SHFLoad<0x8, (outs FR:$dst),
		(ins longmemreg:$addr), "fmov.s\t$addr,$dst">;

	let mayStore = 1, SchedRW = [SH4WriteStore] in
	def FMOV_S_STORE_ALIAS : SHFStore<0xa, (outs),
		(ins FR:$src, longmemreg:$addr), "fmov.s\t$src,$addr">;

	let mayLoad = 1, Constraints = "$base_wb = $base",
		SchedRW = [SH4WriteLoad] in
	def FMOV_S_LOAD_POSTINC_ALIAS : SHFLoadPostInc<
		(outs FR:$dst, postincgpr:$base_wb), (ins GPR:$base),
		"fmov.s\t$base_wb,$dst">;

	let mayStore = 1, Constraints = "$base_wb = $base",
		SchedRW = [SH4WriteStore] in
	def FMOV_S_STORE_PREDEC_ALIAS : SHFStorePreDec<
		(outs predecgpr:$base_wb), (ins FR:$src, GPR:$base),
		"fmov.s\t$src,$base_wb">;

	let mayLoad = 1, Uses = [R0, FPSCR], SchedRW = [SH4WriteLoad] in
	def FMOV_S_LOAD_INDEXED_ALIAS : SHFLoad<0x6, (outs FR:$dst),
		(ins indexedmem:$addr), "fmov.s\t$addr,$dst">;

	let mayStore = 1, Uses = [R0, FPSCR], SchedRW = [SH4WriteStore] in
	def FMOV_S_STORE_INDEXED_ALIAS : SHFStore<0x7, (outs),
		(ins FR:$src, indexedmem:$addr), "fmov.s\t$src,$addr">;
}

let Predicates = [HasFPUSizeMode], Uses = [FPSCR], isAsmParserOnly = 1,
	HasFPURequirement = 1, FPURequirement = SHFPUReqDouble in {
	let SchedRW = [SH4WriteLS] in
	def FMOV_DR_DR : SHDRegReg<0, 0, 0xc, (outs DR:$dst), (ins DR:$src),
		"fmov\t$src,$dst">;

	let mayLoad = 1, SchedRW = [SH4WriteLoad] in
	def FMOV_D_LOAD_DR : SHDLoad<0, 0x8, (outs DR:$dst),
		(ins longmemreg:$addr), "fmov.d\t$addr,$dst">;

	let mayStore = 1, SchedRW = [SH4WriteStore] in
	def FMOV_D_STORE_DR : SHDStore<0, 0xa, (outs),
		(ins DR:$src, longmemreg:$addr), "fmov.d\t$src,$addr">;

	let mayLoad = 1, Constraints = "$base_wb = $base",
		SchedRW = [SH4WriteLoad] in
	def FMOV_D_LOAD_POSTINC_DR : SHDLoadPostInc<0,
		(outs DR:$dst, postincgpr:$base_wb), (ins GPR:$base),
		"fmov.d\t$base_wb,$dst">;

	let mayStore = 1, Constraints = "$base_wb = $base",
		SchedRW = [SH4WriteStore] in
	def FMOV_D_STORE_PREDEC_DR : SHDStorePreDec<0,
		(outs predecgpr:$base_wb), (ins DR:$src, GPR:$base),
		"fmov.d\t$src,$base_wb">;

	let mayLoad = 1, Uses = [R0, FPSCR], SchedRW = [SH4WriteLoad] in
	def FMOV_D_LOAD_INDEXED_DR : SHDLoad<0, 0x6, (outs DR:$dst),
		(ins indexedmem:$addr), "fmov.d\t$addr,$dst">;

	let mayStore = 1, Uses = [R0, FPSCR], SchedRW = [SH4WriteStore] in
	def FMOV_D_STORE_INDEXED_DR : SHDStore<0, 0x7, (outs),
		(ins DR:$src, indexedmem:$addr), "fmov.d\t$src,$addr">;
}

let Predicates = [HasFPUSizeMode, HasFPURegisterBank], Uses = [FPSCR],
	isAsmParserOnly = 1, HasFPURequirement = 1 in {
	let SchedRW = [SH4WriteLS] in {
	def FMOV_DR_XD : SHDRegReg<1, 0, 0xc, (outs XD:$dst), (ins DR:$src),
		"fmov\t$src,$dst">;
	def FMOV_XD_DR : SHDRegReg<0, 1, 0xc, (outs DR:$dst), (ins XD:$src),
		"fmov\t$src,$dst">;
	def FMOV_XD_XD : SHDRegReg<1, 1, 0xc, (outs XD:$dst), (ins XD:$src),
		"fmov\t$src,$dst">;
	}

	let mayLoad = 1, SchedRW = [SH4WriteLoad] in
	def FMOV_D_LOAD_XD : SHDLoad<1, 0x8, (outs XD:$dst),
		(ins longmemreg:$addr), "fmov.d\t$addr,$dst">;

	let mayStore = 1, SchedRW = [SH4WriteStore] in
	def FMOV_D_STORE_XD : SHDStore<1, 0xa, (outs),
		(ins XD:$src, longmemreg:$addr), "fmov.d\t$src,$addr">;

	let mayLoad = 1, Constraints = "$base_wb = $base",
		SchedRW = [SH4WriteLoad] in
	def FMOV_D_LOAD_POSTINC_XD : SHDLoadPostInc<1,
		(outs XD:$dst, postincgpr:$base_wb), (ins GPR:$base),
		"fmov.d\t$base_wb,$dst">;

	let mayStore = 1, Constraints = "$base_wb = $base",
		SchedRW = [SH4WriteStore] in
	def FMOV_D_STORE_PREDEC_XD : SHDStorePreDec<1,
		(outs predecgpr:$base_wb), (ins XD:$src, GPR:$base),
		"fmov.d\t$src,$base_wb">;

	let mayLoad = 1, Uses = [R0, FPSCR], SchedRW = [SH4WriteLoad] in
	def FMOV_D_LOAD_INDEXED_XD : SHDLoad<1, 0x6, (outs XD:$dst),
		(ins indexedmem:$addr), "fmov.d\t$addr,$dst">;

	let mayStore = 1, Uses = [R0, FPSCR], SchedRW = [SH4WriteStore] in
	def FMOV_D_STORE_INDEXED_XD : SHDStore<1, 0x7, (outs),
		(ins XD:$src, indexedmem:$addr), "fmov.d\t$src,$addr">;
}

let Predicates = [HasSingleFPU], Uses = [FPSCR], SchedRW = [SH4WriteFE3],
	HasFPURequirement = 1, FPURequirement = SHFPUReqSingle in {
	let SchedRW = [SH4WriteLS] in {
	def FLDI0 : SHFOneReg<0x8d, (outs FR:$dst), (ins), "fldi0\t$dst">;
	def FLDI1 : SHFOneReg<0x9d, (outs FR:$dst), (ins), "fldi1\t$dst">;
	}

	let Defs = [FPUL], SchedRW = [SH4WriteLS], FPURequirement = SHFPUReqPrimaryBank in
	def FLDS : SHFOneReg<0x1d, (outs), (ins FR:$dst), "flds\t$dst,fpul">;

	let Uses = [FPUL, FPSCR], SchedRW = [SH4WriteLS], FPURequirement = SHFPUReqPrimaryBank in
	def FSTS : SHFOneReg<0x0d, (outs FR:$dst), (ins), "fsts\tfpul,$dst">;

	// E-class FABS/FNEG can clear cause bits and report invalid operands.
	// Treat these as possible status effects across the supported profiles.
	// Status updates read sticky flags and overwrite cause bits. Model the
	// complete register so full FPSCR reads/writes, including expanded mode
	// transitions, remain ordered with them through scheduling and delay filling.
	// A status definition does not invalidate SHFPUState's mode knowledge.
	let MayModifyFPSCRStatus = 1, Defs = [FPSCR] in {
	let Constraints = "$dst = $dst_in", SchedRW = [SH4WriteLS],
		FPURequirement = SHFPUReqPrimaryBank in {
		def FABS_FR : SHFOneReg<0x5d, (outs FR:$dst), (ins FR:$dst_in),
			"fabs\t$dst">;
		def FNEG_FR : SHFOneReg<0x4d, (outs FR:$dst), (ins FR:$dst_in),
			"fneg\t$dst">;
	}

	let Constraints = "$dst = $dst_in", isCommutable = 1 in
	def FADD_FR : SHFRegReg<0x0, (outs FR:$dst),
		(ins FR:$dst_in, FR:$src), "fadd\t$src,$dst">;

	let Constraints = "$dst = $dst_in" in
	def FSUB_FR : SHFRegReg<0x1, (outs FR:$dst),
		(ins FR:$dst_in, FR:$src), "fsub\t$src,$dst">;

	let Constraints = "$dst = $dst_in", isCommutable = 1,
		SchedRW = [SH4WriteSingleMul] in
	def FMUL_FR : SHFRegReg<0x2, (outs FR:$dst),
		(ins FR:$dst_in, FR:$src), "fmul\t$src,$dst">;

	let Constraints = "$dst = $dst_in", Uses = [FR0, FPSCR],
		FPURequirement = SHFPUReqNone in
	def FMAC_FR : SHFRegReg<0xe, (outs FR:$dst),
		(ins FR:$dst_in, FR:$src), "fmac\tfr0,$src,$dst">;

	let Constraints = "$dst = $addend", Defs = [FR0, FPSCR] in
	def FMAC_FR_PSEUDO : SHPseudo<(outs FRNoFR0:$dst),
		(ins FRNoFR0:$addend, FR:$mul_lhs, FR:$mul_rhs)>;

	let Constraints = "$dst = $dst_in", SchedRW = [SH4WriteFE12] in
	def FDIV_FR : SHFRegReg<0x3, (outs FR:$dst),
		(ins FR:$dst_in, FR:$src), "fdiv\t$src,$dst">;

	let Defs = [TBit, FPSCR] in {
		def FCMP_EQ_FR : SHFRegReg<0x4, (outs), (ins FR:$src, FR:$dst),
			"fcmp/eq\t$src,$dst">;
		def FCMP_GT_FR : SHFRegReg<0x5, (outs), (ins FR:$src, FR:$dst),
			"fcmp/gt\t$src,$dst">;
	}

	let Uses = [FPUL, FPSCR] in
	def FLOAT_FR : SHFOneReg<0x2d, (outs FR:$dst), (ins),
		"float\tfpul,$dst">;

	let Defs = [FPUL, FPSCR] in
	def FTRC_FR : SHFOneReg<0x3d, (outs), (ins FR:$dst),
		"ftrc\t$dst,fpul">;
	}
}

let Predicates = [HasFSQRT], Uses = [FPSCR], Defs = [FPSCR], MayModifyFPSCRStatus = 1,
	Constraints = "$dst = $dst_in", SchedRW = [SH4WriteFE12],
	HasFPURequirement = 1, FPURequirement = SHFPUReqSingle in
def FSQRT_FR : SHFOneReg<0x6d, (outs FR:$dst), (ins FR:$dst_in),
	"fsqrt\t$dst">;

let Predicates = [HasFPUVector], Uses = [FPSCR], Defs = [FPSCR], SchedRW = [SH4WriteFE4],
	MayModifyFPSCRStatus = 1,
	HasFPURequirement = 1, FPURequirement = SHFPUReqSingleArithmetic in {
	let Constraints = "$dst = $dst_in" in
	def FIPR : SHFVRegReg<(outs FV:$dst), (ins FV:$dst_in, FV:$src),
		"fipr\t$src,$dst">;

	let Constraints = "$dst = $dst_in", Uses = [XMTRX, FPSCR] in
	def FTRV : SHFTRV<(outs FV:$dst),
		(ins FV:$dst_in), "ftrv\txmtrx,$dst"> {
		let SchedRW = [SH4WriteFE8];
	}
}

let Predicates = [HasDoubleFPU], Uses = [FPSCR], Defs = [FPSCR], HasFPURequirement = 1,
	FPURequirement = SHFPUReqDoubleArithmetic, MayModifyFPSCRStatus = 1 in {
	let Constraints = "$dst = $dst_in", SchedRW = [SH4WriteLS],
		isAsmParserOnly = 1 in {
		let MayModifyFPSCRStatus = 0 in
		def FABS_DR : SHDOneReg<0x5d, (outs DR:$dst), (ins DR:$dst_in),
			"fabs\t$dst"> {
			let Defs = [];
		}
		let MayModifyFPSCRStatus = 0 in
		def FNEG_DR : SHDOneReg<0x4d, (outs DR:$dst), (ins DR:$dst_in),
			"fneg\t$dst"> {
			let Defs = [];
		}
		def FSQRT_DR : SHDOneReg<0x6d, (outs DR:$dst), (ins DR:$dst_in),
			"fsqrt\t$dst"> {
			let Predicates = [HasDoubleFPU, HasFSQRT];
			let SchedRW = [SH4WriteFE24];
		}
	}

	let Constraints = "$dst = $dst_in", SchedRW = [SH4WriteFE7],
		isAsmParserOnly = 1 in {
		def FADD_DR : SHDRegReg<0, 0, 0x0, (outs DR:$dst),
			(ins DR:$dst_in, DR:$src), "fadd\t$src,$dst">;
		def FSUB_DR : SHDRegReg<0, 0, 0x1, (outs DR:$dst),
			(ins DR:$dst_in, DR:$src), "fsub\t$src,$dst">;
	}

	let Constraints = "$dst = $dst_in", isCommutable = 1,
		SchedRW = [SH4WriteFE9], isAsmParserOnly = 1 in
	def FMUL_DR : SHDRegReg<0, 0, 0x2, (outs DR:$dst),
		(ins DR:$dst_in, DR:$src), "fmul\t$src,$dst">;

	let Constraints = "$dst = $dst_in", SchedRW = [SH4WriteFE24],
		isAsmParserOnly = 1 in
	def FDIV_DR : SHDRegReg<0, 0, 0x3, (outs DR:$dst),
		(ins DR:$dst_in, DR:$src), "fdiv\t$src,$dst">;

	let Defs = [TBit, FPSCR], SchedRW = [SH4WriteDoubleCmp5], isAsmParserOnly = 1 in {
		def FCMP_EQ_DR : SHDRegReg<0, 0, 0x4, (outs), (ins DR:$src, DR:$dst),
			"fcmp/eq\t$src,$dst">;
		def FCMP_GT_DR : SHDRegReg<0, 0, 0x5, (outs), (ins DR:$src, DR:$dst),
			"fcmp/gt\t$src,$dst">;
	}

	let Uses = [FPUL, FPSCR], SchedRW = [SH4WriteFE5],
		isAsmParserOnly = 1 in
	def FLOAT_DR : SHDOneReg<0x2d, (outs DR:$dst), (ins),
		"float\tfpul,$dst">;

	let Defs = [FPUL, FPSCR], SchedRW = [SH4WriteFE5] in {
		let isAsmParserOnly = 1 in
		def FTRC_DR : SHDOneReg<0x3d, (outs), (ins DR:$dst),
			"ftrc\t$dst,fpul">;
		def FCNVDS : SHDOneReg<0xbd, (outs), (ins DR:$dst),
			"fcnvds\t$dst,fpul">;
	}

	let Uses = [FPUL, FPSCR], SchedRW = [SH4WriteFE5] in
	def FCNVSD : SHDOneReg<0xad, (outs DR:$dst), (ins),
		"fcnvsd\tfpul,$dst">;
}

let Predicates = [HasSingleFPU], HasFPURequirement = 1 in {
	let Defs = [FPSCR], SchedRW = [SH4WriteCO4] in
	def LDS_FPSCR : SHRegToSpecial<0x6a, (outs), (ins GPR:$src),
		"lds\t$src,fpscr">;

	let Uses = [FPSCR], hasSideEffects = 1, SchedRW = [SH4WriteCO3] in
	def STS_FPSCR : SHSpecialToReg<0x6a, (outs GPR:$dst), (ins),
		"sts\tfpscr,$dst">;

	let mayLoad = 1, Defs = [FPSCR], Constraints = "$base_wb = $base",
		SchedRW = [SH4WriteCO3] in
	def LDS_L_FPSCR : SHRegStack<0x6, 0x6, (outs postincgpr:$base_wb),
		(ins GPR:$base), "lds.l\t$base_wb,fpscr">;

	let mayStore = 1, Uses = [FPSCR], Constraints = "$base_wb = $base",
		SchedRW = [SH4WriteCO] in
	def STS_L_FPSCR : SHRegStack<0x6, 0x2, (outs predecgpr:$base_wb),
		(ins GPR:$base), "sts.l\tfpscr,$base_wb">;

	let Defs = [FPUL], SchedRW = [SH4WriteStore] in
	def LDS_FPUL : SHRegToSpecial<0x5a, (outs), (ins GPR:$src),
		"lds\t$src,fpul">;

	let Uses = [FPUL], SchedRW = [SH4WriteLS3] in
	def STS_FPUL : SHSpecialToReg<0x5a, (outs GPR:$dst), (ins),
		"sts\tfpul,$dst">;

	let mayLoad = 1, Defs = [FPUL], Constraints = "$base_wb = $base",
		SchedRW = [SH4WriteLoad] in
	def LDS_L_FPUL : SHRegStack<0x5, 0x6, (outs postincgpr:$base_wb),
		(ins GPR:$base), "lds.l\t$base_wb,fpul">;

	let mayStore = 1, Uses = [FPUL], Constraints = "$base_wb = $base",
		SchedRW = [SH4WriteCO] in
	def STS_L_FPUL : SHRegStack<0x5, 0x2, (outs predecgpr:$base_wb),
		(ins GPR:$base), "sts.l\tfpul,$base_wb">;
}

let Predicates = [HasFPURegisterBank], Uses = [FPSCR], Defs = [FPSCR],
	hasSideEffects = 1, SchedRW = [SH4WriteFE4], HasFPURequirement = 1 in
def FRCHG : SHFixed<0xfbfd, "frchg">;

let Predicates = [HasFPUSizeMode], Uses = [FPSCR], Defs = [FPSCR],
	hasSideEffects = 1, SchedRW = [SH4WriteFE4], HasFPURequirement = 1 in
def FSCHG : SHFixed<0xf3fd, "fschg">;

let Predicates = [HasSH4ACommon, HasDoubleFPU], Uses = [FPSCR], Defs = [FPSCR],
	hasSideEffects = 1, SchedRW = [SH4WriteFE4], HasFPURequirement = 1 in
def FPCHG : SHFixed<0xf7fd, "fpchg">;

let Predicates = [HasSH4A, HasSingleFPU], Uses = [FPSCR], Defs = [FPSCR],
	HasFPURequirement = 1, FPURequirement = SHFPUReqSingleArithmetic,
	hasSideEffects = 1, MayModifyFPSCRStatus = 1 in {
	let SchedRW = [SH4WriteFE3], Constraints = "$dst = $dst_in" in
	def FSRRA : SHFOneReg<0x7d, (outs FR:$dst), (ins FR:$dst_in),
		"fsrra\t$dst">;

	let Uses = [FPUL, FPSCR], SchedRW = [SH4WriteFE3] in
	def FSCA : SHDOneReg<0xfd, (outs DR:$dst), (ins),
		"fsca\tfpul,$dst">;
}

// Code-generation-only transfers keep the public soft boundary bit-exact.
// The f64 nodes name their operands/results as architectural MSW then LSW;
// lowering maps those words to the endian-dependent public transport order.
let Predicates = [HasSingleFPU], Uses = [FPSCR], HasFPURequirement = 1 in {
	let usesCustomInserter = 1 in {
		let Defs = [TBit, FPUL, FPSCR], MayModifyFPSCRStatus = 1 in {
			def F32_TO_U32_PSEUDO : SHPseudo<(outs GPR:$dst), (ins FR:$src, FR:$threshold)> {
				let FPURequirement = SHFPUReqSingle;
				let Pattern = [(set GPR:$dst, (SHf32tou32 FR:$src, FR:$threshold))];
			}
			def F64_TO_U32_PSEUDO : SHPseudo<(outs GPR:$dst), (ins DR:$src, DR:$threshold)> {
				let Predicates = [HasDoubleFPU];
				let FPURequirement = SHFPUReqDoubleArithmetic;
				let Pattern = [(set GPR:$dst, (SHf64tou32 DR:$src, DR:$threshold))];
			}
		}
		def SELECT_F32_PSEUDO : SHPseudo<(outs FR:$dst), (ins GPR:$condition, FR:$true_value, FR:$false_value)> {
			let Pattern = [(set FR:$dst, (SHselectf32 GPR:$condition, FR:$true_value, FR:$false_value))];
		}
		def SELECT_F64_PSEUDO : SHPseudo<(outs DR:$dst), (ins GPR:$condition, DR:$true_value, DR:$false_value)> {
			let Predicates = [HasDoubleFPU];
			let Pattern = [(set DR:$dst, (SHselectf64 GPR:$condition, DR:$true_value, DR:$false_value))];
		}
	}

	let Defs = [FPUL], FPURequirement = SHFPUReqPrimaryBank in
	def GPR_TO_FR_PSEUDO : SHPseudo<(outs FR:$dst), (ins GPR:$src)> {
		let Pattern = [(set FR:$dst, (SHgprtofr GPR:$src))];
	}
	let Defs = [FPUL], FPURequirement = SHFPUReqPrimaryBank in
	def FR_TO_GPR_PSEUDO : SHPseudo<(outs GPR:$dst), (ins FR:$src)> {
		let Pattern = [(set GPR:$dst, (SHfrtogpr FR:$src))];
	}
	let Defs = [FPUL], FPURequirement = SHFPUReqPrimaryBank in
	def GPR_PAIR_TO_DR_PSEUDO : SHPseudo<(outs DR:$dst), (ins GPR:$msw, GPR:$lsw)> {
		let Predicates = [HasDoubleFPU];
		let Pattern = [(set DR:$dst, (SHgprpairtodr GPR:$msw, GPR:$lsw))];
	}
	let Defs = [FPUL], FPURequirement = SHFPUReqPrimaryBank in
	def DR_TO_GPR_PAIR_PSEUDO : SHPseudo<(outs GPR:$msw, GPR:$lsw), (ins DR:$src)> {
		let Predicates = [HasDoubleFPU];
		let Pattern = [(set GPR:$msw, GPR:$lsw, (SHdrtogprpair DR:$src))];
	}

	let MayModifyFPSCRStatus = 1 in {
	let Defs = [FPUL, FPSCR], FPURequirement = SHFPUReqSingle in
	def I32_TO_F32_PSEUDO : SHPseudo<(outs FR:$dst), (ins GPR:$src)> {
		let Pattern = [(set FR:$dst, (SHi32tof32 GPR:$src))];
	}
	let Defs = [FPUL, FPSCR], FPURequirement = SHFPUReqDoubleArithmetic in
	def I32_TO_F64_PSEUDO : SHPseudo<(outs DR:$dst), (ins GPR:$src)> {
		let Predicates = [HasDoubleFPU];
		let Pattern = [(set DR:$dst, (SHi32tof64 GPR:$src))];
	}
	let Defs = [FPUL, FPSCR], FPURequirement = SHFPUReqSingle in
	def F32_TO_I32_PSEUDO : SHPseudo<(outs GPR:$dst), (ins FR:$src)> {
		let Pattern = [(set GPR:$dst, (SHf32toi32 FR:$src))];
	}
	let Defs = [FPUL, FPSCR], FPURequirement = SHFPUReqDoubleArithmetic in
	def F64_TO_I32_PSEUDO : SHPseudo<(outs GPR:$dst), (ins DR:$src)> {
		let Predicates = [HasDoubleFPU];
		let Pattern = [(set GPR:$dst, (SHf64toi32 DR:$src))];
	}
	let Defs = [FPUL, FPSCR], FPURequirement = SHFPUReqDoubleArithmetic in
	def F32_TO_F64_PSEUDO : SHPseudo<(outs DR:$dst), (ins FR:$src)> {
		let Predicates = [HasDoubleFPU];
		let Pattern = [(set DR:$dst, (SHf32tof64 FR:$src))];
	}
	let Defs = [FPUL, FPSCR], FPURequirement = SHFPUReqDoubleArithmetic in
	def F64_TO_F32_PSEUDO : SHPseudo<(outs FR:$dst), (ins DR:$src)> {
		let Predicates = [HasDoubleFPU];
		let Pattern = [(set FR:$dst, (SHf64tof32 DR:$src))];
	}
	}

	let mayLoad = 1, FPURequirement = SHFPUReqScalarMove in
	def FMOV_S_LOAD_FI : SHPseudo<(outs FR:$dst), (ins longmemdisp:$addr)> {
		let Pattern = [(set FR:$dst, (load addr_fpu_frame:$addr))];
	}
	let mayLoad = 1, Defs = [R0], FPURequirement = SHFPUReqScalarMove in
	def FMOV_S_LOAD_PC_ISLAND : SHPseudo<(outs FR:$dst),
		(ins pcliteral:$target, island_instance:$island)> {
		let Pattern = [(set FR:$dst, (load tconstpool:$target))];
	}
	let mayLoad = 1, Defs = [R0], FPURequirement = SHFPUReqScalarMove in
	def FMOV_D_LOAD_PC_ISLAND : SHPseudo<(outs DR:$dst),
		(ins pcliteral:$target, island_instance:$island)> {
		let Predicates = [HasDoubleFPU];
		let Pattern = [(set DR:$dst, (load tconstpool:$target))];
	}
	let mayStore = 1, FPURequirement = SHFPUReqScalarMove in
	def FMOV_S_STORE_FI : SHPseudo<(outs), (ins FR:$src, longmemdisp:$addr)> {
		let Pattern = [(store FR:$src, addr_fpu_frame:$addr)];
	}
	let mayLoad = 1, FPURequirement = SHFPUReqScalarMove in
	def FMOV_D_LOAD_FI : SHPseudo<(outs DR:$dst), (ins longmemdisp:$addr)> {
		let Predicates = [HasDoubleFPU];
		let Pattern = [(set DR:$dst, (load addr_fpu_frame:$addr))];
	}
	let mayStore = 1, FPURequirement = SHFPUReqScalarMove in
	def FMOV_D_STORE_FI : SHPseudo<(outs), (ins DR:$src, longmemdisp:$addr)> {
		let Predicates = [HasDoubleFPU];
		let Pattern = [(store DR:$src, addr_fpu_frame:$addr)];
	}
	let mayLoad = 1, Defs = [R0], FPURequirement = SHFPUReqScalarMove in
	def FMOV_D_LOAD_SCALAR : SHPseudo<(outs DR:$dst), (ins GPRNoR0:$addr)> {
		let Predicates = [HasDoubleFPU];
		let Pattern = [(set DR:$dst, (f64 (load addr_fpu_reg:$addr)))];
	}
	let mayStore = 1, Defs = [R0], FPURequirement = SHFPUReqScalarMove in
	def FMOV_D_STORE_SCALAR : SHPseudo<(outs), (ins DR:$src, GPRNoR0:$addr)> {
		let Predicates = [HasDoubleFPU];
		let Pattern = [(store DR:$src, addr_fpu_reg:$addr)];
	}
}

// A dedicated machine pass inserts these requirements.  Expansion performs
// an FPSCR read-modify-write and changes only PR/SZ/FR.
let Predicates = [HasSingleFPU] in
def MATERIALIZE_FPSCR_MASK : SHPseudo<(outs GPR:$dst), (ins i32imm:$mask)>;

let Predicates = [HasSingleFPU], Uses = [FPSCR], Defs = [R0, R1, FPSCR],
	hasSideEffects = 1, HasFPURequirement = 1 in
def SET_FPSCR_MODE : SHPseudo<(outs), (ins i32imm:$mode)>;

// Read-modify-write preserves the *current* mode and replaces other fields.
// Two independent early clobbers prevent the input from overlapping either
// scratch while the complete operation remains visible to the mode owner.
let Predicates = [HasSingleFPU], Uses = [FPSCR], Defs = [FPSCR],
    hasSideEffects = 1, HasFPURequirement = 1, MayModifyFPSCRStatus = 1,
    Constraints = "@earlyclobber $scratch,@earlyclobber $mask" in
def SET_FPSCR_FIELDS : SHPseudo<(outs GPR:$scratch, GPR:$mask),
                                 (ins GPR:$value)>;

// Preserve absolute intent through allocation: spills can invalidate the
// pre-RA state that made a scratch-free FSCHG/FPCHG transition possible.
let Predicates = [HasSingleFPU], Uses = [FPSCR], Defs = [FPSCR],
    hasSideEffects = 1, HasFPURequirement = 1 in
def SET_FPSCR_MODE_TOGGLE : SHPseudo<(outs), (ins i32imm:$mode)>;

let Predicates = [HasSingleFPU], Uses = [FPSCR], Defs = [R0, R1, FPSCR],
	hasSideEffects = 1, HasFPURequirement = 1 in
def TRANSITION_FPSCR_MODE : SHPseudo<(outs), (ins i32imm:$mode, i32imm:$mask)>;

let Predicates = [HasSingleFPU], Uses = [FPSCR], Defs = [R0, FPSCR],
	hasSideEffects = 1, HasFPURequirement = 1 in
// Expansion reads FPSCR into R0 before using the mask, including at a dying
// mask use. A normal implicit R0 definition alone does not model that overlap.
// Keep the original mask bits as well as the allocated register, so the late
// state owner can verify its source-mode assumption after inserting spills.
def TRANSITION_FPSCR_MODE_REG : SHPseudo<(outs),
    (ins i32imm:$mode, GPRNoR0:$mask, i32imm:$maskbits)>;

let Predicates = [HasSingleFPU] in {
	def : Pat<(SHfconst0), (FLDI0)>;
	def : Pat<(SHfconst1), (FLDI1)>;
	def : Pat<(f32 (bitconvert GPR:$src)), (GPR_TO_FR_PSEUDO GPR:$src)>;
	def : Pat<(i32 (bitconvert FR:$src)), (FR_TO_GPR_PSEUDO FR:$src)>;

	def : Pat<(f32 (load addr_fpu_reg:$addr)), (FMOV_S_LOAD addr_fpu_reg:$addr)>;
	def : Pat<(store FR:$src, addr_fpu_reg:$addr), (FMOV_S_STORE FR:$src, addr_fpu_reg:$addr)>;
	let Predicates = [HasDoubleFPU] in {
		def : Pat<(f64 (load addr_fpu_reg:$addr)), (FMOV_D_LOAD_DR addr_fpu_reg:$addr)>;
		def : Pat<(store DR:$src, addr_fpu_reg:$addr), (FMOV_D_STORE_DR DR:$src, addr_fpu_reg:$addr)>;
	}

	def : Pat<(fadd FR:$lhs, FR:$rhs), (FADD_FR FR:$lhs, FR:$rhs)>;
	def : Pat<(fsub FR:$lhs, FR:$rhs), (FSUB_FR FR:$lhs, FR:$rhs)>;
	def : Pat<(fmul FR:$lhs, FR:$rhs), (FMUL_FR FR:$lhs, FR:$rhs)>;
	def : Pat<(any_fma FR:$mul_lhs, FR:$mul_rhs, FRNoFR0:$addend),
		(FMAC_FR_PSEUDO FRNoFR0:$addend, FR:$mul_lhs, FR:$mul_rhs)>;
	def : Pat<(fdiv FR:$lhs, FR:$rhs), (FDIV_FR FR:$lhs, FR:$rhs)>;
	def : Pat<(fneg FR:$src), (FNEG_FR FR:$src)>;
	def : Pat<(fabs FR:$src), (FABS_FR FR:$src)>;
	let Predicates = [HasFSQRT] in
	def : Pat<(fsqrt FR:$src), (FSQRT_FR FR:$src)>;

	let Predicates = [HasDoubleFPU] in {
		def : Pat<(fadd DR:$lhs, DR:$rhs), (FADD_DR DR:$lhs, DR:$rhs)>;
		def : Pat<(fsub DR:$lhs, DR:$rhs), (FSUB_DR DR:$lhs, DR:$rhs)>;
		def : Pat<(fmul DR:$lhs, DR:$rhs), (FMUL_DR DR:$lhs, DR:$rhs)>;
		def : Pat<(fdiv DR:$lhs, DR:$rhs), (FDIV_DR DR:$lhs, DR:$rhs)>;
		def : Pat<(fneg DR:$src), (FNEG_DR DR:$src)>;
		def : Pat<(fabs DR:$src), (FABS_DR DR:$src)>;
		let Predicates = [HasDoubleFPU, HasFSQRT] in
		def : Pat<(fsqrt DR:$src), (FSQRT_DR DR:$src)>;
	}

	def : Pat<(SHfcmpeq32 FR:$lhs, FR:$rhs), (FCMP_EQ_FR FR:$rhs, FR:$lhs)>;
	def : Pat<(SHfcmpgt32 FR:$lhs, FR:$rhs), (FCMP_GT_FR FR:$rhs, FR:$lhs)>;
	let Predicates = [HasDoubleFPU] in {
		def : Pat<(SHfcmpeq64 DR:$lhs, DR:$rhs), (FCMP_EQ_DR DR:$rhs, DR:$lhs)>;
		def : Pat<(SHfcmpgt64 DR:$lhs, DR:$rhs), (FCMP_GT_DR DR:$rhs, DR:$lhs)>;
	}
}

// Native short-vector transport. Scratch lifetimes are visible before RA.
let Predicates = [HasFPUVector], Uses = [FPSCR], HasFPURequirement = 1 in {
  let FPURequirement = SHFPUReqScalarMove in {
    let mayLoad = 1, Constraints = "$scratch = $addr" in
    def FMOV_V_LOAD : SHPseudo<(outs FV:$dst, GPR:$scratch), (ins GPR:$addr)>;
    let mayStore = 1, Constraints = "$scratch = $addr" in
    def FMOV_V_STORE : SHPseudo<(outs GPR:$scratch), (ins FV:$src, GPR:$addr)>;
    let mayLoad = 1 in
    def FMOV_V_LOAD_FI : SHPseudo<(outs FV:$dst), (ins longmemdisp:$addr)>;
    let mayStore = 1 in
    def FMOV_V_STORE_FI : SHPseudo<(outs), (ins FV:$src, longmemdisp:$addr)>;
  }
  let FPURequirement = SHFPUReqPairedSingle in {
    // Transfer a contiguous range of XD pairs. These internal operations are
    // used inside the bounded matrix owner, never as ambient source state.
    let Constraints = "$ptr_wb = $ptr", Uses = [XMTRX, FPSCR] in {
      let mayLoad = 1, Defs = [XMTRX] in
      def XMTRX_LOAD : SHPseudo<(outs GPR:$ptr_wb),
        (ins GPR:$ptr, i32imm:$first, i32imm:$count)>;
      let mayStore = 1 in
      def XMTRX_STORE : SHPseudo<(outs GPR:$ptr_wb),
        (ins GPR:$ptr, i32imm:$first, i32imm:$count)>;
    }
    let mayLoad = 1, Constraints = "$scratch = $addr" in
    def FMOV_V_LOAD_PAIR : SHPseudo<(outs FV:$dst, GPR:$scratch), (ins GPR:$addr)>;
    let mayStore = 1, Constraints = "$scratch = $addr" in
    def FMOV_V_STORE_PAIR : SHPseudo<(outs GPR:$scratch), (ins FV:$src, GPR:$addr)>;
    // Regions save and restore XD12/XD14 in the supplied private frame slot.
    // Every contained operation tracks the scoped XMTRX resource. No FRCHG.
    let Defs = [XMTRX, FPSCR], Uses = [XMTRX, FPSCR], mayLoad = 1, mayStore = 1,
        hasSideEffects = 1, MayModifyFPSCRStatus = 1 in {
      let Constraints = "$dst = $src,@earlyclobber $scratch,@earlyclobber $temp,@earlyclobber $word" in
      def FTRV_MEM : SHPseudo<(outs FV:$dst, GPR:$scratch, GPR:$temp, GPR:$word),
                             (ins FV:$src, GPR:$matrix, GPR:$save)>;
      // Regions expand before allocation. Their FV temporaries, loop-carried
      // pointers and preservation requirements remain ordinary LLVM values.
      let usesCustomInserter = 1 in {
        def FTRV4_MEM : SHPseudo<(outs),
            (ins GPR:$dst, GPR:$src, GPR:$matrix, GPR:$save)>;
        let Defs = [XMTRX, FPSCR, TBit] in
        def FTRV_ARRAY : SHPseudo<(outs),
            (ins GPR:$dst, GPR:$src, GPR:$count, GPR:$matrix, GPR:$save)>;
        let Defs = [XMTRX, FPSCR, TBit] in
        def FTRV_COMPOSE : SHPseudo<(outs),
            (ins GPR:$dst, GPR:$src, GPR:$count, GPR:$left, GPR:$right,
                 GPR:$save)>;
      }
      let Defs = [XMTRX], MayModifyFPSCRStatus = 0,
          Constraints = "@earlyclobber $scratch,@earlyclobber $temp,@earlyclobber $word" in
        def GRAPHICS_BEGIN : SHPseudo<(outs GPR:$scratch, GPR:$temp, GPR:$word),
                                      (ins GPR:$matrix, GPR:$save)>;
      let Defs = [XMTRX], MayModifyFPSCRStatus = 0,
          Constraints = "@earlyclobber $scratch" in
        def GRAPHICS_END : SHPseudo<(outs GPR:$scratch), (ins GPR:$save)>;
    }
    let Defs = [XMTRX], hasSideEffects = 1 in
      def XMTRX_FROM_FVS : SHPseudo<(outs),
          (ins FV:$c0, FV:$c1, FV:$c2, FV:$c3)>;
  }
  // Establish the scalar-transfer state in a counted loop's preheader. The
  // state owner handles the transition; this marker emits no instruction.
  let FPURequirement = SHFPUReqSingle, hasSideEffects = 1 in
    def GRAPHICS_SCALAR : SHPseudo<(outs), (ins)>;
}
let Predicates = [HasSH4A, HasSingleFPU], Uses = [FPSCR],
    Defs = [FPUL, FPSCR], HasFPURequirement = 1,
    FPURequirement = SHFPUReqSingleArithmetic, MayModifyFPSCRStatus = 1,
    hasSideEffects = 1 in
 def FSCA_VALUE : SHPseudo<(outs DR:$dst), (ins GPR:$phase)>;

// Keep the destructive FV lifetime local to the dot product. The independently
// allocated scalar result can feed packing or reductions without retaining four
// lanes. Extraction through FPUL is legal in paired-single transfer mode.
let Predicates = [HasFPUVector], Uses = [FPSCR], Defs = [FPUL, FPSCR],
    HasFPURequirement = 1, FPURequirement = SHFPUReqSingleArithmetic,
    MayModifyFPSCRStatus = 1, hasSideEffects = 1,
    Constraints = "$scratch = $dst_in", SchedRW = [SH4WriteFE4] in
def FIPR_VALUE : SHPseudo<(outs FR:$result, FV:$scratch),
                            (ins FV:$dst_in, FV:$src)>;

// Paired single transfers use architectural word order, including on LE.
// Keep their mode requirement distinct from the scalar f64 arithmetic path.
let Predicates = [HasFPUVector], Uses = [FPSCR], HasFPURequirement = 1,
    FPURequirement = SHFPUReqPairedSingle, isCodeGenOnly = 1 in {
  let SchedRW = [SH4WriteLS] in
    def FMOV_P_COPY_XD : SHDRegReg<1, 0, 0xc, (outs XD:$dst), (ins DR:$src),
                                  "fmov\t$src,$dst">;
  foreach Bank = 0-1 in {
    let mayLoad = 1, Constraints = "$base_wb = $base", SchedRW = [SH4WriteLoad] in
    def FMOV_P_LOAD_ # !if(Bank, "XD", "DR") : SHDLoadPostInc<Bank,
      (outs !if(Bank, XD, DR):$dst, postincgpr:$base_wb), (ins GPR:$base),
      "fmov.d\t$base_wb,$dst">;
    let mayStore = 1, Constraints = "$base_wb = $base", SchedRW = [SH4WriteStore] in
    def FMOV_P_STORE_ # !if(Bank, "XD", "DR") : SHDStorePreDec<Bank,
      (outs predecgpr:$base_wb), (ins !if(Bank, XD, DR):$src, GPR:$base),
      "fmov.d\t$src,$base_wb">;
  }
}
