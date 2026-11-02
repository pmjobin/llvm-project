//===-- SHInstrFormats.td - SuperH instruction formats -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class SHFPURequirement<bits<4> value> {
	bits<4> Value = value;
}

def SHFPUReqNone : SHFPURequirement<0>;
def SHFPUReqPrimaryBank : SHFPURequirement<1>;
def SHFPUReqSingle : SHFPURequirement<2>;
def SHFPUReqDouble : SHFPURequirement<3>;
def SHFPUReqDoubleArithmetic : SHFPURequirement<4>;
def SHFPUReqScalarMove : SHFPURequirement<6>;
def SHFPUReqPairedSingle : SHFPURequirement<7>;
def SHFPUReqSingleArithmetic : SHFPURequirement<8>;

class SHInst<dag Outs, dag Ins, string Asm> : Instruction {
  field bits<16> Inst;
	SHFPURequirement FPURequirement = SHFPUReqNone;
	bit HasFPURequirement = 0;
	bit IsDelaySlotIllegal = 0;
	bit MayModifyFPSCRStatus = 0;

  let Namespace = "SH";
  let OutOperandList = Outs;
  let InOperandList = Ins;
  let AsmString = Asm;
  let Size = 2;
  let Pattern = [];
  let mayLoad = 0;
  let mayStore = 0;
  let hasSideEffects = 0;
  // Opt ordinary instructions into descriptor-constrained renaming explicitly.
  // Pseudos and special-state instructions keep their conservative contract.
  let hasExtraSrcRegAllocReq = 1;
  let hasExtraDefRegAllocReq = 1;
	let TSFlags{2-0} = FPURequirement.Value{2-0};
	let TSFlags{6} = FPURequirement.Value{3};
	let TSFlags{3} = HasFPURequirement;
	let TSFlags{4} = IsDelaySlotIllegal;
	let TSFlags{5} = MayModifyFPSCRStatus;
}

class SHInst32<dag Outs, dag Ins, string Asm> : Instruction {
	field bits<32> Inst;
	SHFPURequirement FPURequirement = SHFPUReqNone;
	bit HasFPURequirement = 0;
	bit IsDelaySlotIllegal = 1;
	bit MayModifyFPSCRStatus = 0;

	let Namespace = "SH";
	let OutOperandList = Outs;
	let InOperandList = Ins;
	let AsmString = Asm;
	let Size = 4;
	let Pattern = [];
	let mayLoad = 0;
	let mayStore = 0;
	let hasSideEffects = 0;
  let hasExtraSrcRegAllocReq = 1;
  let hasExtraDefRegAllocReq = 1;
	let TSFlags{2-0} = FPURequirement.Value{2-0};
	let TSFlags{6} = FPURequirement.Value{3};
	let TSFlags{3} = HasFPURequirement;
	let TSFlags{4} = IsDelaySlotIllegal;
	let TSFlags{5} = MayModifyFPSCRStatus;
}

// SH4AL-DSP double-data-transfer and parallel-processing instructions have a
// grammar in which up to four operations share one architectural encoding.
// TableGen owns the width and the fixed instruction-class bits; the target MC
// layer validates and renders the compound fields as one atomic instruction.
class SHDSPTransferInst<dag Outs, dag Ins, string Asm> : SHInst<Outs, Ins, Asm> {
	bits<10> encoding;

	let Inst{15-14} = 0b11;
	let Inst{13-12} = 0b11;
	let Inst{11-10} = 0b00;
	let Inst{9-0} = encoding;
}

class SHDSPSingleTransferInst<dag Outs, dag Ins, string Asm> : SHInst<Outs, Ins, Asm> {
	bits<10> encoding;

	let Inst{15-14} = 0b11;
	let Inst{13-12} = 0b11;
	let Inst{11-10} = 0b01;
	let Inst{9-0} = encoding;
}

class SHDSPParallelInst<dag Outs, dag Ins, string Asm> : SHInst32<Outs, Ins, Asm> {
	bits<26> encoding;

	let Inst{31-30} = 0b11;
	let Inst{29-28} = 0b11;
	let Inst{27-26} = 0b10;
	let Inst{25-0} = encoding;
}

class SH2AMovI20<bit Shifted, dag Outs, dag Ins, string Asm,
	list<dag> pattern = []> : SHInst32<Outs, Ins, Asm> {
	bits<4> dst;
	bits<20> imm;

	let Pattern = pattern;
	let Inst{31-28} = 0;
	let Inst{27-24} = dst;
	let Inst{23-20} = imm{19-16};
	let Inst{19-17} = 0;
	let Inst{16} = Shifted;
	let Inst{15-0} = imm{15-0};
}

class SH2ADisp12Load<bits<4> Opcode, dag Outs, dag Ins, string Asm>
	: SHInst32<Outs, Ins, Asm> {
	bits<4> dst;
	bits<16> addr;

	let Inst{31-28} = 0x3;
	let Inst{27-24} = dst;
	let Inst{23-20} = addr{15-12};
	let Inst{19-16} = 0x1;
	let Inst{15-12} = Opcode;
	let Inst{11-0} = addr{11-0};
}

class SH2ADisp12Store<bits<4> Opcode, dag Outs, dag Ins, string Asm>
	: SHInst32<Outs, Ins, Asm> {
	bits<4> src;
	bits<16> addr;

	let Inst{31-28} = 0x3;
	let Inst{27-24} = addr{15-12};
	let Inst{23-20} = src;
	let Inst{19-16} = 0x1;
	let Inst{15-12} = Opcode;
	let Inst{11-0} = addr{11-0};
}

class SH2ADisp12DLoad<bits<4> Opcode, dag Outs, dag Ins, string Asm>
	: SHInst32<Outs, Ins, Asm> {
	bits<3> dst;
	bits<16> addr;

	let Inst{31-28} = 0x3;
	let Inst{27-25} = dst;
	let Inst{24} = 0;
	let Inst{23-20} = addr{15-12};
	let Inst{19-16} = 0x1;
	let Inst{15-12} = Opcode;
	let Inst{11-0} = addr{11-0};
}

class SH2ADisp12DStore<bits<4> Opcode, dag Outs, dag Ins, string Asm>
	: SHInst32<Outs, Ins, Asm> {
	bits<3> src;
	bits<16> addr;

	let Inst{31-28} = 0x3;
	let Inst{27-24} = addr{15-12};
	let Inst{23-21} = src;
	let Inst{20} = 0;
	let Inst{19-16} = 0x1;
	let Inst{15-12} = Opcode;
	let Inst{11-0} = addr{11-0};
}

class SH2ABitMemory<bits<4> Opcode, dag Outs, dag Ins, string Asm>
	: SHInst32<Outs, Ins, Asm> {
	bits<3> imm;
	bits<16> addr;

	let Inst{31-28} = 0x3;
	let Inst{27-24} = addr{15-12};
	let Inst{23} = 0;
	let Inst{22-20} = imm;
	let Inst{19-16} = 0x9;
	let Inst{15-12} = Opcode;
	let Inst{11-0} = addr{11-0};
}

class SH2ABitRegister<bits<8> Prefix, bit SetForm, dag Outs, dag Ins,
	string Asm> : SHInst<Outs, Ins, Asm> {
	bits<4> dst;
	bits<3> imm;

	let Inst{15-8} = Prefix;
	let Inst{7-4} = dst;
	let Inst{3} = SetForm;
	let Inst{2-0} = imm;
}

class SH2AR0Update<bits<8> Suffix, dag Outs, dag Ins, string Asm>
	: SHInst<Outs, Ins, Asm> {
	bits<4> base;

	let Inst{15-12} = 0x4;
	let Inst{11-8} = base;
	let Inst{7-0} = Suffix;
}

class SHFixed<bits<16> Encoding, string Asm>
    : SHInst<(outs), (ins), Asm> {
  let Inst = Encoding;
}

class SHPseudo<dag Outs, dag Ins>
    : SHInst<Outs, Ins, ""> {
  let Inst = 0;
  let Size = 0;
  let isPseudo = 1;
  let isCodeGenOnly = 1;
}

class SHRegReg<bits<4> Prefix, bits<4> Suffix, dag Outs, dag Ins, string Asm,
               list<dag> pattern = []>
    : SHInst<Outs, Ins, Asm> {
  bits<4> dst;
  bits<4> src;

  let Pattern = pattern;
  let Inst{15-12} = Prefix;
  let Inst{11-8} = dst;
  let Inst{7-4} = src;
  let Inst{3-0} = Suffix;
}

class SHRegImm<bits<4> Prefix, dag Outs, dag Ins, string Asm,
               list<dag> pattern = []>
    : SHInst<Outs, Ins, Asm> {
  bits<4> dst;
  bits<8> imm;

  let Pattern = pattern;
  let Inst{15-12} = Prefix;
  let Inst{11-8} = dst;
  let Inst{7-0} = imm;
}

class SHOneReg<bits<8> Suffix, dag Outs, dag Ins, string Asm,
               list<dag> pattern = []>
    : SHInst<Outs, Ins, Asm> {
  bits<4> dst;

  let Pattern = pattern;
  let Inst{15-12} = 0x4;
  let Inst{11-8} = dst;
  let Inst{7-0} = Suffix;
}

class SHSpecialToReg<bits<8> Suffix, dag Outs, dag Ins, string Asm>
    : SHInst<Outs, Ins, Asm> {
  bits<4> dst;

  let Inst{15-12} = 0x0;
  let Inst{11-8} = dst;
  let Inst{7-0} = Suffix;
}

class SHRegToSpecial<bits<8> Suffix, dag Outs, dag Ins, string Asm>
	: SHInst<Outs, Ins, Asm> {
	bits<4> src;

	let Inst{15-12} = 0x4;
	let Inst{11-8} = src;
	let Inst{7-0} = Suffix;
}

class SHBranch8<bits<8> Prefix, dag Ins, string Asm, list<dag> pattern = []>
    : SHInst<(outs), Ins, Asm> {
  bits<8> target;

  let Pattern = pattern;
  let Inst{15-8} = Prefix;
  let Inst{7-0} = target;
}

class SHBranch12<bits<4> Prefix, dag Ins, string Asm, list<dag> pattern = []>
    : SHInst<(outs), Ins, Asm> {
  bits<12> target;

  let Pattern = pattern;
  let Inst{15-12} = Prefix;
  let Inst{11-0} = target;
}

class SHIndirectCall<dag Ins, string Asm, list<dag> pattern = []>
    : SHInst<(outs), Ins, Asm> {
  bits<4> callee;

  let Pattern = pattern;
  let Inst{15-12} = 0x4;
  let Inst{11-8} = callee;
  let Inst{7-0} = 0x0b;
}

class SH2AIndirectCallNoDelay<dag Ins, string Asm, list<dag> pattern = []>
	: SHInst<(outs), Ins, Asm> {
	bits<4> callee;

	let Pattern = pattern;
	let Inst{15-12} = 0x4;
	let Inst{11-8} = callee;
	let Inst{7-0} = 0x4b;
}

class SHIndirectBranch<dag Ins, string Asm, list<dag> pattern = []>
    : SHInst<(outs), Ins, Asm> {
  bits<4> target;

  let Pattern = pattern;
  let Inst{15-12} = 0x4;
  let Inst{11-8} = target;
  let Inst{7-0} = 0x2b;
}

class SHPRStack<bits<4> LowNibble, dag Outs, dag Ins, string Asm>
    : SHInst<Outs, Ins, Asm> {
  bits<4> base_wb;

  let Inst{15-12} = 0x4;
  let Inst{11-8} = base_wb;
  let Inst{7-4} = 0x2;
  let Inst{3-0} = LowNibble;
}

class SHRegStack<bits<4> Special, bits<4> LowNibble, dag Outs, dag Ins,
								 string Asm> : SHInst<Outs, Ins, Asm> {
	bits<4> base_wb;

	let Inst{15-12} = 0x4;
	let Inst{11-8} = base_wb;
	let Inst{7-4} = Special;
	let Inst{3-0} = LowNibble;
}

class SHBankLoad<dag Outs, dag Ins, string Asm> : SHInst<Outs, Ins, Asm> {
	bits<4> src;
	bits<3> bank_dst;

	let Inst{15-12} = 0x4;
	let Inst{11-8} = src;
	let Inst{7} = 1;
	let Inst{6-4} = bank_dst;
	let Inst{3-0} = 0xe;
}

class SHBankLoadStack<dag Outs, dag Ins, string Asm> : SHInst<Outs, Ins, Asm> {
	bits<4> base_wb;
	bits<3> bank_dst;

	let Inst{15-12} = 0x4;
	let Inst{11-8} = base_wb;
	let Inst{7} = 1;
	let Inst{6-4} = bank_dst;
	let Inst{3-0} = 0x7;
}

class SHBankStore<dag Outs, dag Ins, string Asm> : SHInst<Outs, Ins, Asm> {
	bits<4> dst;
	bits<3> bank_src;

	let Inst{15-12} = 0x0;
	let Inst{11-8} = dst;
	let Inst{7} = 1;
	let Inst{6-4} = bank_src;
	let Inst{3-0} = 0x2;
}

class SHBankStoreStack<dag Outs, dag Ins, string Asm> : SHInst<Outs, Ins, Asm> {
	bits<4> base_wb;
	bits<3> bank_src;

	let Inst{15-12} = 0x4;
	let Inst{11-8} = base_wb;
	let Inst{7} = 1;
	let Inst{6-4} = bank_src;
	let Inst{3-0} = 0x3;
}

class SHSystemMemory<bits<8> Suffix, dag Outs, dag Ins, string Asm>
	: SHInst<Outs, Ins, Asm> {
	bits<4> addr;

	let Inst{15-12} = 0x0;
	let Inst{11-8} = addr;
	let Inst{7-0} = Suffix;
}

class SHFRegReg<bits<4> Suffix, dag Outs, dag Ins, string Asm>
	: SHInst<Outs, Ins, Asm> {
	bits<4> dst;
	bits<4> src;

	let Inst{15-12} = 0xf;
	let Inst{11-8} = dst;
	let Inst{7-4} = src;
	let Inst{3-0} = Suffix;
}

class SHFOneReg<bits<8> Suffix, dag Outs, dag Ins, string Asm>
	: SHInst<Outs, Ins, Asm> {
	bits<4> dst;

	let Inst{15-12} = 0xf;
	let Inst{11-8} = dst;
	let Inst{7-0} = Suffix;
}

class SHDRegReg<bit DstBank, bit SrcBank, bits<4> Suffix, dag Outs, dag Ins,
	string Asm> : SHInst<Outs, Ins, Asm> {
	bits<3> dst;
	bits<3> src;

	let Inst{15-12} = 0xf;
	let Inst{11-9} = dst;
	let Inst{8} = DstBank;
	let Inst{7-5} = src;
	let Inst{4} = SrcBank;
	let Inst{3-0} = Suffix;
}

class SHDOneReg<bits<8> Suffix, dag Outs, dag Ins, string Asm>
	: SHInst<Outs, Ins, Asm> {
	bits<3> dst;

	let Inst{15-12} = 0xf;
	let Inst{11-9} = dst;
	let Inst{8} = 0;
	let Inst{7-0} = Suffix;
}

class SHFLoad<bits<4> Suffix, dag Outs, dag Ins, string Asm>
	: SHInst<Outs, Ins, Asm> {
	bits<4> dst;
	bits<4> addr;

	let Inst{15-12} = 0xf;
	let Inst{11-8} = dst;
	let Inst{7-4} = addr;
	let Inst{3-0} = Suffix;
}

class SHFStore<bits<4> Suffix, dag Outs, dag Ins, string Asm>
	: SHInst<Outs, Ins, Asm> {
	bits<4> addr;
	bits<4> src;

	let Inst{15-12} = 0xf;
	let Inst{11-8} = addr;
	let Inst{7-4} = src;
	let Inst{3-0} = Suffix;
}

class SHFLoadPostInc<dag Outs, dag Ins, string Asm>
	: SHInst<Outs, Ins, Asm> {
	bits<4> dst;
	bits<4> base_wb;

	let Inst{15-12} = 0xf;
	let Inst{11-8} = dst;
	let Inst{7-4} = base_wb;
	let Inst{3-0} = 0x9;
}

class SHFStorePreDec<dag Outs, dag Ins, string Asm>
	: SHInst<Outs, Ins, Asm> {
	bits<4> base_wb;
	bits<4> src;

	let Inst{15-12} = 0xf;
	let Inst{11-8} = base_wb;
	let Inst{7-4} = src;
	let Inst{3-0} = 0xb;
}

class SHDLoad<bit DstBank, bits<4> Suffix, dag Outs, dag Ins, string Asm>
	: SHInst<Outs, Ins, Asm> {
	bits<3> dst;
	bits<4> addr;

	let Inst{15-12} = 0xf;
	let Inst{11-9} = dst;
	let Inst{8} = DstBank;
	let Inst{7-4} = addr;
	let Inst{3-0} = Suffix;
}

class SHDStore<bit SrcBank, bits<4> Suffix, dag Outs, dag Ins, string Asm>
	: SHInst<Outs, Ins, Asm> {
	bits<4> addr;
	bits<3> src;

	let Inst{15-12} = 0xf;
	let Inst{11-8} = addr;
	let Inst{7-5} = src;
	let Inst{4} = SrcBank;
	let Inst{3-0} = Suffix;
}

class SHDLoadPostInc<bit DstBank, dag Outs, dag Ins, string Asm>
	: SHInst<Outs, Ins, Asm> {
	bits<3> dst;
	bits<4> base_wb;

	let Inst{15-12} = 0xf;
	let Inst{11-9} = dst;
	let Inst{8} = DstBank;
	let Inst{7-4} = base_wb;
	let Inst{3-0} = 0x9;
}

class SHDStorePreDec<bit SrcBank, dag Outs, dag Ins, string Asm>
	: SHInst<Outs, Ins, Asm> {
	bits<4> base_wb;
	bits<3> src;

	let Inst{15-12} = 0xf;
	let Inst{11-8} = base_wb;
	let Inst{7-5} = src;
	let Inst{4} = SrcBank;
	let Inst{3-0} = 0xb;
}

class SHFVRegReg<dag Outs, dag Ins, string Asm> : SHInst<Outs, Ins, Asm> {
	bits<2> dst;
	bits<2> src;

	let Inst{15-12} = 0xf;
	let Inst{11-10} = dst;
	let Inst{9-8} = src;
	let Inst{7-0} = 0xed;
}

class SHFTRV<dag Outs, dag Ins, string Asm> : SHInst<Outs, Ins, Asm> {
	bits<2> dst;

	let Inst{15-12} = 0xf;
	let Inst{11-10} = dst;
	let Inst{9-0} = 0x1fd;
}

class SHPCLoad<bits<4> Prefix, dag Outs, dag Ins, string Asm>
		: SHInst<Outs, Ins, Asm> {
	bits<4> dst;
	bits<8> target;

	let Inst{15-12} = Prefix;
	let Inst{11-8} = dst;
	let Inst{7-0} = target;
}

class SHIndexedLoad<bits<4> Suffix, dag Outs, dag Ins, string Asm>
		: SHInst<Outs, Ins, Asm> {
	bits<4> dst;
	bits<4> base;

	let Inst{15-12} = 0x0;
	let Inst{11-8} = dst;
	let Inst{7-4} = base;
	let Inst{3-0} = Suffix;
}

class SHIndexedStore<bits<4> Suffix, dag Outs, dag Ins, string Asm>
		: SHInst<Outs, Ins, Asm> {
	bits<4> src;
	bits<4> base;

	let Inst{15-12} = 0x0;
	let Inst{11-8} = base;
	let Inst{7-4} = src;
	let Inst{3-0} = Suffix;
}

class SHGBRDisp<bits<8> Prefix, dag Outs, dag Ins, string Asm>
		: SHInst<Outs, Ins, Asm> {
	bits<8> addr;
	bits<4> dst = 0;
	bits<4> src = 0;

	let Inst{15-8} = Prefix;
	let Inst{7-0} = addr;
}

class SHGBRImm<bits<8> Prefix, dag Outs, dag Ins, string Asm>
		: SHInst<Outs, Ins, Asm> {
	bits<8> imm;
	bits<4> addr = 0;

	let Inst{15-8} = Prefix;
	let Inst{7-0} = imm;
}

class SHR0Imm<bits<8> Prefix, dag Outs, dag Ins, string Asm>
		: SHInst<Outs, Ins, Asm> {
	bits<8> imm;

	let Inst{15-8} = Prefix;
	let Inst{7-0} = imm;
}

class SHRegRelative<bits<8> Suffix, dag Outs, dag Ins, string Asm>
		: SHInst<Outs, Ins, Asm> {
	bits<4> target;

	let Inst{15-12} = 0x0;
	let Inst{11-8} = target;
	let Inst{7-0} = Suffix;
}

class SHPostIncLoad<bits<4> Suffix, dag Outs, dag Ins, string Asm>
		: SHInst<Outs, Ins, Asm> {
	bits<4> dst;
	bits<4> base_wb;

	let Inst{15-12} = 0x6;
	let Inst{11-8} = dst;
	let Inst{7-4} = base_wb;
	let Inst{3-0} = Suffix;
}

class SHPreDecStore<bits<4> Suffix, dag Outs, dag Ins, string Asm>
		: SHInst<Outs, Ins, Asm> {
	bits<4> src;
	bits<4> base_wb;

	let Inst{15-12} = 0x2;
	let Inst{11-8} = base_wb;
	let Inst{7-4} = src;
	let Inst{3-0} = Suffix;
}

class SHMAC<bits<4> Prefix, bits<4> Suffix, dag Outs, dag Ins, string Asm>
		: SHInst<Outs, Ins, Asm> {
	bits<4> lhs_wb;
	bits<4> rhs_wb;

	let Inst{15-12} = Prefix;
	let Inst{11-8} = rhs_wb;
	let Inst{7-4} = lhs_wb;
	let Inst{3-0} = Suffix;
}

class SHLongLoadReg<dag Outs, dag Ins, string Asm, list<dag> pattern = []>
    : SHInst<Outs, Ins, Asm> {
  bits<4> dst;
  bits<4> addr;

  let Pattern = pattern;
  let Inst{15-12} = 0x6;
  let Inst{11-8} = dst;
  let Inst{7-4} = addr;
  let Inst{3-0} = 0x2;
}

class SHLongLoadIndexed<dag Outs, dag Ins, string Asm>
    : SHInst<Outs, Ins, Asm> {
  bits<4> dst;
  bits<4> base;

  let Inst{15-12} = 0x0;
  let Inst{11-8} = dst;
  let Inst{7-4} = base;
  let Inst{3-0} = 0xe;
}

class SHLongLoadPC<dag Outs, dag Ins, string Asm, list<dag> pattern = []>
    : SHInst<Outs, Ins, Asm> {
  bits<4> dst;
  bits<8> target;

  let Pattern = pattern;
  let Inst{15-12} = 0xd;
  let Inst{11-8} = dst;
  let Inst{7-0} = target;
}

class SHMOVA<dag Outs, dag Ins, string Asm>
    : SHInst<Outs, Ins, Asm> {
  bits<4> dst = 0;
  bits<8> target;

  let Inst{15-8} = 0xc7;
  let Inst{7-0} = target;
}

class SHLongStoreReg<dag Outs, dag Ins, string Asm, list<dag> pattern = []>
    : SHInst<Outs, Ins, Asm> {
  bits<4> src;
  bits<4> addr;

  let Pattern = pattern;
  let Inst{15-12} = 0x2;
  let Inst{11-8} = addr;
  let Inst{7-4} = src;
  let Inst{3-0} = 0x2;
}

class SHLongLoadDisp<dag Outs, dag Ins, string Asm, list<dag> pattern = []>
    : SHInst<Outs, Ins, Asm> {
  bits<4> dst;
  bits<8> addr;

  let Pattern = pattern;
  let Inst{15-12} = 0x5;
  let Inst{11-8} = dst;
  let Inst{7-4} = addr{7-4};
  let Inst{3-0} = addr{3-0};
}

class SHLongStoreDisp<dag Outs, dag Ins, string Asm, list<dag> pattern = []>
    : SHInst<Outs, Ins, Asm> {
  bits<4> src;
  bits<8> addr;

  let Pattern = pattern;
  let Inst{15-12} = 0x1;
  let Inst{11-8} = addr{7-4};
  let Inst{7-4} = src;
  let Inst{3-0} = addr{3-0};
}

class SHNarrowLoadReg<bits<4> Suffix, dag Outs, dag Ins, string Asm,
                      list<dag> pattern = []>
    : SHInst<Outs, Ins, Asm> {
  bits<4> dst;
  bits<4> addr;

  let Pattern = pattern;
  let Inst{15-12} = 0x6;
  let Inst{11-8} = dst;
  let Inst{7-4} = addr;
  let Inst{3-0} = Suffix;
}

class SHNarrowStoreReg<bits<4> Suffix, dag Outs, dag Ins, string Asm,
                       list<dag> pattern = []>
    : SHInst<Outs, Ins, Asm> {
  bits<4> src;
  bits<4> addr;

  let Pattern = pattern;
  let Inst{15-12} = 0x2;
  let Inst{11-8} = addr;
  let Inst{7-4} = src;
  let Inst{3-0} = Suffix;
}

class SHNarrowLoadDisp<bits<8> Prefix, dag Outs, dag Ins, string Asm>
    : SHInst<Outs, Ins, Asm> {
  bits<4> dst = 0;
  bits<8> addr;

  let Inst{15-8} = Prefix;
  let Inst{7-4} = addr{7-4};
  let Inst{3-0} = addr{3-0};
}

class SHNarrowStoreDisp<bits<8> Prefix, dag Outs, dag Ins, string Asm>
    : SHInst<Outs, Ins, Asm> {
  bits<4> src = 0;
  bits<8> addr;

  let Inst{15-8} = Prefix;
  let Inst{7-4} = addr{7-4};
  let Inst{3-0} = addr{3-0};
}
