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

let Namespace = "SH" in {
def PC : Register<"pc">, DwarfRegNum<[16]>;
def PR : Register<"pr">, DwarfRegNum<[17]>;
def GBR : Register<"gbr">, DwarfRegNum<[18]>;
def VBR : Register<"vbr">, DwarfRegNum<[19]>;
def MACH : Register<"mach">, DwarfRegNum<[20]>;
def MACL : Register<"macl">, DwarfRegNum<[21]>;
def SR : Register<"sr">, DwarfRegNum<[22]>;
def TBit : Register<"">;
def MBit : Register<"">;
def QBit : Register<"">;
}

// Keep caller-saved registers first so the narrow register-only leaf-function
// subset does not need callee-save spills.
def GPR : RegisterClass<"SH", [i32], 32,
                        (add (sequence "R%u", 0, 7),
                             (sequence "R%u", 8, 14), R15)>;
