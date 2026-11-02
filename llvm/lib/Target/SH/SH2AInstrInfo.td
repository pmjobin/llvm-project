//===-- SH2AInstrInfo.td - SH-2A instructions ----------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

let Predicates = [HasSH2A] in {
	def MOVI20 : SH2AMovI20<0, (outs GPR:$dst), (ins simm20:$imm),
		"movi20\t$imm,$dst">;
	def MOVI20S : SH2AMovI20<1, (outs GPR:$dst), (ins simm20by8:$imm),
		"movi20s\t$imm,$dst">;

	let mayLoad = 1, Constraints = "$base_wb = $base", Defs = [R0] in {
		def MOVB_R0_LOAD_PREDEC : SH2AR0Update<0xcb,
			(outs predecgpr:$base_wb), (ins GPR:$base),
			"mov.b\t$base_wb,r0">;
		def MOVW_R0_LOAD_PREDEC : SH2AR0Update<0xdb,
			(outs predecgpr:$base_wb), (ins GPR:$base),
			"mov.w\t$base_wb,r0">;
		def MOVL_R0_LOAD_PREDEC : SH2AR0Update<0xeb,
			(outs predecgpr:$base_wb), (ins GPR:$base),
			"mov.l\t$base_wb,r0">;
	}

	let mayStore = 1, Constraints = "$base_wb = $base", Uses = [R0] in {
		def MOVB_R0_STORE_POSTINC : SH2AR0Update<0x8b,
			(outs postincgpr:$base_wb), (ins GPR:$base),
			"mov.b\tr0,$base_wb">;
		def MOVW_R0_STORE_POSTINC : SH2AR0Update<0x9b,
			(outs postincgpr:$base_wb), (ins GPR:$base),
			"mov.w\tr0,$base_wb">;
		def MOVL_R0_STORE_POSTINC : SH2AR0Update<0xab,
			(outs postincgpr:$base_wb), (ins GPR:$base),
			"mov.l\tr0,$base_wb">;
	}

	let mayLoad = 1 in {
		def MOVB_LOAD_DISP12 : SH2ADisp12Load<0x4, (outs GPR:$dst),
			(ins extbytememdisp:$addr), "mov.b\t$addr,$dst">;
		def MOVUB_LOAD_DISP12 : SH2ADisp12Load<0x8, (outs GPR:$dst),
			(ins extbytememdisp:$addr), "movu.b\t$addr,$dst">;
		def MOVW_LOAD_DISP12 : SH2ADisp12Load<0x5, (outs GPR:$dst),
			(ins extwordmemdisp:$addr), "mov.w\t$addr,$dst">;
		def MOVUW_LOAD_DISP12 : SH2ADisp12Load<0x9, (outs GPR:$dst),
			(ins extwordmemdisp:$addr), "movu.w\t$addr,$dst">;
		def MOVL_LOAD_DISP12 : SH2ADisp12Load<0x6, (outs GPR:$dst),
			(ins extlongmemdisp:$addr), "mov.l\t$addr,$dst">;
	}

	let mayStore = 1 in {
		def MOVB_STORE_DISP12 : SH2ADisp12Store<0x0, (outs),
			(ins GPR:$src, extbytememdisp:$addr), "mov.b\t$src,$addr">;
		def MOVW_STORE_DISP12 : SH2ADisp12Store<0x1, (outs),
			(ins GPR:$src, extwordmemdisp:$addr), "mov.w\t$src,$addr">;
		def MOVL_STORE_DISP12 : SH2ADisp12Store<0x2, (outs),
			(ins GPR:$src, extlongmemdisp:$addr), "mov.l\t$src,$addr">;
	}

	let Uses = [R15], Defs = [R15], mayStore = 1, hasSideEffects = 1 in {
		def MOVML_STORE : SHOneReg<0xf1, (outs), (ins GPR:$dst, predecr15:$sp),
			"movml.l\t$dst,$sp"> { let DecoderMethod = "decodeSH2AMovm"; }
		def MOVMU_STORE : SHOneReg<0xf0, (outs), (ins GPR:$dst, predecr15:$sp),
			"movmu.l\t$dst,$sp"> { let DecoderMethod = "decodeSH2AMovm"; }
	}
	let Uses = [R15], Defs = [R15], mayLoad = 1, hasSideEffects = 1 in {
		def MOVML_LOAD : SHOneReg<0xf5, (outs GPR:$dst), (ins postincr15:$sp),
			"movml.l\t$sp,$dst"> { let DecoderMethod = "decodeSH2AMovm"; }
		def MOVMU_LOAD : SHOneReg<0xf4, (outs GPR:$dst), (ins postincr15:$sp),
			"movmu.l\t$sp,$dst"> { let DecoderMethod = "decodeSH2AMovm"; }
	}

	let Uses = [TBit] in
	def MOVRT : SHSpecialToReg<0x39, (outs GPR:$dst), (ins), "movrt\t$dst">;

	let Uses = [TBit], Defs = [TBit] in
	def NOTT : SHFixed<0x0068, "nott">;

	let mayLoad = 1, mayStore = 1 in {
		let Uses = [TBit], Defs = [TBit] in {
			def BAND_B : SH2ABitMemory<0x4, (outs),
				(ins uimm3:$imm, extbytememdisp:$addr), "band.b\t$imm,$addr">;
			def BANDNOT_B : SH2ABitMemory<0xc, (outs),
				(ins uimm3:$imm, extbytememdisp:$addr), "bandnot.b\t$imm,$addr">;
			def BOR_B : SH2ABitMemory<0x5, (outs),
				(ins uimm3:$imm, extbytememdisp:$addr), "bor.b\t$imm,$addr">;
			def BORNOT_B : SH2ABitMemory<0xd, (outs),
				(ins uimm3:$imm, extbytememdisp:$addr), "bornot.b\t$imm,$addr">;
			def BXOR_B : SH2ABitMemory<0x6, (outs),
				(ins uimm3:$imm, extbytememdisp:$addr), "bxor.b\t$imm,$addr">;
		}
		let Defs = [TBit] in {
			def BLD_B : SH2ABitMemory<0x3, (outs),
				(ins uimm3:$imm, extbytememdisp:$addr), "bld.b\t$imm,$addr">;
			def BLDNOT_B : SH2ABitMemory<0xb, (outs),
				(ins uimm3:$imm, extbytememdisp:$addr), "bldnot.b\t$imm,$addr">;
		}
		def BCLR_B : SH2ABitMemory<0x0, (outs),
			(ins uimm3:$imm, extbytememdisp:$addr), "bclr.b\t$imm,$addr">;
		def BSET_B : SH2ABitMemory<0x1, (outs),
			(ins uimm3:$imm, extbytememdisp:$addr), "bset.b\t$imm,$addr">;
		let Uses = [TBit] in
		def BST_B : SH2ABitMemory<0x2, (outs),
			(ins uimm3:$imm, extbytememdisp:$addr), "bst.b\t$imm,$addr">;
	}

	let Constraints = "$dst = $dst_in" in {
		def BCLR : SH2ABitRegister<0x86, 0, (outs GPR:$dst),
			(ins GPR:$dst_in, uimm3:$imm), "bclr\t$imm,$dst">;
		def BSET : SH2ABitRegister<0x86, 1, (outs GPR:$dst),
			(ins GPR:$dst_in, uimm3:$imm), "bset\t$imm,$dst">;
		let Uses = [TBit] in
		def BST : SH2ABitRegister<0x87, 0, (outs GPR:$dst),
			(ins GPR:$dst_in, uimm3:$imm), "bst\t$imm,$dst">;
	}
	let Defs = [TBit] in
	def BLD : SH2ABitRegister<0x87, 1, (outs),
		(ins GPR:$dst, uimm3:$imm), "bld\t$imm,$dst">;

	let Constraints = "$dst = $dst_in" in {
		def CLIPS_B : SHOneReg<0x91, (outs GPR:$dst), (ins GPR:$dst_in),
			"clips.b\t$dst">;
		def CLIPS_W : SHOneReg<0x95, (outs GPR:$dst), (ins GPR:$dst_in),
			"clips.w\t$dst">;
		def CLIPU_B : SHOneReg<0x81, (outs GPR:$dst), (ins GPR:$dst_in),
			"clipu.b\t$dst">;
		def CLIPU_W : SHOneReg<0x85, (outs GPR:$dst), (ins GPR:$dst_in),
			"clipu.w\t$dst">;
	}

	let Constraints = "$dst = $dst_in", IsDelaySlotIllegal = 1, Uses = [R0] in {
		def DIVS : SHOneReg<0x94, (outs GPR:$dst),
			(ins GPR:$dst_in), "divs\tr0,$dst">;
		def DIVU : SHOneReg<0x84, (outs GPR:$dst),
			(ins GPR:$dst_in), "divu\tr0,$dst">;
	}
	let usesCustomInserter = 1 in {
		def SH2A_SDIV_PSEUDO : SHPseudo<(outs GPRNoR0:$dst),
			(ins GPR:$dividend, GPR:$divisor)> {
			let Pattern = [(set GPRNoR0:$dst,
				(sdiv GPR:$dividend, GPR:$divisor))];
		}
		def SH2A_UDIV_PSEUDO : SHPseudo<(outs GPRNoR0:$dst),
			(ins GPR:$dividend, GPR:$divisor)> {
			let Pattern = [(set GPRNoR0:$dst,
				(udiv GPR:$dividend, GPR:$divisor))];
		}
	}
	let Constraints = "$dst = $dst_in", Uses = [R0] in
	def MULR : SHOneReg<0x80, (outs GPR:$dst),
		(ins GPR:$dst_in), "mulr\tr0,$dst">;

	let isCall = 1, Defs = [PR] in {
		def JSR_N : SH2AIndirectCallNoDelay<(ins GPR:$callee),
			"jsr/n\t@$callee">;
		let Uses = [TBR] in
		def JSR_N_TBR : SHBranch8<0x83, (ins tbrdisp:$target),
			"jsr/n\t$target">;
	}
	let isReturn = 1, isTerminator = 1, isBarrier = 1, Uses = [PR] in
	def RTS_N : SHFixed<0x006b, "rts/n">;
	let isReturn = 1, isTerminator = 1, isBarrier = 1, Uses = [PR], Defs = [R0] in
	def RTV_N : SHSpecialToReg<0x7b, (outs), (ins GPR:$dst), "rtv/n\t$dst">;

	let mayLoad = 1, Defs = [R0], hasSideEffects = 1 in
	def LDBANK : SHOneReg<0xe5, (outs), (ins longmemreg:$dst),
		"ldbank\t$dst,r0">;
	let mayStore = 1, Uses = [R0], hasSideEffects = 1 in
	def STBANK : SHOneReg<0xe1, (outs), (ins longmemreg:$dst),
		"stbank\tr0,$dst">;
	let mayLoad = 1, hasSideEffects = 1, IsDelaySlotIllegal = 1,
		Defs = [R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12,
			R13, R14, GBR, MACH, MACL, PR] in
	def RESBANK : SHFixed<0x005b, "resbank">;

	let Defs = [TBR] in
	def LDC_TBR : SHRegToSpecial<0x4a, (outs), (ins GPR:$src),
		"ldc\t$src,tbr">;
	let Uses = [TBR] in
	def STC_TBR : SHSpecialToReg<0x4a, (outs GPR:$dst), (ins),
		"stc\ttbr,$dst">;
}

let Predicates = [HasSH2A, HasSingleFPU], Uses = [FPSCR],
	HasFPURequirement = 1, FPURequirement = SHFPUReqSingle in {
	let mayLoad = 1 in
	def FMOV_S_LOAD_DISP12 : SH2ADisp12Load<0x7, (outs FR:$dst),
		(ins extlongmemdisp:$addr), "fmov.s\t$addr,$dst">;
	let mayStore = 1 in
	def FMOV_S_STORE_DISP12 : SH2ADisp12Store<0x3, (outs),
		(ins FR:$src, extlongmemdisp:$addr), "fmov.s\t$src,$addr">;
}

let Predicates = [HasSH2A, HasDoubleFPU], Uses = [FPSCR],
	HasFPURequirement = 1, FPURequirement = SHFPUReqDouble in {
	let mayLoad = 1 in
	def FMOV_D_LOAD_DISP12 : SH2ADisp12DLoad<0x7, (outs DR:$dst),
		(ins extdoublememdisp:$addr), "fmov.d\t$addr,$dst">;
	let mayStore = 1 in
	def FMOV_D_STORE_DISP12 : SH2ADisp12DStore<0x3, (outs),
		(ins DR:$src, extdoublememdisp:$addr), "fmov.d\t$src,$addr">;
}
