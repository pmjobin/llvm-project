//===-- SHCallingConv.td - SH calling conventions -----------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

def CC_SH : CallingConv<[
  CCIfType<[i32], CCAssignToReg<[R4, R5, R6, R7]>>,
  CCAssignToStack<4, 4>
]>;

def RetCC_SH : CallingConv<[
  CCIfType<[i32], CCAssignToReg<[R0, R1]>>
]>;

def CSR_SH_Save : CalleeSavedRegs<(add PR, R8, R9, R10, R11, R12, R13, R14)>;

def CSR_SH
    : CalleeSavedRegs<(add R8, R9, R10, R11, R12, R13, R14, R15,
                          GBR, VBR, SR)>;
