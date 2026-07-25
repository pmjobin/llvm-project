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
