//===-- SHRegisterInfo.td - SuperH register definitions ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class SHReg<bits<4> Encoding, string Name>
    : Register<Name>, DwarfRegNum<[Encoding]> {
  let Namespace = "SH";
  let HWEncoding{3-0} = Encoding;
}

foreach I = 0-15 in
  def R#I : SHReg<I, "r"#I>;

class SHBankedReg<bits<3> Encoding, string Name> : Register<Name> {
	let Namespace = "SH";
	let HWEncoding{2-0} = Encoding;
}

foreach I = 0-7 in
	def R#I#_BANK : SHBankedReg<I, "r"#I#"_bank">;

let Namespace = "SH" in {
def PC : Register<"pc">, DwarfRegNum<[16]>;
def PR : Register<"pr">, DwarfRegNum<[17]>;
def GBR : Register<"gbr">, DwarfRegNum<[18]>;
def VBR : Register<"vbr">, DwarfRegNum<[19]>;
def TBR : Register<"tbr">;
def MACH : Register<"mach">, DwarfRegNum<[20]>;
def MACL : Register<"macl">, DwarfRegNum<[21]>;
def SR : Register<"sr">, DwarfRegNum<[22]>;
def SSR : Register<"ssr">;
def SPC : Register<"spc">;
def SGR : Register<"sgr">;
def DBR : Register<"dbr">;
def FPUL : Register<"fpul">, DwarfRegNum<[23]>;
def FPSCR : Register<"fpscr">, DwarfRegNum<[24]>;
def TBit : Register<"t">;
def MBit : Register<"">;
def QBit : Register<"">;
def SBit : Register<"">;

	def sub_dsp_lo32 : SubRegIndex<32, 0>;
	def sub_dsp_guard8 : SubRegIndex<8, 32>;
}

class SHDSPReg<bits<4> Encoding, string Name> : Register<Name> {
	let Namespace = "SH";
	let HWEncoding{3-0} = Encoding;
}

def A0L : SHDSPReg<7, "">;
def A1L : SHDSPReg<5, "">;
def A0G : SHDSPReg<15, "a0g">;
def A1G : SHDSPReg<13, "a1g">;

class SHDSPAccumulator<bits<4> Encoding, string Name,
	list<Register> SubRegs> : RegisterWithSubRegs<Name, SubRegs> {
	let Namespace = "SH";
	let HWEncoding{3-0} = Encoding;
	let SubRegIndices = [sub_dsp_lo32, sub_dsp_guard8];
	let CoveredBySubRegs = 1;
}

def A0 : SHDSPAccumulator<7, "a0", [A0L, A0G]>;
def A1 : SHDSPAccumulator<5, "a1", [A1L, A1G]>;

def X0 : SHDSPReg<8, "x0">;
def X1 : SHDSPReg<9, "x1">;
def Y0 : SHDSPReg<10, "y0">;
def Y1 : SHDSPReg<11, "y1">;
def M0 : SHDSPReg<12, "m0">;
def M1 : SHDSPReg<14, "m1">;

let Namespace = "SH" in {
	def DSR : Register<"dsr">;
	def RS : Register<"rs">;
	def RE : Register<"re">;
	def MOD : Register<"mod">;
}

// Keep caller-saved registers first so the narrow register-only leaf-function
// subset does not need callee-save spills.
def GPR : RegisterClass<"SH", [i32], 32,
                        (add (sequence "R%u", 0, 7),
                             (sequence "R%u", 8, 14), R15)>;

def R0Reg : RegisterClass<"SH", [i32], 32, (add R0)>;

// A sibling-call target must survive callee-save restoration and the r3
// scratch used by large epilogue stack adjustments.
def GPRTailCall : RegisterClass<"SH", [i32], 32,
	(add R0, R1, R2, (sequence "R%u", 4, 7))>;

// PIC address materialization uses r0 for MOVA, so its result cannot use r0.
def GPRNoR0 : RegisterClass<"SH", [i32], 32,
                            (add (sequence "R%u", 1, 7),
                                 (sequence "R%u", 8, 14), R15)>;

// The incoming FDPIC context must remain available independently of physical
// r12, which public calls are allowed to clobber.
def GPRNoR12 : RegisterClass<"SH", [i32], 32,
	(add (sequence "R%u", 0, 11), R13, R14, R15)>;

// These classes exist for GNU-compatible inline-assembly constraints. The
// registers remain reserved and are never available to ordinary allocation.
def PRReg : RegisterClass<"SH", [i32], 32, (add PR)>;
def MACReg : RegisterClass<"SH", [i32], 32, (add MACH, MACL)>;
def TReg : RegisterClass<"SH", [i32], 32, (add TBit)>;
def GBRReg : RegisterClass<"SH", [i32], 32, (add GBR)>;

let isAllocatable = 0 in {
	// Keep named inline-assembly clobbers visible to effect discovery. These
	// control registers remain reserved, independent of the argument ABI.
	def FPUControl : RegisterClass<"SH", [i32], 32, (add FPUL, FPSCR)>;
	def DSPAccumulator : RegisterClass<"SH", [i64], 32, (add A0, A1)> {
		let Size = 40;
	}
	def DSPGuard : RegisterClass<"SH", [i32], 8, (add A0G, A1G)> {
		let Size = 8;
	}
	def DSPData32 : RegisterClass<"SH", [i32], 32,
		(add X0, X1, Y0, Y1, M0, M1)>;
	def DSPData : RegisterClass<"SH", [i64], 32,
		(add A0, A1, X0, X1, Y0, Y1, M0, M1)> {
		let Size = 40;
	}
	def DSPXSource : RegisterClass<"SH", [i64], 32,
		(add X0, X1, A0, A1)> {
		let Size = 40;
	}
	def DSPYSource : RegisterClass<"SH", [i32], 32,
		(add Y0, Y1, M0, M1)>;
	def DSPMulXSource : RegisterClass<"SH", [i64], 32,
		(add X0, X1, Y0, A1)> {
		let Size = 40;
	}
	def DSPMulYSource : RegisterClass<"SH", [i64], 32,
		(add Y0, Y1, X0, A1)> {
		let Size = 40;
	}
	def DSPMulDestination : RegisterClass<"SH", [i64], 32,
		(add M0, M1, A0, A1)> {
		let Size = 40;
	}
	def DSPMoveData : RegisterClass<"SH", [i64], 32,
		(add A0, A1, X0, X1, Y0, Y1, M0, A1G, M1, A0G)> {
		let Size = 40;
	}
	def DSRReg : RegisterClass<"SH", [i32], 32, (add DSR)>;
	def RSReg : RegisterClass<"SH", [i32], 32, (add RS)>;
	def REReg : RegisterClass<"SH", [i32], 32, (add RE)>;
	def MODReg : RegisterClass<"SH", [i32], 32, (add MOD)>;
}

// Opposite-bank registers are explicit privileged instruction operands. They
// are distinct storage and never members of the ABI GPR class.
def BankedGPR : RegisterClass<"SH", [i32], 32,
	(add (sequence "R%u_BANK", 0, 7))>;

class SHFPUStorageReg<bits<4> Encoding, string Name, int Dwarf>
	: Register<Name>, DwarfRegNum<[Dwarf]> {
	let Namespace = "SH";
	let HWEncoding{3-0} = Encoding;
}

class SHFPUStorageRegNoDwarf<bits<4> Encoding, string Name> : Register<Name> {
	let Namespace = "SH";
	let HWEncoding{3-0} = Encoding;
}

foreach I = 0-15 in {
	def FR#I : SHFPUStorageReg<I, "fr"#I, !add(25, I)>;
	def XF#I : SHFPUStorageRegNoDwarf<I, "xf"#I>;
}

let Namespace = "SH" in {
	def sub_fpu32_0 : SubRegIndex<32, 0>;
	def sub_fpu32_1 : SubRegIndex<32, 32>;
	def sub_fpu32_2 : SubRegIndex<32, 64>;
	def sub_fpu32_3 : SubRegIndex<32, 96>;
	def sub_fpu32_4 : SubRegIndex<32, 128>;
	def sub_fpu32_5 : SubRegIndex<32, 160>;
	def sub_fpu32_6 : SubRegIndex<32, 192>;
	def sub_fpu32_7 : SubRegIndex<32, 224>;
	def sub_fpu32_8 : SubRegIndex<32, 256>;
	def sub_fpu32_9 : SubRegIndex<32, 288>;
	def sub_fpu32_10 : SubRegIndex<32, 320>;
	def sub_fpu32_11 : SubRegIndex<32, 352>;
	def sub_fpu32_12 : SubRegIndex<32, 384>;
	def sub_fpu32_13 : SubRegIndex<32, 416>;
	def sub_fpu32_14 : SubRegIndex<32, 448>;
	def sub_fpu32_15 : SubRegIndex<32, 480>;
}

class SHFPUViewReg<bits<4> Encoding, string Name, list<Register> SubRegs,
	list<SubRegIndex> Indices> : RegisterWithSubRegs<Name, SubRegs> {
	let Namespace = "SH";
	let HWEncoding{3-0} = Encoding;
	let SubRegIndices = Indices;
	let CoveredBySubRegs = 1;
}

def DR0 : SHFPUViewReg<0, "dr0", [FR0, FR1], [sub_fpu32_0, sub_fpu32_1]>;
def DR2 : SHFPUViewReg<1, "dr2", [FR2, FR3], [sub_fpu32_0, sub_fpu32_1]>;
def DR4 : SHFPUViewReg<2, "dr4", [FR4, FR5], [sub_fpu32_0, sub_fpu32_1]>;
def DR6 : SHFPUViewReg<3, "dr6", [FR6, FR7], [sub_fpu32_0, sub_fpu32_1]>;
def DR8 : SHFPUViewReg<4, "dr8", [FR8, FR9], [sub_fpu32_0, sub_fpu32_1]>;
def DR10 : SHFPUViewReg<5, "dr10", [FR10, FR11], [sub_fpu32_0, sub_fpu32_1]>;
def DR12 : SHFPUViewReg<6, "dr12", [FR12, FR13], [sub_fpu32_0, sub_fpu32_1]>;
def DR14 : SHFPUViewReg<7, "dr14", [FR14, FR15], [sub_fpu32_0, sub_fpu32_1]>;

def XD0 : SHFPUViewReg<0, "xd0", [XF0, XF1], [sub_fpu32_0, sub_fpu32_1]>;
def XD2 : SHFPUViewReg<1, "xd2", [XF2, XF3], [sub_fpu32_0, sub_fpu32_1]>;
def XD4 : SHFPUViewReg<2, "xd4", [XF4, XF5], [sub_fpu32_0, sub_fpu32_1]>;
def XD6 : SHFPUViewReg<3, "xd6", [XF6, XF7], [sub_fpu32_0, sub_fpu32_1]>;
def XD8 : SHFPUViewReg<4, "xd8", [XF8, XF9], [sub_fpu32_0, sub_fpu32_1]>;
def XD10 : SHFPUViewReg<5, "xd10", [XF10, XF11], [sub_fpu32_0, sub_fpu32_1]>;
def XD12 : SHFPUViewReg<6, "xd12", [XF12, XF13], [sub_fpu32_0, sub_fpu32_1]>;
def XD14 : SHFPUViewReg<7, "xd14", [XF14, XF15], [sub_fpu32_0, sub_fpu32_1]>;

def FV0 : SHFPUViewReg<0, "fv0", [FR0, FR1, FR2, FR3],
	[sub_fpu32_0, sub_fpu32_1, sub_fpu32_2, sub_fpu32_3]>;
def FV4 : SHFPUViewReg<1, "fv4", [FR4, FR5, FR6, FR7],
	[sub_fpu32_0, sub_fpu32_1, sub_fpu32_2, sub_fpu32_3]>;
def FV8 : SHFPUViewReg<2, "fv8", [FR8, FR9, FR10, FR11],
	[sub_fpu32_0, sub_fpu32_1, sub_fpu32_2, sub_fpu32_3]>;
def FV12 : SHFPUViewReg<3, "fv12", [FR12, FR13, FR14, FR15],
	[sub_fpu32_0, sub_fpu32_1, sub_fpu32_2, sub_fpu32_3]>;

def XMTRX : SHFPUViewReg<0, "xmtrx",
	[XF0, XF1, XF2, XF3, XF4, XF5, XF6, XF7,
	 XF8, XF9, XF10, XF11, XF12, XF13, XF14, XF15],
	[sub_fpu32_0, sub_fpu32_1, sub_fpu32_2, sub_fpu32_3,
	 sub_fpu32_4, sub_fpu32_5, sub_fpu32_6, sub_fpu32_7,
	 sub_fpu32_8, sub_fpu32_9, sub_fpu32_10, sub_fpu32_11,
	 sub_fpu32_12, sub_fpu32_13, sub_fpu32_14, sub_fpu32_15]>;


// Canonical SH-4 scalar CodeGen allocates the primary bank as either single
// registers or architectural even double-register pairs.  The overlapping
// RegisterWithSubRegs definitions above make FR/DR interference visible to RA.
def FR : RegisterClass<"SH", [f32], 32, (add (sequence "FR%u", 0, 15))>;
def FRNoFR0 : RegisterClass<"SH", [f32], 32, (add (sequence "FR%u", 1, 15))>;
def DR : RegisterClass<"SH", [f64], 32,
	(add DR0, DR2, DR4, DR6, DR8, DR10, DR12, DR14)>;

def FV : RegisterClass<"SH", [v4f32], 32, (add FV0, FV4, FV8, FV12)>;

let isAllocatable = 0 in {
	def FR0Reg : RegisterClass<"SH", [f32], 32, (add FR0)>;
	def XF : RegisterClass<"SH", [f32], 32, (add (sequence "XF%u", 0, 15))>;
	def XD : RegisterClass<"SH", [f64], 64,
		(add XD0, XD2, XD4, XD6, XD8, XD10, XD12, XD14)>;
	def XMTRXReg : RegisterClass<"SH", [v16f32], 512, (add XMTRX)>;
}
