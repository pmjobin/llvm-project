//===-- SHSubtarget.td - SuperH subtarget definitions -------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

def FeatureSH2 : SubtargetFeature<"sh2", "HasSH2", "true",
	"Enable the SH-2 instruction-set additions">;
def FeatureSH2A : SubtargetFeature<"sh2a", "HasSH2A", "true",
	"Enable the SH-2A instruction-set additions", [FeatureSH2]>;
def FeatureDynamicShift : SubtargetFeature<"dynamic-shift", "HasDynamicShift",
	"true", "Enable SHAD and SHLD", [FeatureSH2]>;
def FeatureSBitControl : SubtargetFeature<"s-bit-control", "HasSBitControl",
	"true", "Enable CLRS and SETS", [FeatureSH2]>;
def FeatureExceptionState : SubtargetFeature<"exception-state",
	"HasExceptionState", "true", "Enable SSR and SPC exception state",
	[FeatureSH2]>;
def FeatureBankedGPRs : SubtargetFeature<"banked-gprs", "HasBankedGPRs",
	"true", "Enable opposite-bank GPR transfers", [FeatureExceptionState]>;
def FeatureMMU : SubtargetFeature<"mmu", "HasMMU", "true",
	"Enable the memory-management unit", [FeatureSH2]>;
def FeaturePrefetch : SubtargetFeature<"prefetch", "HasPrefetch", "true",
	"Enable cache prefetch", [FeatureSH2]>;
def FeatureSH4DebugState : SubtargetFeature<"sh4-debug-state",
	"HasSH4DebugState", "true", "Enable SH-4 debug-state transfers",
	[FeatureExceptionState]>;
def FeatureSH4Cache : SubtargetFeature<"sh4-cache", "HasSH4Cache", "true",
	"Enable SH-4 operand-cache operations", [FeaturePrefetch]>;
def FeatureMOVAIllegalInDelaySlot : SubtargetFeature<"mova-slot-illegal",
	"HasMOVAIllegalInDelaySlot", "true",
	"Disallow MOVA in delayed branch slots", [FeatureSH2]>;
def FeatureSH4ACommon : SubtargetFeature<"sh4a-common", "HasSH4ACommon",
	"true", "Enable the SH-4A instruction subset shared with ST40-300",
	[FeatureSH2]>;
def FeatureSH4A : SubtargetFeature<"sh4a", "HasSH4A", "true",
	"Enable the SH-4A instruction-set additions", [FeatureSH4ACommon]>;
def FeatureDSP : SubtargetFeature<"dsp", "HasDSP", "true",
	"Enable the SH DSP register and instruction architecture", [FeatureSH2]>;
def FeatureSH4ALDSP : SubtargetFeature<"sh4al-dsp", "HasSH4ALDSP", "true",
	"Enable the SH4AL-DSP architectural additions", [FeatureSH4A, FeatureDSP]>;
def FeatureST40300 : SubtargetFeature<"st40-300", "HasST40300", "true",
	"Enable the ST40-300 instruction-set additions", [FeatureSH4ACommon]>;
def FeatureSingleFPU : SubtargetFeature<"single-fpu", "HasSingleFPU", "true",
	"Enable the single-precision FPU architecture", [FeatureSH2]>;
def FeatureFSQRT : SubtargetFeature<"fsqrt", "HasFSQRT", "true",
	"Enable FSQRT", [FeatureSingleFPU]>;
def FeatureFPUSizeMode : SubtargetFeature<"fpu-size-mode", "HasFPUSizeMode",
	"true", "Enable FPSCR.SZ and paired-width transfers", [FeatureSingleFPU]>;
def FeatureDoubleFPU : SubtargetFeature<"double-fpu", "HasDoubleFPU", "true",
	"Enable double-precision FPU operations",
	[FeatureSingleFPU, FeatureFPUSizeMode]>;
def FeatureFPURegisterBank : SubtargetFeature<"fpu-register-bank",
	"HasFPURegisterBank", "true", "Enable the alternate FPU register bank",
	[FeatureSingleFPU]>;
def FeatureFPUVector : SubtargetFeature<"fpu-vector", "HasFPUVector", "true",
	"Enable FPU vector and matrix operations",
	[FeatureSingleFPU, FeatureFPURegisterBank]>;
def FeatureFPUSingleMode : SubtargetFeature<"fpu-single-mode",
	"HasFPUSingleMode", "true", "Use the public FPSCR single-mode contract",
	[FeatureSingleFPU]>;

def HasSH2 : Predicate<"Subtarget->hasSH2()">,
	AssemblerPredicate<(all_of FeatureSH2)>;
def HasSH2A : Predicate<"Subtarget->hasSH2A()">,
	AssemblerPredicate<(all_of FeatureSH2A)>;
def HasDynamicShift : Predicate<"Subtarget->hasDynamicShift()">,
	AssemblerPredicate<(all_of FeatureDynamicShift)>;
def HasSBitControl : Predicate<"Subtarget->hasSBitControl()">,
	AssemblerPredicate<(all_of FeatureSBitControl)>;
def HasExceptionState : Predicate<"Subtarget->hasExceptionState()">,
	AssemblerPredicate<(all_of FeatureExceptionState)>;
def HasBankedGPRs : Predicate<"Subtarget->hasBankedGPRs()">,
	AssemblerPredicate<(all_of FeatureBankedGPRs)>;
def HasMMU : Predicate<"Subtarget->hasMMU()">,
	AssemblerPredicate<(all_of FeatureMMU)>;
def HasPrefetch : Predicate<"Subtarget->hasPrefetch()">,
	AssemblerPredicate<(all_of FeaturePrefetch)>;
def HasSH4DebugState : Predicate<"Subtarget->hasSH4DebugState()">,
	AssemblerPredicate<(all_of FeatureSH4DebugState)>;
def HasSH4Cache : Predicate<"Subtarget->hasSH4Cache()">,
	AssemblerPredicate<(all_of FeatureSH4Cache)>;
def HasSH4ACommon : Predicate<"Subtarget->hasSH4ACommon()">,
	AssemblerPredicate<(all_of FeatureSH4ACommon)>;
def HasSH4A : Predicate<"Subtarget->hasSH4A()">,
	AssemblerPredicate<(all_of FeatureSH4A)>;
def HasDSP : Predicate<"Subtarget->hasDSP()">,
	AssemblerPredicate<(all_of FeatureDSP)>;
def HasSH4ALDSP : Predicate<"Subtarget->hasSH4ALDSP()">,
	AssemblerPredicate<(all_of FeatureSH4ALDSP)>;
def HasST40300 : Predicate<"Subtarget->hasST40300()">,
	AssemblerPredicate<(all_of FeatureST40300)>;
def HasSingleFPU : Predicate<"Subtarget->hasSingleFPU()">,
	AssemblerPredicate<(all_of FeatureSingleFPU)>;
def HasFSQRT : Predicate<"Subtarget->hasFSQRT()">,
	AssemblerPredicate<(all_of FeatureFSQRT)>;
def HasFPUSizeMode : Predicate<"Subtarget->hasFPUSizeMode()">,
	AssemblerPredicate<(all_of FeatureFPUSizeMode)>;
def HasDoubleFPU : Predicate<"Subtarget->hasDoubleFPU()">,
	AssemblerPredicate<(all_of FeatureDoubleFPU)>;
def HasFPURegisterBank : Predicate<"Subtarget->hasFPURegisterBank()">,
	AssemblerPredicate<(all_of FeatureFPURegisterBank)>;
def HasFPUVector : Predicate<"Subtarget->hasFPUVector()">,
	AssemblerPredicate<(all_of FeatureFPUVector)>;

def SH1Model : SchedMachineModel {
	let IssueWidth = 1;
	let CompleteModel = 0;
}

def SH3Model : SchedMachineModel {
	let IssueWidth = 1;
	let CompleteModel = 0;
}

def SH2EModel : SchedMachineModel {
	let IssueWidth = 1;
	let CompleteModel = 0;
}

def SH2AModel : SchedMachineModel {
	let IssueWidth = 2;
	let CompleteModel = 0;
}

def SH3EModel : SchedMachineModel {
	let IssueWidth = 1;
	let CompleteModel = 0;
}

def SH4Model : SchedMachineModel {
	let IssueWidth = 2;
	let CompleteModel = 0;
}

def SH4AModel : SchedMachineModel {
	let IssueWidth = 2;
	let CompleteModel = 0;
}

def SH4ALDSPModel : SchedMachineModel {
	let IssueWidth = 2;
	let MicroOpBufferSize = 0;
	let CompleteModel = 0;
}

def ST40100Model : SchedMachineModel {
	let IssueWidth = 2;
	let MicroOpBufferSize = 0;
	let CompleteModel = 0;
}

def ST40200Model : SchedMachineModel {
	let IssueWidth = 2;
	let MicroOpBufferSize = 0;
	let CompleteModel = 0;
}

def ST40300Model : SchedMachineModel {
	let IssueWidth = 2;
	let MicroOpBufferSize = 0;
	let CompleteModel = 0;
}

// These abstract writes describe the compiler-visible SH4 pipeline groups.
// Each implementation model binds them to its own processor resources.
def SH4WriteEX : SchedWrite;
def SH4WriteMT : SchedWrite;
def SH4WriteMT1 : SchedWrite;
def SH4WriteLS : SchedWrite;
def SH4WriteLoad : SchedWrite;
def SH4WriteLS3 : SchedWrite;
def SH4WriteStore : SchedWrite;
def SH4WriteBR : SchedWrite;
def SH4WriteCO : SchedWrite;
def SH4WriteCO2 : SchedWrite;
def SH4WriteCO3 : SchedWrite;
def SH4WriteCO4 : SchedWrite;
def SH4WriteCO5 : SchedWrite;
def SH4WriteCO7 : SchedWrite;
def SH4WriteControl3 : SchedWrite;
def SH4WriteRTE5 : SchedWrite;
def SH4WriteMul4 : SchedWrite;
def SH4WriteDoubleCmp5 : SchedWrite;
def SH4WriteTrap7 : SchedWrite;
def SH4WriteFE3 : SchedWrite;
def SH4WriteSingleMul : SchedWrite;
def SH4WriteFE4 : SchedWrite;
def SH4WriteFE5 : SchedWrite;
def SH4WriteFE7 : SchedWrite;
def SH4WriteFE8 : SchedWrite;
def SH4WriteFE9 : SchedWrite;
def SH4WriteFE12 : SchedWrite;
def SH4WriteFE24 : SchedWrite;
def SH4ReadDynamicShift : SchedRead;

def SH4AWriteAtomic : SchedWrite;
def SH4AWriteMOVUA : SchedWrite;
def SH4AWriteICBI : SchedWrite;
def SH4AWritePREFI : SchedWrite;
def SH4AWriteSYNCO : SchedWrite;
def SH4AWriteFSRRA : SchedWrite;
def SH4AWriteFSCA : SchedWrite;
def SH4AWriteFPChange : SchedWrite;

def SH4ALDSPWriteControl2 : SchedWrite;
def SH4ALDSPWriteTransfer : SchedWrite;
def SH4ALDSPWriteOperation : SchedWrite;
def SH4ALDSPWriteMul1 : SchedWrite;
def SH4ALDSPWriteMul2 : SchedWrite;
def SH4ALDSPWriteMACW : SchedWrite;
def SH4ALDSPWriteMACL : SchedWrite;

// ST40-300 has a seven-stage integer pipeline and a ten-stage floating-point
// pipeline.  Keep its reservations separate from the earlier ST40 cores: the
// resource topology and documented producer latencies are different.
def ST40300WriteMove : SchedWrite;
def ST40300WriteALU : SchedWrite;
def ST40300WriteLoad : SchedWrite;
def ST40300WriteStore : SchedWrite;
def ST40300WriteMul : SchedWrite;
def ST40300WriteDMul : SchedWrite;
def ST40300WriteMACW : SchedWrite;
def ST40300WriteMACL : SchedWrite;
def ST40300WriteBranch : SchedWrite;
def ST40300WriteControl : SchedWrite;
def ST40300WriteRTE : SchedWrite;
def ST40300WriteICBI : SchedWrite;
def ST40300WriteSYNCO : SchedWrite;
def ST40300WriteFPT : SchedWrite;
def ST40300WriteFPLoad : SchedWrite;
def ST40300WriteFPStore : SchedWrite;
def ST40300WriteFPAdd : SchedWrite;
def ST40300WriteFPCmp : SchedWrite;
def ST40300WriteFPMul : SchedWrite;
def ST40300WriteFPDiv : SchedWrite;
def ST40300WriteFPMulD : SchedWrite;
def ST40300WriteFPDivD : SchedWrite;
def ST40300WriteFPVector : SchedWrite;
def ST40300WriteFPMatrix : SchedWrite;
def ST40300WriteFPConvert : SchedWrite;
def ST40300WriteFPChange : SchedWrite;

multiclass SH4WriteResources<ProcResourceKind MT, ProcResourceKind EX, ProcResourceKind LS, ProcResourceKind BR, ProcResourceKind CO, ProcResourceKind FE, bit IsST40> {
	def : WriteRes<SH4WriteEX, [EX]>;
	def : WriteRes<SH4WriteMT, [MT]> { let Latency = 0; }
	def : WriteRes<SH4WriteMT1, [MT]> { let Latency = 1; }
	def : WriteRes<SH4WriteLS, [LS]> { let Latency = 0; }
	def : WriteRes<SH4WriteLoad, [LS]> { let Latency = 2; }
	def : WriteRes<SH4WriteLS3, [LS]> { let Latency = 3; }
	def : WriteRes<SH4WriteStore, [LS]>;
	def : WriteRes<SH4WriteBR, [BR]> { let Latency = 2; }
	def : WriteRes<SH4WriteCO, [CO]> { let SingleIssue = IsST40; }
	def : WriteRes<SH4WriteCO2, [CO]> {
		let Latency = 2;
		let SingleIssue = IsST40;
	}
	def : WriteRes<SH4WriteCO3, [CO]> {
		let Latency = 3;
		let SingleIssue = IsST40;
	}
	def : WriteRes<SH4WriteCO4, [CO]> {
		let Latency = 4;
		let SingleIssue = IsST40;
	}
	def : WriteRes<SH4WriteCO5, [CO]> {
		let Latency = 5;
		let SingleIssue = IsST40;
	}
	def : WriteRes<SH4WriteCO7, [CO]> {
		let Latency = 7;
		let SingleIssue = IsST40;
	}
	def : WriteRes<SH4WriteControl3, [CO]> {
		let Latency = 3;
		let ReleaseAtCycles = !if(IsST40, [2], []);
		let SingleIssue = IsST40;
	}
	def : WriteRes<SH4WriteRTE5, [CO]> {
		let Latency = 5;
		let ReleaseAtCycles = !if(IsST40, [5], []);
		let SingleIssue = IsST40;
	}
	def : WriteRes<SH4WriteMul4, [CO]> {
		let Latency = 4;
		let ReleaseAtCycles = !if(IsST40, [2], []);
		let SingleIssue = IsST40;
	}
	def : WriteRes<SH4WriteDoubleCmp5, [CO]> {
		let Latency = 5;
		let ReleaseAtCycles = !if(IsST40, [2], []);
		let SingleIssue = IsST40;
	}
	def : WriteRes<SH4WriteTrap7, [CO]> {
		let Latency = 7;
		let ReleaseAtCycles = !if(IsST40, [7], []);
		let SingleIssue = IsST40;
	}
	def : WriteRes<SH4WriteFE3, [FE]> { let Latency = 3; }
	def : WriteRes<SH4WriteSingleMul, [FE]> {
		let Latency = !if(IsST40, 3, 4);
	}
	def : WriteRes<SH4WriteFE4, [FE]> { let Latency = 4; }
	def : WriteRes<SH4WriteFE5, [FE]> { let Latency = 5; }
	def : WriteRes<SH4WriteFE7, [FE]> {
		let Latency = !if(IsST40, 8, 7);
		let ReleaseAtCycles = !if(IsST40, [6], []);
	}
	def : WriteRes<SH4WriteFE8, [FE]> {
		let Latency = !if(IsST40, 7, 8);
		let ReleaseAtCycles = !if(IsST40, [4], []);
	}
	def : WriteRes<SH4WriteFE9, [FE]> {
		let Latency = 9;
		let ReleaseAtCycles = !if(IsST40, [6], []);
	}
	def : WriteRes<SH4WriteFE12, [FE]> { let Latency = 12; }
	def : WriteRes<SH4WriteFE24, [FE]> { let Latency = 24; }
	def : ReadAdvance<SH4ReadDynamicShift, !if(IsST40, -1, 0),
		[SH4WriteLoad]>;
}

let SchedModel = SH4Model in {
	def SH4MT : ProcResource<1>;
	def SH4EX : ProcResource<1>;
	def SH4LS : ProcResource<1>;
	def SH4BR : ProcResource<1>;
	def SH4CO : ProcResource<1>;
	def SH4FE : ProcResource<1>;

	defm : SH4WriteResources<SH4MT, SH4EX, SH4LS, SH4BR, SH4CO, SH4FE, 0>;
}

let SchedModel = SH4AModel in {
	def SH4AEX : ProcResource<1>;
	def SH4ALS : ProcResource<1>;
	def SH4ACO : ProcResource<1>;
	def SH4AFEX : ProcResource<1>;
	def SH4AFLS : ProcResource<1>;
	def SH4AMULT : ProcResource<1>;
	def SH4AFDIV : ProcResource<1>;
	def SH4AMemory : ProcResource<1>;

	def : WriteRes<SH4WriteEX, [SH4AEX]> { let Latency = 0; }
	def : WriteRes<SH4WriteMT, [SH4AEX]> { let Latency = 0; }
	def : WriteRes<SH4WriteMT1, [SH4AEX]>;
	def : WriteRes<SH4WriteLS, [SH4ALS]> { let Latency = 2; }
	def : WriteRes<SH4WriteLoad, [SH4ALS, SH4AMemory]> {
		let Latency = 3;
	}
	def : WriteRes<SH4WriteLS3, [SH4ALS]> { let Latency = 3; }
	def : WriteRes<SH4WriteStore, [SH4ALS, SH4AMemory]> {
		let Latency = 0;
	}
	def : WriteRes<SH4WriteBR, [SH4AEX]> { let Latency = 2; }
	def : WriteRes<SH4WriteCO, [SH4ACO]>;
	def : WriteRes<SH4WriteCO2, [SH4ACO]> { let Latency = 2; }
	def : WriteRes<SH4WriteCO3, [SH4ACO]> { let Latency = 3; }
	def : WriteRes<SH4WriteCO4, [SH4ACO]> { let Latency = 4; }
	def : WriteRes<SH4WriteCO5, [SH4ACO]> { let Latency = 5; }
	def : WriteRes<SH4WriteCO7, [SH4ACO]> { let Latency = 7; }
	def : WriteRes<SH4WriteControl3, [SH4AEX]> { let Latency = 3; }
	def : WriteRes<SH4WriteRTE5, [SH4AEX, SH4ALS]> {
		let Latency = 3;
		let ReleaseAtCycles = [4, 4];
		let SingleIssue = 1;
	}
	def : WriteRes<SH4WriteMul4, [SH4AEX, SH4AMULT]> {
		let Latency = 3;
	}
	def : WriteRes<SH4WriteDoubleCmp5, [SH4AFEX]> { let Latency = 3; }
	def : WriteRes<SH4WriteTrap7, [SH4ACO]> { let Latency = 7; }
	def : WriteRes<SH4WriteFE3, [SH4AFEX]> { let Latency = 3; }
	def : WriteRes<SH4WriteSingleMul, [SH4AFEX]> { let Latency = 3; }
	def : WriteRes<SH4WriteFE4, [SH4AFEX]> { let Latency = 3; }
	def : WriteRes<SH4WriteFE5, [SH4AFEX]> { let Latency = 5; }
	def : WriteRes<SH4WriteFE7, [SH4AFEX]> {
		let Latency = 7;
		let ReleaseAtCycles = [3];
	}
	def : WriteRes<SH4WriteFE8, [SH4AFEX]> {
		let Latency = 5;
		let ReleaseAtCycles = [3];
	}
	def : WriteRes<SH4WriteFE9, [SH4AFEX]> {
		let Latency = 5;
		let ReleaseAtCycles = [3];
	}
	def : WriteRes<SH4WriteFE12, [SH4AFEX, SH4AFDIV]> {
		let Latency = 20;
	}
	def : WriteRes<SH4WriteFE24, [SH4AFEX, SH4AFDIV]> {
		let Latency = 36;
		let ReleaseAtCycles = [3, 1];
	}
	def : ReadAdvance<SH4ReadDynamicShift, 0, [SH4WriteLoad]>;

	def : WriteRes<SH4AWriteAtomic, [SH4ACO]>;
	def : WriteRes<SH4AWriteMOVUA, [SH4ALS, SH4AMemory]> {
		let Latency = 3;
		let ReleaseAtCycles = [1, 2];
	}
	def : WriteRes<SH4AWriteICBI, [SH4ACO]> {
		let Latency = 13;
		let ReleaseAtCycles = [16];
		let SingleIssue = 1;
	}
	def : WriteRes<SH4AWritePREFI, [SH4ACO]> {
		let Latency = 10;
		let ReleaseAtCycles = [13];
		let SingleIssue = 1;
	}
	def : WriteRes<SH4AWriteSYNCO, [SH4ACO]> {
		let SingleIssue = 1;
	}
	def : WriteRes<SH4AWriteFSRRA, [SH4AFEX]> { let Latency = 5; }
	def : WriteRes<SH4AWriteFSCA, [SH4AFEX]> {
		let Latency = 7;
		let ReleaseAtCycles = [3];
	}
	def : WriteRes<SH4AWriteFPChange, [SH4AFEX, SH4AFLS]> {
		let Latency = 3;
	}
}

// SH4AL-DSP has a seven-stage CPU pipeline plus independent DSP-LS and
// DSP-extended pipelines. The reference groups permit DSP transfers to pair
// with EX, MT, or BR, while DSP operations and CPU CO instructions issue
// alone. The atomic DSP_PPI envelope reserves every unit that its encoded
// parallel operation may use; CompleteModel remains false because the MC-only
// envelope intentionally does not split scheduling classes by encoded field.
let SchedModel = SH4ALDSPModel in {
	def SH4ALDSP_EX : ProcResource<1>;
	def SH4ALDSP_MT : ProcResource<1>;
	def SH4ALDSP_BR : ProcResource<1>;
	def SH4ALDSP_LS : ProcResource<1>;
	def SH4ALDSP_CO : ProcResource<1>;
	def SH4ALDSP_Memory : ProcResource<1>;
	def SH4ALDSP_Mult : ProcResource<1>;
	def SH4ALDSP_FPU : ProcResource<1>;
	def SH4ALDSP_DSP_LS : ProcResource<1>;
	def SH4ALDSP_DSP_EX : ProcResource<1>;
	def SH4ALDSP_DSP_Mult : ProcResource<1>;

	def : WriteRes<SH4WriteEX, [SH4ALDSP_EX]>;
	def : WriteRes<SH4WriteMT, [SH4ALDSP_MT]> { let Latency = 0; }
	def : WriteRes<SH4WriteMT1, [SH4ALDSP_MT]>;
	def : WriteRes<SH4WriteLS, [SH4ALDSP_LS]>;
	def : WriteRes<SH4WriteLoad, [SH4ALDSP_LS, SH4ALDSP_Memory]> {
		let Latency = 3;
	}
	def : WriteRes<SH4WriteLS3, [SH4ALDSP_LS]> { let Latency = 3; }
	def : WriteRes<SH4WriteStore, [SH4ALDSP_LS, SH4ALDSP_Memory]>;
	def : WriteRes<SH4WriteBR, [SH4ALDSP_BR]>;
	def : WriteRes<SH4WriteCO, [SH4ALDSP_CO]> { let SingleIssue = 1; }
	def : WriteRes<SH4WriteCO2, [SH4ALDSP_CO]> {
		let Latency = 2;
		let ReleaseAtCycles = [2];
		let SingleIssue = 1;
	}
	def : WriteRes<SH4WriteCO3, [SH4ALDSP_CO]> {
		let Latency = 3;
		let ReleaseAtCycles = [3];
		let SingleIssue = 1;
	}
	def : WriteRes<SH4WriteCO4, [SH4ALDSP_CO]> {
		let Latency = 4;
		let ReleaseAtCycles = [4];
		let SingleIssue = 1;
	}
	def : WriteRes<SH4WriteCO5, [SH4ALDSP_CO]> {
		let Latency = 5;
		let ReleaseAtCycles = [5];
		let SingleIssue = 1;
	}
	def : WriteRes<SH4WriteCO7, [SH4ALDSP_CO]> {
		let Latency = 7;
		let ReleaseAtCycles = [7];
		let SingleIssue = 1;
	}
	def : WriteRes<SH4WriteControl3, [SH4ALDSP_BR]> { let Latency = 4; }
	def : WriteRes<SH4WriteRTE5, [SH4ALDSP_CO]> {
		let Latency = 4;
		let ReleaseAtCycles = [5];
		let SingleIssue = 1;
	}
	def : WriteRes<SH4WriteMul4, [SH4ALDSP_EX, SH4ALDSP_Mult]> {
		let Latency = 2;
	}
	def : WriteRes<SH4WriteDoubleCmp5, [SH4ALDSP_FPU]> { let Latency = 5; }
	def : WriteRes<SH4WriteTrap7, [SH4ALDSP_CO]> {
		let Latency = 13;
		let ReleaseAtCycles = [14];
		let SingleIssue = 1;
	}
	def : WriteRes<SH4WriteFE3, [SH4ALDSP_FPU]> { let Latency = 3; }
	def : WriteRes<SH4WriteSingleMul, [SH4ALDSP_FPU]> { let Latency = 4; }
	def : WriteRes<SH4WriteFE4, [SH4ALDSP_FPU]> { let Latency = 4; }
	def : WriteRes<SH4WriteFE5, [SH4ALDSP_FPU]> { let Latency = 5; }
	def : WriteRes<SH4WriteFE7, [SH4ALDSP_FPU]> { let Latency = 7; }
	def : WriteRes<SH4WriteFE8, [SH4ALDSP_FPU]> { let Latency = 8; }
	def : WriteRes<SH4WriteFE9, [SH4ALDSP_FPU]> { let Latency = 9; }
	def : WriteRes<SH4WriteFE12, [SH4ALDSP_FPU]> { let Latency = 12; }
	def : WriteRes<SH4WriteFE24, [SH4ALDSP_FPU]> { let Latency = 24; }
	def : ReadAdvance<SH4ReadDynamicShift, 0, [SH4WriteLoad]>;

	def : WriteRes<SH4AWriteAtomic, [SH4ALDSP_CO]> {
		let SingleIssue = 1;
	}
	def : WriteRes<SH4AWriteMOVUA, [SH4ALDSP_LS, SH4ALDSP_Memory]> {
		let Latency = 2;
		let ReleaseAtCycles = [2, 2];
	}
	def : WriteRes<SH4AWriteICBI, [SH4ALDSP_CO]> {
		let Latency = 13;
		let ReleaseAtCycles = [16];
		let SingleIssue = 1;
	}
	def : WriteRes<SH4AWritePREFI, [SH4ALDSP_CO]> {
		let Latency = 10;
		let ReleaseAtCycles = [13];
		let SingleIssue = 1;
	}
	def : WriteRes<SH4AWriteSYNCO, [SH4ALDSP_CO]> { let SingleIssue = 1; }
	def : WriteRes<SH4AWriteFSRRA, [SH4ALDSP_FPU]> { let Latency = 5; }
	def : WriteRes<SH4AWriteFSCA, [SH4ALDSP_FPU]> { let Latency = 7; }
	def : WriteRes<SH4AWriteFPChange, [SH4ALDSP_FPU]> { let Latency = 3; }

	def : WriteRes<SH4ALDSPWriteControl2, [SH4ALDSP_CO]> {
		let Latency = 2;
		let ReleaseAtCycles = [2];
		let SingleIssue = 1;
	}
	def : WriteRes<SH4ALDSPWriteTransfer,
		[SH4ALDSP_LS, SH4ALDSP_DSP_LS, SH4ALDSP_Memory]>;
	def : WriteRes<SH4ALDSPWriteOperation,
		[SH4ALDSP_DSP_LS, SH4ALDSP_DSP_EX, SH4ALDSP_DSP_Mult]> {
		let SingleIssue = 1;
	}
	def : WriteRes<SH4ALDSPWriteMul1, [SH4ALDSP_EX, SH4ALDSP_Mult]>;
	def : WriteRes<SH4ALDSPWriteMul2, [SH4ALDSP_EX, SH4ALDSP_Mult]> {
		let Latency = 2;
	}
	def : WriteRes<SH4ALDSPWriteMACW,
		[SH4ALDSP_CO, SH4ALDSP_LS, SH4ALDSP_Mult]> {
		let Latency = 4;
		let ReleaseAtCycles = [2, 2, 2];
		let SingleIssue = 1;
	}
	def : WriteRes<SH4ALDSPWriteMACL,
		[SH4ALDSP_CO, SH4ALDSP_LS, SH4ALDSP_Mult]> {
		let Latency = 5;
		let ReleaseAtCycles = [2, 2, 2];
		let SingleIssue = 1;
	}
}

let SchedModel = ST40100Model in {
	def ST40100MT : ProcResource<1>;
	def ST40100EX : ProcResource<1>;
	def ST40100LS : ProcResource<1>;
	def ST40100BR : ProcResource<1>;
	def ST40100CO : ProcResource<1>;
	def ST40100FE : ProcResource<1>;

	defm : SH4WriteResources<ST40100MT, ST40100EX, ST40100LS,
		ST40100BR, ST40100CO, ST40100FE, 1>;
}

let SchedModel = ST40200Model in {
	def ST40200MT : ProcResource<1>;
	def ST40200EX : ProcResource<1>;
	def ST40200LS : ProcResource<1>;
	def ST40200BR : ProcResource<1>;
	def ST40200CO : ProcResource<1>;
	def ST40200FE : ProcResource<1>;

	defm : SH4WriteResources<ST40200MT, ST40200EX, ST40200LS,
		ST40200BR, ST40200CO, ST40200FE, 1>;
}

let SchedModel = ST40300Model in {
	def ST40300ALU : ProcResource<2>;
	def ST40300LS : ProcResource<1>;
	def ST40300MUL : ProcResource<1>;
	def ST40300BR : ProcResource<1>;
	def ST40300FPT : ProcResource<1>;
	def ST40300FPU : ProcResource<1>;
	def ST40300FDS : ProcResource<1>;
	def ST40300ALL : ProcResource<1>;

	// Bind the shared instruction scheduling classes to ST40-300 resources.
	// Model-specific InstRW entries below refine every documented core family
	// whose ST40-300 timing differs from these conservative defaults.
	def : WriteRes<SH4WriteEX, [ST40300ALU]>;
	def : WriteRes<SH4WriteMT, [ST40300ALU]> { let Latency = 0; }
	def : WriteRes<SH4WriteMT1, [ST40300ALU]>;
	def : WriteRes<SH4WriteLS, [ST40300FPT]>;
	def : WriteRes<SH4WriteLoad, [ST40300LS]> { let Latency = 3; }
	def : WriteRes<SH4WriteLS3, [ST40300FPT]>;
	def : WriteRes<SH4WriteStore, [ST40300LS]>;
	def : WriteRes<SH4WriteBR, [ST40300BR]> { let Latency = 4; }
	def : WriteRes<SH4WriteCO, [ST40300ALL]>;
	def : WriteRes<SH4WriteCO2, [ST40300ALL]> { let Latency = 2; }
	def : WriteRes<SH4WriteCO3, [ST40300ALL]> { let Latency = 3; }
	def : WriteRes<SH4WriteCO4, [ST40300ALL]> { let Latency = 4; }
	def : WriteRes<SH4WriteCO5, [ST40300ALL]> { let Latency = 5; }
	def : WriteRes<SH4WriteCO7, [ST40300ALL]> { let Latency = 7; }
	def : WriteRes<SH4WriteControl3, [ST40300BR]> { let Latency = 4; }
	def : WriteRes<SH4WriteRTE5, [ST40300BR]> { let Latency = 9; }
	def : WriteRes<SH4WriteMul4, [ST40300MUL]> { let Latency = 3; }
	def : WriteRes<SH4WriteDoubleCmp5, [ST40300FPU]> { let Latency = 3; }
	def : WriteRes<SH4WriteTrap7, [ST40300ALL]> { let Latency = 7; }
	def : WriteRes<SH4WriteFE3, [ST40300FPU]> { let Latency = 6; }
	def : WriteRes<SH4WriteSingleMul, [ST40300FPU]> { let Latency = 6; }
	def : WriteRes<SH4WriteFE4, [ST40300FPU]> { let Latency = 6; }
	def : WriteRes<SH4WriteFE5, [ST40300FPU]> { let Latency = 6; }
	def : WriteRes<SH4WriteFE7, [ST40300FPU]> { let Latency = 6; }
	def : WriteRes<SH4WriteFE8, [ST40300FPU]> {
		let Latency = 9;
		let ReleaseAtCycles = [4];
	}
	def : WriteRes<SH4WriteFE9, [ST40300FPU]> {
		let Latency = 9;
		let ReleaseAtCycles = [4];
	}
	def : WriteRes<SH4WriteFE12, [ST40300FPU, ST40300FDS]> {
		let Latency = 19;
		let ReleaseAtCycles = [1, 16];
	}
	def : WriteRes<SH4WriteFE24, [ST40300FPU, ST40300FDS]> {
		let Latency = 35;
		let ReleaseAtCycles = [1, 32];
	}
	def : ReadAdvance<SH4ReadDynamicShift, 0, [SH4WriteLoad]>;

	def : WriteRes<ST40300WriteMove, [ST40300ALU]> { let Latency = 0; }
	def : WriteRes<ST40300WriteALU, [ST40300ALU]>;
	def : WriteRes<ST40300WriteLoad, [ST40300LS]> { let Latency = 3; }
	def : WriteRes<ST40300WriteStore, [ST40300LS]>;
	def : WriteRes<ST40300WriteMul, [ST40300MUL]> { let Latency = 2; }
	def : WriteRes<ST40300WriteDMul, [ST40300MUL]> { let Latency = 3; }
	def : WriteRes<ST40300WriteMACW, [ST40300MUL, ST40300LS]> {
		let Latency = 5;
		let ReleaseAtCycles = [2, 2];
	}
	def : WriteRes<ST40300WriteMACL, [ST40300MUL, ST40300LS]> {
		let Latency = 4;
		let ReleaseAtCycles = [2, 2];
	}
	def : WriteRes<ST40300WriteBranch, [ST40300BR]> { let Latency = 4; }
	def : WriteRes<ST40300WriteControl, [ST40300BR]> { let Latency = 4; }
	def : WriteRes<ST40300WriteRTE, [ST40300BR]> {
		let Latency = 9;
		let ReleaseAtCycles = [9];
		let SingleIssue = 1;
	}
	def : WriteRes<ST40300WriteICBI, [ST40300LS, ST40300ALL]> {
		let Latency = 8;
		let ReleaseAtCycles = [1, 7];
		let SingleIssue = 1;
	}
	def : WriteRes<ST40300WriteSYNCO, [ST40300LS, ST40300ALL]> {
		let Latency = 1;
		let SingleIssue = 1;
	}
	def : WriteRes<ST40300WriteFPT, [ST40300FPT]>;
	def : WriteRes<ST40300WriteFPLoad, [ST40300FPT, ST40300LS]> {
		let Latency = 4;
	}
	def : WriteRes<ST40300WriteFPStore, [ST40300FPT, ST40300LS]>;
	def : WriteRes<ST40300WriteFPAdd, [ST40300FPU]> { let Latency = 6; }
	def : WriteRes<ST40300WriteFPCmp, [ST40300FPU]> { let Latency = 3; }
	def : WriteRes<ST40300WriteFPMul, [ST40300FPU]> { let Latency = 6; }
	def : WriteRes<ST40300WriteFPDiv, [ST40300FPU, ST40300FDS]> {
		let Latency = 19;
		let ReleaseAtCycles = [1, 16];
	}
	def : WriteRes<ST40300WriteFPMulD, [ST40300FPU]> {
		let Latency = 9;
		let ReleaseAtCycles = [4];
	}
	def : WriteRes<ST40300WriteFPDivD, [ST40300FPU, ST40300FDS]> {
		let Latency = 35;
		let ReleaseAtCycles = [1, 32];
	}
	def : WriteRes<ST40300WriteFPVector, [ST40300FPU]> { let Latency = 6; }
	def : WriteRes<ST40300WriteFPMatrix, [ST40300FPU]> {
		let Latency = 9;
		let ReleaseAtCycles = [4];
	}
	def : WriteRes<ST40300WriteFPConvert, [ST40300FPU]> { let Latency = 6; }
	def : WriteRes<ST40300WriteFPChange, [ST40300FPU, ST40300FPT]>;
}

def : ProcessorModel<"sh1", SH1Model, []>;
def : Processor<"sh2", NoItineraries, [FeatureSH2]>;
def : ProcessorModel<"sh2e", SH2EModel,
	[FeatureSH2, FeatureSingleFPU, FeatureFPUSingleMode]>;
def : ProcessorModel<"sh2a", SH2AModel,
	[FeatureSH2A, FeatureDynamicShift, FeaturePrefetch,
	 FeatureSingleFPU, FeatureFSQRT, FeatureFPUSizeMode, FeatureDoubleFPU]>;
def : ProcessorModel<"sh3", SH3Model,
	[FeatureSH2, FeatureDynamicShift, FeatureSBitControl,
	 FeatureExceptionState, FeatureBankedGPRs, FeatureMMU, FeaturePrefetch]>;
def : ProcessorModel<"sh3e", SH3EModel,
	[FeatureSH2, FeatureDynamicShift, FeatureSBitControl,
	 FeatureExceptionState, FeatureBankedGPRs, FeatureMMU, FeaturePrefetch,
	 FeatureSingleFPU, FeatureFSQRT, FeatureFPUSingleMode]>;
def : ProcessorModel<"sh4", SH4Model,
	[FeatureSH2, FeatureDynamicShift, FeatureSBitControl,
	 FeatureExceptionState, FeatureBankedGPRs, FeatureMMU, FeaturePrefetch,
	 FeatureSH4DebugState, FeatureSH4Cache, FeatureMOVAIllegalInDelaySlot,
	 FeatureSingleFPU, FeatureFSQRT,
	 FeatureFPUSizeMode, FeatureDoubleFPU, FeatureFPURegisterBank,
	 FeatureFPUVector]>;
def : ProcessorModel<"sh4a", SH4AModel,
	[FeatureSH2, FeatureDynamicShift, FeatureSBitControl,
	 FeatureExceptionState, FeatureBankedGPRs, FeatureMMU, FeaturePrefetch,
	 FeatureSH4DebugState, FeatureSH4Cache, FeatureMOVAIllegalInDelaySlot,
	 FeatureSH4ACommon, FeatureSH4A, FeatureSingleFPU, FeatureFSQRT,
	 FeatureFPUSizeMode, FeatureDoubleFPU, FeatureFPURegisterBank,
	 FeatureFPUVector]>;
def : ProcessorModel<"sh4al-dsp", SH4ALDSPModel,
	[FeatureSH2, FeatureDynamicShift, FeatureSBitControl,
	 FeatureExceptionState, FeatureBankedGPRs, FeatureMMU, FeaturePrefetch,
	 FeatureSH4DebugState, FeatureSH4Cache, FeatureMOVAIllegalInDelaySlot,
	 FeatureSH4ACommon, FeatureSH4A, FeatureDSP, FeatureSH4ALDSP]>;
def : ProcessorModel<"st40-100", ST40100Model, []>;
def : ProcessorModel<"st40-200", ST40200Model, []>;
def : ProcessorModel<"st40-300", ST40300Model, []>;
def : ProcessorModel<"st40-340", ST40300Model, []>;
def : ProcessorModel<"st40-400", ST40200Model, []>;
def : ProcessorModel<"st40-500", ST40200Model, []>;
