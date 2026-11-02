//===-- SH4AInstrInfo.td - SuperH SH-4A instructions --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class SH4AMemToR0<bits<8> Suffix, dag Outs, dag Ins, string Asm>
	: SHInst<Outs, Ins, Asm> {
	bits<4> addr;

	let Inst{15-12} = 0x4;
	let Inst{11-8} = addr;
	let Inst{7-0} = Suffix;
}

let Predicates = [HasSH4A] in {
	let mayStore = 1, hasSideEffects = 1, Uses = [R0], Defs = [TBit],
		SchedRW = [SH4WriteCO] in
	def MOVCO_L : SHSystemMemory<0x73, (outs), (ins longmemreg:$addr),
		"movco.l\tr0,$addr">;

	let mayLoad = 1, hasSideEffects = 1, Defs = [R0],
		SchedRW = [SH4WriteCO] in
	def MOVLI_L : SHSystemMemory<0x63, (outs), (ins longmemreg:$addr),
		"movli.l\t$addr,r0">;

	let mayLoad = 1, Defs = [R0], SchedRW = [SH4WriteLoad] in {
		def MOVUA_L : SH4AMemToR0<0xa9, (outs), (ins longmemreg:$addr),
			"movua.l\t$addr,r0">;

		let Constraints = "$base_wb = $base" in
		def MOVUA_L_POSTINC : SHRegStack<0xe, 0x9,
			(outs postincgpr:$base_wb), (ins GPR:$base),
			"movua.l\t$base_wb,r0">;
	}

	let Defs = [SGR], IsDelaySlotIllegal = 1, SchedRW = [SH4WriteCO4] in
	def LDC_SGR : SHRegToSpecial<0x3a, (outs), (ins GPR:$src),
		"ldc\t$src,sgr">;

	let mayLoad = 1, Defs = [SGR], Constraints = "$base_wb = $base",
		IsDelaySlotIllegal = 1, SchedRW = [SH4WriteCO4] in
	def LDC_L_SGR : SHRegStack<0x3, 0x6, (outs postincgpr:$base_wb),
		(ins GPR:$base), "ldc.l\t$base_wb,sgr">;

	let hasSideEffects = 1, IsDelaySlotIllegal = 1,
		SchedRW = [SH4WriteCO] in
	def PREFI : SHSystemMemory<0xd3, (outs), (ins longmemreg:$addr),
		"prefi\t$addr">;
}
