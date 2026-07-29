//===-- SHInstrInfo.td - SuperH instruction definitions ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "SHInstrFormats.td"

def BranchTargetAsmOperand : AsmOperandClass {
  let Name = "BranchTarget";
  let PredicateMethod = "isBranchTarget";
  let RenderMethod = "addBranchTargetOperands";
  let ParserMethod = "parseBranchTarget";
}

def brtarget8 : Operand<OtherVT> {
  let ParserMatchClass = BranchTargetAsmOperand;
  let PrintMethod = "printBranchTarget";
  let EncoderMethod = "getBranchTargetOpValue";
  let DecoderMethod = "decodeBranchTarget8";
  let OperandType = "OPERAND_PCREL";
}

def brtarget12 : Operand<OtherVT> {
  let ParserMatchClass = BranchTargetAsmOperand;
  let PrintMethod = "printBranchTarget";
  let EncoderMethod = "getBranchTargetOpValue";
  let DecoderMethod = "decodeBranchTarget12";
  let OperandType = "OPERAND_PCREL";
}

def calltarget12 : Operand<i32> {
  let ParserMatchClass = BranchTargetAsmOperand;
  let PrintMethod = "printBranchTarget";
  let EncoderMethod = "getBranchTargetOpValue";
  let DecoderMethod = "decodeBranchTarget12";
  let OperandType = "OPERAND_PCREL";
}

def PCLiteralAsmOperand : AsmOperandClass {
  let Name = "PCLiteral";
  let PredicateMethod = "isPCLiteral";
  let RenderMethod = "addPCLiteralOperands";
  let ParserMethod = "parsePCLiteral";
  let DiagnosticType = "InvalidPCLiteral";
}

def pcliteral : Operand<iPTR> {
  let ParserMatchClass = PCLiteralAsmOperand;
  let PrintMethod = "printPCLiteral";
  let EncoderMethod = "getPCLiteralOpValue";
  let DecoderMethod = "decodePCLiteral";
  let OperandType = "OPERAND_PCREL";
}

def island_instance : OperandWithDefaultOps<i32, (ops (i32 -1))>;

def cpinst_operand : Operand<i32>;

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

def ByteMemDispAsmOperand : AsmOperandClass {
  let Name = "ByteMemDisp";
  let PredicateMethod = "isByteMemDisp";
  let RenderMethod = "addNarrowMemDispOperands";
}

def WordMemDispAsmOperand : AsmOperandClass {
  let Name = "WordMemDisp";
  let PredicateMethod = "isWordMemDisp";
  let RenderMethod = "addNarrowMemDispOperands";
}

def R0AsmOperand : AsmOperandClass {
  let Name = "R0";
  let PredicateMethod = "isR0";
  let RenderMethod = "addRegOperands";
  let DiagnosticType = "InvalidR0";
}

def PreDecGPRAsmOperand : AsmOperandClass {
  let Name = "PreDecGPR";
  let PredicateMethod = "isPreDecGPR";
  let RenderMethod = "addPreDecGPROperands";
}

def PostIncGPRAsmOperand : AsmOperandClass {
  let Name = "PostIncGPR";
  let PredicateMethod = "isPostIncGPR";
  let RenderMethod = "addPostIncGPROperands";
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

def bytememdisp : Operand<iPTR> {
  let MIOperandInfo = (ops GPR, i32imm);
  let ParserMatchClass = ByteMemDispAsmOperand;
  let PrintMethod = "printNarrowMemDisp";
  let EncoderMethod = "getNarrowDispMemOpValue";
  let DecoderMethod = "decodeByteDispMemOperand";
}

def wordmemdisp : Operand<iPTR> {
  let MIOperandInfo = (ops GPR, i32imm);
  let ParserMatchClass = WordMemDispAsmOperand;
  let PrintMethod = "printNarrowMemDisp";
  let EncoderMethod = "getNarrowDispMemOpValue";
  let DecoderMethod = "decodeWordDispMemOperand";
}

def r0reg : RegisterOperand<GPR> {
  let ParserMatchClass = R0AsmOperand;
  let DecoderMethod = "decodeR0RegisterClass";
}

def predecgpr : Operand<iPTR> {
  let MIOperandInfo = (ops GPR);
  let ParserMatchClass = PreDecGPRAsmOperand;
  let PrintMethod = "printPreDecGPR";
  let DecoderMethod = "DecodeGPRRegisterClass";
}

def postincgpr : Operand<iPTR> {
  let MIOperandInfo = (ops GPR);
  let ParserMatchClass = PostIncGPRAsmOperand;
  let PrintMethod = "printPostIncGPR";
  let DecoderMethod = "DecodeGPRRegisterClass";
}

def addr_reg : ComplexPattern<iPTR, 1, "SelectAddrReg">;
def addr_disp : ComplexPattern<iPTR, 2, "SelectAddrDisp", [add, frameindex]>;
def addr_narrow_reg : ComplexPattern<iPTR, 1, "SelectNarrowAddrReg", [add]>;
def addr_byte_frame
    : ComplexPattern<iPTR, 2, "SelectByteFrameAddr", [add, frameindex]>;
def addr_word_frame
    : ComplexPattern<iPTR, 2, "SelectWordFrameAddr", [add, frameindex]>;

def SDT_SHCmp : SDTypeProfile<0, 2, [SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;
def SDT_SHBrcond : SDTypeProfile<0, 1, [SDTCisVT<0, OtherVT>]>;
def SDT_SHCall : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;
def SDT_SHDivRem
    : SDTypeProfile<2, 2, [SDTCisVT<0, i32>, SDTCisSameAs<0, 1>,
                           SDTCisSameAs<0, 2>, SDTCisSameAs<0, 3>]>;
def SDT_SHShiftParts
    : SDTypeProfile<2, 3, [SDTCisVT<0, i32>, SDTCisSameAs<0, 1>,
                           SDTCisSameAs<0, 2>, SDTCisSameAs<0, 3>,
                           SDTCisSameAs<0, 4>]>;
def SDT_SHSetCC64
    : SDTypeProfile<1, 5, [SDTCisVT<0, i32>, SDTCisSameAs<0, 1>,
                           SDTCisSameAs<0, 2>, SDTCisSameAs<0, 3>,
                           SDTCisSameAs<0, 4>, SDTCisSameAs<0, 5>]>;
def SDT_SHCallSeqStart
    : SDCallSeqStart<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;
def SDT_SHCallSeqEnd
    : SDCallSeqEnd<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;
def SDT_SHBrJT
    : SDTypeProfile<0, 3, [SDTCisVT<0, i32>, SDTCisVT<1, i32>,
                           SDTCisVT<2, i32>]>;

def SHret : SDNode<"SHISD::RET_GLUE", SDTNone,
                   [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def SHcall : SDNode<"SHISD::CALL", SDT_SHCall,
                    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def SHcallseq_start : SDNode<"ISD::CALLSEQ_START", SDT_SHCallSeqStart,
                            [SDNPHasChain, SDNPOutGlue]>;
def SHcallseq_end : SDNode<"ISD::CALLSEQ_END", SDT_SHCallSeqEnd,
                          [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;
def SHcmpeq : SDNode<"SHISD::CMP_EQ", SDT_SHCmp, [SDNPOutGlue]>;
def SHcmphs : SDNode<"SHISD::CMP_HS", SDT_SHCmp, [SDNPOutGlue]>;
def SHcmpge : SDNode<"SHISD::CMP_GE", SDT_SHCmp, [SDNPOutGlue]>;
def SHcmphi : SDNode<"SHISD::CMP_HI", SDT_SHCmp, [SDNPOutGlue]>;
def SHcmpgt : SDNode<"SHISD::CMP_GT", SDT_SHCmp, [SDNPOutGlue]>;
def SHtst : SDNode<"SHISD::TST", SDT_SHCmp, [SDNPOutGlue]>;
def SHbt : SDNode<"SHISD::BT", SDT_SHBrcond, [SDNPHasChain, SDNPInGlue]>;
def SHbf : SDNode<"SHISD::BF", SDT_SHBrcond, [SDNPHasChain, SDNPInGlue]>;
def SHudiv : SDNode<"SHISD::UDIV", SDTIntBinOp>;
def SHurem : SDNode<"SHISD::UREM", SDTIntBinOp>;
def SHudivrem : SDNode<"SHISD::UDIVREM", SDT_SHDivRem>;
def SHsdiv : SDNode<"SHISD::SDIV", SDTIntBinOp>;
def SHsrem : SDNode<"SHISD::SREM", SDTIntBinOp>;
def SHsdivrem : SDNode<"SHISD::SDIVREM", SDT_SHDivRem>;
def SHshlparts : SDNode<"SHISD::SHL_PARTS", SDT_SHShiftParts>;
def SHsrlparts : SDNode<"SHISD::SRL_PARTS", SDT_SHShiftParts>;
def SHsraparts : SDNode<"SHISD::SRA_PARTS", SDT_SHShiftParts>;
def SHsetcc64 : SDNode<"SHISD::SETCC64", SDT_SHSetCC64>;
def SHbrjt
    : SDNode<"SHISD::BR_JT", SDT_SHBrJT,
             [SDNPHasChain, SDNPMayLoad, SDNPMemOperand]>;

def non_simm8 : ImmLeaf<i32, [{ return !isInt<8>(Imm); }]>;
def u5imm : ImmLeaf<i32, [{ return isUInt<5>(Imm); }]>;

def NOP : SHFixed<0x0009, "nop">;

let Defs = [TBit] in
def CLRT : SHFixed<0x0008, "clrt">;

let isCodeGenOnly = 1, Uses = [R15], Defs = [R15] in {
  def ADJCALLSTACKDOWN
      : SHPseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2)> {
    let Pattern = [(SHcallseq_start timm:$amt1, timm:$amt2)];
  }
  def ADJCALLSTACKUP
      : SHPseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2)> {
    let Pattern = [(SHcallseq_end timm:$amt1, timm:$amt2)];
  }
}

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

let Constraints = "$dst = $dst_in" in
def SUBrr : SHRegReg<0x3, 0x8, (outs GPR:$dst),
                     (ins GPR:$dst_in, GPR:$src), "sub\t$src,$dst",
                     [(set GPR:$dst, (sub GPR:$dst_in, GPR:$src))]>;

def NEG : SHRegReg<0x6, 0xb, (outs GPR:$dst), (ins GPR:$src),
                   "neg\t$src,$dst",
                   [(set GPR:$dst, (ineg GPR:$src))]>;

let Constraints = "$dst = $dst_in", isCommutable = 1 in {
  def ANDrr : SHRegReg<0x2, 0x9, (outs GPR:$dst),
                       (ins GPR:$dst_in, GPR:$src), "and\t$src,$dst",
                       [(set GPR:$dst, (and GPR:$dst_in, GPR:$src))]>;
  def ORrr : SHRegReg<0x2, 0xb, (outs GPR:$dst),
                      (ins GPR:$dst_in, GPR:$src), "or\t$src,$dst",
                      [(set GPR:$dst, (or GPR:$dst_in, GPR:$src))]>;
  def XORrr : SHRegReg<0x2, 0xa, (outs GPR:$dst),
                       (ins GPR:$dst_in, GPR:$src), "xor\t$src,$dst",
                       [(set GPR:$dst, (xor GPR:$dst_in, GPR:$src))]>;
}

def NOT : SHRegReg<0x6, 0x7, (outs GPR:$dst), (ins GPR:$src),
                   "not\t$src,$dst",
                   [(set GPR:$dst, (not GPR:$src))]>;

let Defs = [TBit], isCompare = 1 in
def TST : SHRegReg<0x2, 0x8, (outs), (ins GPR:$src, GPR:$dst),
                   "tst\t$src,$dst", [(SHtst GPR:$dst, GPR:$src)]>;

let Constraints = "$dst = $dst_in", Defs = [TBit] in {
  def DT : SHOneReg<0x10, (outs GPR:$dst), (ins GPR:$dst_in), "dt\t$dst">;
  def SHLL
      : SHOneReg<0x00, (outs GPR:$dst), (ins GPR:$dst_in), "shll\t$dst">;
  def SHLR
      : SHOneReg<0x01, (outs GPR:$dst), (ins GPR:$dst_in), "shlr\t$dst">;
  def SHAR
      : SHOneReg<0x21, (outs GPR:$dst), (ins GPR:$dst_in), "shar\t$dst">;
}

let Constraints = "$dst = $dst_in" in {
  def SHLL2
      : SHOneReg<0x08, (outs GPR:$dst), (ins GPR:$dst_in), "shll2\t$dst">;
  def SHLR2
      : SHOneReg<0x09, (outs GPR:$dst), (ins GPR:$dst_in), "shlr2\t$dst">;
  def SHLL8
      : SHOneReg<0x18, (outs GPR:$dst), (ins GPR:$dst_in), "shll8\t$dst">;
  def SHLR8
      : SHOneReg<0x19, (outs GPR:$dst), (ins GPR:$dst_in), "shlr8\t$dst">;
  def SHLL16
      : SHOneReg<0x28, (outs GPR:$dst), (ins GPR:$dst_in), "shll16\t$dst">;
  def SHLR16
      : SHOneReg<0x29, (outs GPR:$dst), (ins GPR:$dst_in), "shlr16\t$dst">;
}

def MOVri : SHRegImm<0xe, (outs GPR:$dst), (ins simm8:$imm),
                     "mov\t$imm,$dst", [(set GPR:$dst, simm8:$imm)]>;

def LEA_FI : SHPseudo<(outs GPR:$dst), (ins i32imm:$fi, i32imm:$disp)>;

let Constraints = "$dst = $dst_in" in
def ADDri : SHRegImm<0x7, (outs GPR:$dst),
                     (ins GPR:$dst_in, simm8:$imm), "add\t$imm,$dst",
                     [(set GPR:$dst, (add GPR:$dst_in, simm8:$imm))]>;

let usesCustomInserter = 1 in {
  def ADDC_LO_PSEUDO
      : SHPseudo<(outs GPR:$dst), (ins GPR:$lhs, GPR:$rhs)> {
    let Pattern = [(set GPR:$dst, (addc GPR:$lhs, GPR:$rhs))];
    let Defs = [TBit];
  }
  def SUBC_LO_PSEUDO
      : SHPseudo<(outs GPR:$dst), (ins GPR:$lhs, GPR:$rhs)> {
    let Pattern = [(set GPR:$dst, (subc GPR:$lhs, GPR:$rhs))];
    let Defs = [TBit];
  }
  let isCommutable = 1 in
  def MUL32_PSEUDO
      : SHPseudo<(outs GPR:$dst), (ins GPR:$lhs, GPR:$rhs)> {
    let Pattern = [(set GPR:$dst, (mul GPR:$lhs, GPR:$rhs))];
  }
  def UDIV32_PSEUDO
      : SHPseudo<(outs GPR:$quotient),
                 (ins GPR:$dividend, GPR:$divisor)> {
    let Pattern =
        [(set GPR:$quotient, (SHudiv GPR:$dividend, GPR:$divisor))];
  }
  def UREM32_PSEUDO
      : SHPseudo<(outs GPR:$remainder),
                 (ins GPR:$dividend, GPR:$divisor)> {
    let Pattern =
        [(set GPR:$remainder, (SHurem GPR:$dividend, GPR:$divisor))];
  }
  def UDIVREM32_PSEUDO
      : SHPseudo<(outs GPR:$quotient, GPR:$remainder),
                 (ins GPR:$dividend, GPR:$divisor)> {
    let Pattern =
        [(set GPR:$quotient, GPR:$remainder,
              (SHudivrem GPR:$dividend, GPR:$divisor))];
  }
  def SDIV32_PSEUDO
      : SHPseudo<(outs GPR:$quotient),
                 (ins GPR:$dividend, GPR:$divisor)> {
    let Pattern =
        [(set GPR:$quotient, (SHsdiv GPR:$dividend, GPR:$divisor))];
  }
  def SREM32_PSEUDO
      : SHPseudo<(outs GPR:$remainder),
                 (ins GPR:$dividend, GPR:$divisor)> {
    let Pattern =
        [(set GPR:$remainder, (SHsrem GPR:$dividend, GPR:$divisor))];
  }
  def SDIVREM32_PSEUDO
      : SHPseudo<(outs GPR:$quotient, GPR:$remainder),
                 (ins GPR:$dividend, GPR:$divisor)> {
    let Pattern =
        [(set GPR:$quotient, GPR:$remainder,
              (SHsdivrem GPR:$dividend, GPR:$divisor))];
  }
  def MOVi32 : SHPseudo<(outs GPR:$dst), (ins i32imm:$imm)> {
    let Pattern = [(set GPR:$dst, non_simm8:$imm)];
  }
  def SHLri : SHPseudo<(outs GPR:$dst), (ins GPR:$src, i32imm:$amount)> {
    let Pattern = [(set GPR:$dst, (shl GPR:$src, u5imm:$amount))];
  }
  def SRLri : SHPseudo<(outs GPR:$dst), (ins GPR:$src, i32imm:$amount)> {
    let Pattern = [(set GPR:$dst, (srl GPR:$src, u5imm:$amount))];
  }
  def SRAri : SHPseudo<(outs GPR:$dst), (ins GPR:$src, i32imm:$amount)> {
    let Pattern = [(set GPR:$dst, (sra GPR:$src, u5imm:$amount))];
  }
  def SHLrr : SHPseudo<(outs GPR:$dst), (ins GPR:$src, GPR:$amount)> {
    let Pattern = [(set GPR:$dst, (shl GPR:$src, GPR:$amount))];
  }
  def SRLrr : SHPseudo<(outs GPR:$dst), (ins GPR:$src, GPR:$amount)> {
    let Pattern = [(set GPR:$dst, (srl GPR:$src, GPR:$amount))];
  }
  def SRArr : SHPseudo<(outs GPR:$dst), (ins GPR:$src, GPR:$amount)> {
    let Pattern = [(set GPR:$dst, (sra GPR:$src, GPR:$amount))];
  }
  def SHL64_PSEUDO
      : SHPseudo<(outs GPR:$lo_dst, GPR:$hi_dst),
                 (ins GPR:$lo, GPR:$hi, GPR:$amount)> {
    let Pattern =
        [(set GPR:$lo_dst, GPR:$hi_dst,
              (SHshlparts GPR:$lo, GPR:$hi, GPR:$amount))];
  }
  def SRL64_PSEUDO
      : SHPseudo<(outs GPR:$lo_dst, GPR:$hi_dst),
                 (ins GPR:$lo, GPR:$hi, GPR:$amount)> {
    let Pattern =
        [(set GPR:$lo_dst, GPR:$hi_dst,
              (SHsrlparts GPR:$lo, GPR:$hi, GPR:$amount))];
  }
  def SRA64_PSEUDO
      : SHPseudo<(outs GPR:$lo_dst, GPR:$hi_dst),
                 (ins GPR:$lo, GPR:$hi, GPR:$amount)> {
    let Pattern =
        [(set GPR:$lo_dst, GPR:$hi_dst,
              (SHsraparts GPR:$lo, GPR:$hi, GPR:$amount))];
  }
}

let usesCustomInserter = 1, isBranch = 1, isTerminator = 1 in
def BR_CC64_PSEUDO
    : SHPseudo<(outs),
               (ins GPR:$lhs_lo, GPR:$lhs_hi, GPR:$rhs_lo, GPR:$rhs_hi,
                    i32imm:$cc, brtarget8:$target)>;

let isCommutable = 1, Defs = [MACL] in
def MUL_L : SHRegReg<0x0, 0x7, (outs), (ins GPR:$src, GPR:$dst),
                     "mul.l\t$src,$dst">;

let Uses = [MACL] in
def STS_MACL
    : SHSpecialToReg<0x1a, (outs GPR:$dst), (ins), "sts\tmacl,$dst">;

let Defs = [MBit, QBit, TBit] in
def DIV0U : SHFixed<0x0019, "div0u">;

let Defs = [MBit, QBit, TBit] in
def DIV0S : SHRegReg<0x2, 0x7, (outs), (ins GPR:$src, GPR:$dst),
                     "div0s\t$src,$dst">;

let Constraints = "$dst = $dst_in", Uses = [MBit, QBit, TBit],
    Defs = [QBit, TBit] in
def DIV1 : SHRegReg<0x3, 0x4, (outs GPR:$dst),
                    (ins GPR:$dst_in, GPR:$src), "div1\t$src,$dst">;

let Constraints = "$dst = $dst_in", Uses = [TBit], Defs = [TBit] in
def ROTCL
    : SHOneReg<0x24, (outs GPR:$dst), (ins GPR:$dst_in), "rotcl\t$dst">;

let Constraints = "$dst = $dst_in", Uses = [TBit], Defs = [TBit] in
def ROTCR
    : SHOneReg<0x25, (outs GPR:$dst), (ins GPR:$dst_in), "rotcr\t$dst">;

let Constraints = "$dst = $dst_in", Uses = [TBit], Defs = [TBit] in {
  def ADDC : SHRegReg<0x3, 0xe, (outs GPR:$dst),
                      (ins GPR:$dst_in, GPR:$src), "addc\t$src,$dst",
                      [(set GPR:$dst, (adde GPR:$dst_in, GPR:$src))]>;
  def SUBC : SHRegReg<0x3, 0xa, (outs GPR:$dst),
                      (ins GPR:$dst_in, GPR:$src), "subc\t$src,$dst",
                      [(set GPR:$dst, (sube GPR:$dst_in, GPR:$src))]>;
}

let Defs = [TBit], isCompare = 1 in {
  def CMP_EQ : SHRegReg<0x3, 0x0, (outs), (ins GPR:$src, GPR:$dst),
                        "cmp/eq\t$src,$dst",
                        [(SHcmpeq GPR:$dst, GPR:$src)]>;
  def CMP_HS : SHRegReg<0x3, 0x2, (outs), (ins GPR:$src, GPR:$dst),
                        "cmp/hs\t$src,$dst",
                        [(SHcmphs GPR:$dst, GPR:$src)]>;
  def CMP_GE : SHRegReg<0x3, 0x3, (outs), (ins GPR:$src, GPR:$dst),
                        "cmp/ge\t$src,$dst",
                        [(SHcmpge GPR:$dst, GPR:$src)]>;
  def CMP_HI : SHRegReg<0x3, 0x6, (outs), (ins GPR:$src, GPR:$dst),
                        "cmp/hi\t$src,$dst",
                        [(SHcmphi GPR:$dst, GPR:$src)]>;
  def CMP_GT : SHRegReg<0x3, 0x7, (outs), (ins GPR:$src, GPR:$dst),
                        "cmp/gt\t$src,$dst",
                        [(SHcmpgt GPR:$dst, GPR:$src)]>;
}

let isBranch = 1, isTerminator = 1, Uses = [TBit] in {
  def BT : SHBranch8<0x89, (ins brtarget8:$target), "bt\t$target",
                     [(SHbt bb:$target)]>;
  def BF : SHBranch8<0x8b, (ins brtarget8:$target), "bf\t$target",
                     [(SHbf bb:$target)]>;
}

let isBranch = 1, isTerminator = 1, isBarrier = 1, hasDelaySlot = 1 in
def BRA : SHBranch12<0xa, (ins brtarget12:$target), "bra\t$target",
                     [(br bb:$target)]>;

let isBranch = 1, isIndirectBranch = 1, isTerminator = 1, isBarrier = 1,
    hasDelaySlot = 1 in
def JMP : SHIndirectBranch<(ins GPR:$target, variable_ops), "jmp\t@$target",
                           [(brind GPR:$target)]>;

let usesCustomInserter = 1, isBranch = 1, isIndirectBranch = 1,
    isTerminator = 1, isBarrier = 1, mayLoad = 1 in
def SH_JT_DISPATCH
    : SHPseudo<(outs), (ins GPR:$index, GPR:$base, i32imm:$jti)> {
  let Pattern = [(SHbrjt GPR:$index, GPR:$base, tjumptable:$jti)];
}

let isCall = 1, hasDelaySlot = 1, Defs = [PR] in {
  def BSR : SHBranch12<0xb, (ins calltarget12:$target), "bsr\t$target",
                       [(SHcall tglobaladdr:$target)]>;
  def JSR : SHIndirectCall<(ins GPR:$callee), "jsr\t@$callee",
                           [(SHcall GPR:$callee)]>;
}

let mayStore = 1, Uses = [PR], Constraints = "$base_wb = $base" in
def STS_L_PR
    : SHPRStack<0x2, (outs predecgpr:$base_wb), (ins GPR:$base),
                "sts.l\tpr,$base_wb">;

let mayLoad = 1, Defs = [PR], Constraints = "$base_wb = $base" in
def LDS_L_PR
    : SHPRStack<0x6, (outs postincgpr:$base_wb), (ins GPR:$base),
                "lds.l\t$base_wb,pr">;

let mayLoad = 1 in
def MOVL_load_reg
    : SHLongLoadReg<(outs GPR:$dst), (ins longmemreg:$addr),
                    "mov.l\t$addr,$dst",
                    [(set GPR:$dst, (load addr_reg:$addr))]>;

let mayLoad = 1 in
def MOVL_load_pc
    : SHLongLoadPC<(outs GPR:$dst), (ins pcliteral:$target),
                   "mov.l\t$target,$dst",
                   []>;

let mayLoad = 1 in
def MOVL_load_pc_island
    : SHPseudo<(outs GPR:$dst),
               (ins pcliteral:$target, island_instance:$island)> {
  let Pattern = [(set GPR:$dst, (load tconstpool:$target))];
}

let hasSideEffects = 0, isNotDuplicable = 1 in
def SH_CONSTPOOL_ENTRY
    : SHPseudo<(outs),
               (ins cpinst_operand:$cpi, i32imm:$instance, i32imm:$size,
                    i32imm:$alignment)>;

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

let mayStore = 1 in {
  def MOVB_store_reg
      : SHNarrowStoreReg<0x0, (outs), (ins GPR:$src, longmemreg:$addr),
                         "mov.b\t$src,$addr",
                         [(truncstorei8 GPR:$src,
                                          addr_narrow_reg:$addr)]>;
  def MOVW_store_reg
      : SHNarrowStoreReg<0x1, (outs), (ins GPR:$src, longmemreg:$addr),
                         "mov.w\t$src,$addr",
                         [(truncstorei16 GPR:$src,
                                           addr_narrow_reg:$addr)]>;
}

let mayLoad = 1 in {
  def MOVB_load_reg
      : SHNarrowLoadReg<0x0, (outs GPR:$dst), (ins longmemreg:$addr),
                        "mov.b\t$addr,$dst",
                        [(set GPR:$dst,
                              (sextloadi8 addr_narrow_reg:$addr))]>;
  def MOVW_load_reg
      : SHNarrowLoadReg<0x1, (outs GPR:$dst), (ins longmemreg:$addr),
                        "mov.w\t$addr,$dst",
                        [(set GPR:$dst,
                              (sextloadi16 addr_narrow_reg:$addr))]>;
}

let mayStore = 1 in {
  def MOVB_store_disp
      : SHNarrowStoreDisp<0x80, (outs),
                          (ins r0reg:$src, bytememdisp:$addr),
                          "mov.b\t$src,$addr">;
  def MOVW_store_disp
      : SHNarrowStoreDisp<0x81, (outs),
                          (ins r0reg:$src, wordmemdisp:$addr),
                          "mov.w\t$src,$addr">;
}

let mayLoad = 1 in {
  def MOVB_load_disp
      : SHNarrowLoadDisp<0x84, (outs r0reg:$dst),
                         (ins bytememdisp:$addr), "mov.b\t$addr,$dst">;
  def MOVW_load_disp
      : SHNarrowLoadDisp<0x85, (outs r0reg:$dst),
                         (ins wordmemdisp:$addr), "mov.w\t$addr,$dst">;
}

def EXTUB : SHRegReg<0x6, 0xc, (outs GPR:$dst), (ins GPR:$src),
                     "extu.b\t$src,$dst",
                     [(set GPR:$dst, (and GPR:$src, 0xff))]>;
def EXTUW : SHRegReg<0x6, 0xd, (outs GPR:$dst), (ins GPR:$src),
                     "extu.w\t$src,$dst",
                     [(set GPR:$dst, (and GPR:$src, 0xffff))]>;
def EXTSB : SHRegReg<0x6, 0xe, (outs GPR:$dst), (ins GPR:$src),
                     "exts.b\t$src,$dst",
                     [(set GPR:$dst, (sext_inreg GPR:$src, i8))]>;
def EXTSW : SHRegReg<0x6, 0xf, (outs GPR:$dst), (ins GPR:$src),
                     "exts.w\t$src,$dst",
                     [(set GPR:$dst, (sext_inreg GPR:$src, i16))]>;

let mayLoad = 1, Defs = [R0] in {
  def MOVB_load_frame
      : SHPseudo<(outs GPR:$dst), (ins bytememdisp:$addr)> {
    let Pattern =
        [(set GPR:$dst, (sextloadi8 addr_byte_frame:$addr))];
  }
  def MOVW_load_frame
      : SHPseudo<(outs GPR:$dst), (ins wordmemdisp:$addr)> {
    let Pattern =
        [(set GPR:$dst, (sextloadi16 addr_word_frame:$addr))];
  }
}

let mayStore = 1, Defs = [R0], hasPostISelHook = 1 in {
  def MOVB_store_frame
      : SHPseudo<(outs), (ins GPR:$src, bytememdisp:$addr)> {
    let Pattern =
        [(truncstorei8 GPR:$src, addr_byte_frame:$addr)];
  }
  def MOVW_store_frame
      : SHPseudo<(outs), (ins GPR:$src, wordmemdisp:$addr)> {
    let Pattern =
        [(truncstorei16 GPR:$src, addr_word_frame:$addr)];
  }
}

def : Pat<(i32 (extloadi8 addr_narrow_reg:$addr)),
          (MOVB_load_reg addr_narrow_reg:$addr)>;
def : Pat<(i32 (extloadi16 addr_narrow_reg:$addr)),
          (MOVW_load_reg addr_narrow_reg:$addr)>;
def : Pat<(i32 (zextloadi8 addr_narrow_reg:$addr)),
          (EXTUB (MOVB_load_reg addr_narrow_reg:$addr))>;
def : Pat<(i32 (zextloadi16 addr_narrow_reg:$addr)),
          (EXTUW (MOVW_load_reg addr_narrow_reg:$addr))>;

def : Pat<(i32 (extloadi8 addr_byte_frame:$addr)),
          (MOVB_load_frame addr_byte_frame:$addr)>;
def : Pat<(i32 (extloadi16 addr_word_frame:$addr)),
          (MOVW_load_frame addr_word_frame:$addr)>;
def : Pat<(i32 (zextloadi8 addr_byte_frame:$addr)),
          (EXTUB (MOVB_load_frame addr_byte_frame:$addr))>;
def : Pat<(i32 (zextloadi16 addr_word_frame:$addr)),
          (EXTUW (MOVW_load_frame addr_word_frame:$addr))>;
