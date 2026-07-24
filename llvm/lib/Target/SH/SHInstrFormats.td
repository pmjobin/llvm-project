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

class SHRegReg<bits<4> Prefix, bits<4> Suffix, dag Outs, dag Ins, string Asm>
    : SHInst<Outs, Ins, Asm> {
  bits<4> dst;
  bits<4> src;

  let Inst{15-12} = Prefix;
  let Inst{11-8} = dst;
  let Inst{7-4} = src;
  let Inst{3-0} = Suffix;
}

class SHRegImm<bits<4> Prefix, dag Outs, dag Ins, string Asm>
    : SHInst<Outs, Ins, Asm> {
  bits<4> dst;
  bits<8> imm;

  let Inst{15-12} = Prefix;
  let Inst{11-8} = dst;
  let Inst{7-0} = imm;
}
