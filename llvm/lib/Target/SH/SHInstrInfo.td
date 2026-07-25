//===-- SHInstrInfo.td - SuperH instruction definitions ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "SHInstrFormats.td"

def SImm8AsmOperand : AsmOperandClass {
  let Name = "SImm8";
  let PredicateMethod = "isSImm8";
  let RenderMethod = "addImmOperands";
  let ParserMethod = "parseSImm8";
  let DiagnosticType = "InvalidSImm8";
}

def simm8 : Operand<i32>, ImmLeaf<i32, [{ return isInt<8>(Imm); }]> {
  let ParserMatchClass = SImm8AsmOperand;
  let PrintMethod = "printSImm8";
  let DecoderMethod = "decodeSImm8";
}

def LongMemRegAsmOperand : AsmOperandClass {
  let Name = "LongMemReg";
  let PredicateMethod = "isLongMemReg";
  let RenderMethod = "addLongMemRegOperands";
}

def LongMemDispAsmOperand : AsmOperandClass {
  let Name = "LongMemDisp";
  let PredicateMethod = "isLongMemDisp";
  let RenderMethod = "addLongMemDispOperands";
}

def longmemreg : Operand<iPTR> {
  let MIOperandInfo = (ops GPR);
  let ParserMatchClass = LongMemRegAsmOperand;
  let PrintMethod = "printLongMemReg";
  let DecoderMethod = "DecodeGPRRegisterClass";
}

def longmemdisp : Operand<iPTR> {
  let MIOperandInfo = (ops GPR, i32imm);
  let ParserMatchClass = LongMemDispAsmOperand;
  let PrintMethod = "printLongMemDisp";
  let EncoderMethod = "getLongDispMemOpValue";
  let DecoderMethod = "decodeLongDispMemOperand";
}

def addr_reg : ComplexPattern<iPTR, 1, "SelectAddrReg">;
def addr_disp : ComplexPattern<iPTR, 2, "SelectAddrDisp", [add, frameindex]>;

def SHret : SDNode<"SHISD::RET_GLUE", SDTNone,
                   [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

def NOP : SHFixed<0x0009, "nop">;

let isReturn = 1, isTerminator = 1, isBarrier = 1, hasDelaySlot = 1,
    Uses = [PR] in
def RTS : SHFixed<0x000b, "rts"> {
  let Pattern = [(SHret)];
}

def MOVrr : SHRegReg<0x6, 0x3, (outs GPR:$dst), (ins GPR:$src),
                     "mov\t$src,$dst">;

let Constraints = "$dst = $dst_in", isCommutable = 1 in
def ADDrr : SHRegReg<0x3, 0xc, (outs GPR:$dst),
                     (ins GPR:$dst_in, GPR:$src), "add\t$src,$dst",
                     [(set GPR:$dst, (add GPR:$dst_in, GPR:$src))]>;

def MOVri : SHRegImm<0xe, (outs GPR:$dst), (ins simm8:$imm),
                     "mov\t$imm,$dst", [(set GPR:$dst, simm8:$imm)]>;

let Constraints = "$dst = $dst_in" in
def ADDri : SHRegImm<0x7, (outs GPR:$dst),
                     (ins GPR:$dst_in, simm8:$imm), "add\t$imm,$dst",
                     [(set GPR:$dst, (add GPR:$dst_in, simm8:$imm))]>;

let mayLoad = 1 in
def MOVL_load_reg
    : SHLongLoadReg<(outs GPR:$dst), (ins longmemreg:$addr),
                    "mov.l\t$addr,$dst",
                    [(set GPR:$dst, (load addr_reg:$addr))]>;

let mayStore = 1 in
def MOVL_store_reg
    : SHLongStoreReg<(outs), (ins GPR:$src, longmemreg:$addr),
                     "mov.l\t$src,$addr",
                     [(store GPR:$src, addr_reg:$addr)]>;

let mayLoad = 1 in
def MOVL_load_disp
    : SHLongLoadDisp<(outs GPR:$dst), (ins longmemdisp:$addr),
                     "mov.l\t$addr,$dst",
                     [(set GPR:$dst, (load addr_disp:$addr))]>;

let mayStore = 1 in
def MOVL_store_disp
    : SHLongStoreDisp<(outs), (ins GPR:$src, longmemdisp:$addr),
                      "mov.l\t$src,$addr",
                      [(store GPR:$src, addr_disp:$addr)]>;
