//===-- SH.td - Describe the SuperH target -----------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

include "SHRegisterInfo.td"
include "SHInstrInfo.td"
include "SHSubtarget.td"

defm : RemapAllTargetPseudoPointerOperands<GPR>;

def SHInstrInfo : InstrInfo;

def SHAsmWriter : AsmWriter {
  string AsmWriterClassName = "InstPrinter";
  bit isMCAsmWriter = 1;
}

def SHAsmParser : AsmParser {
  let ShouldEmitMatchRegisterName = 1;
}

def SH : Target {
  let InstructionSet = SHInstrInfo;
  let AssemblyWriters = [SHAsmWriter];
  let AssemblyParsers = [SHAsmParser];
}
