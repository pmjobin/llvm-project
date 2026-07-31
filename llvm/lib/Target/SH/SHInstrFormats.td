//===-- SHInstrFormats.td - SuperH instruction formats -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class SHInst<dag Outs, dag Ins, string Asm> : Instruction {
  field bits<16> Inst;

  let Namespace = "SH";
  let OutOperandList = Outs;
  let InOperandList = Ins;
  let AsmString = Asm;
  let Size = 2;
  let Pattern = [];
  let mayLoad = 0;
  let mayStore = 0;
  let hasSideEffects = 0;
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
