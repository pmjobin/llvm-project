//===-- SHRegisterInfo.td - SuperH register definitions ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class SHReg<bits<4> Encoding, string Name> : Register<Name> {
  let Namespace = "SH";
  let HWEncoding{3-0} = Encoding;
}

foreach I = 0-15 in
  def R#I : SHReg<I, "r"#I>;

let Namespace = "SH" in {
def PC : Register<"pc">;
def PR : Register<"pr">;
def GBR : Register<"gbr">;
def VBR : Register<"vbr">;
def MACH : Register<"mach">;
def MACL : Register<"macl">;
def SR : Register<"sr">;
def TBit : Register<"">;
def MBit : Register<"">;
def QBit : Register<"">;
}

// Keep caller-saved registers first so the narrow register-only leaf-function
// subset does not need callee-save spills.
def GPR : RegisterClass<"SH", [i32], 32,
                        (add (sequence "R%u", 0, 7),
                             (sequence "R%u", 8, 14), R15)>;
