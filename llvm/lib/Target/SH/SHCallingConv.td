//===-- SHCallingConv.td - SH calling conventions -----------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

def CSR_SH_Save : CalleeSavedRegs<(add PR, R8, R9, R10, R11, R12, R13, R14)>;

def CSR_SH_Interrupt_Save
	: CalleeSavedRegs<(add PR, R0, R1, R2, R3, R4, R5, R6, R7,
		R8, R9, R10, R11, R12, R13, R14,
		FR0, FR1, FR2, FR3, FR4, FR5, FR6, FR7,
		FR8, FR9, FR10, FR11, FR12, FR13, FR14, FR15)>;

def CSR_SH_Interrupt_NoLow_Save
	: CalleeSavedRegs<(add PR, R8, R9, R10, R11, R12, R13, R14,
		FR0, FR1, FR2, FR3, FR4, FR5, FR6, FR7,
		FR8, FR9, FR10, FR11, FR12, FR13, FR14, FR15)>;

def CSR_SH_Interrupt_StackSwitch_Save
	: CalleeSavedRegs<(add PR, R1, R2, R3, R4, R5, R6, R7,
		R8, R9, R10, R11, R12, R13, R14,
		FR0, FR1, FR2, FR3, FR4, FR5, FR6, FR7,
		FR8, FR9, FR10, FR11, FR12, FR13, FR14, FR15)>;

def CSR_SH_Interrupt_Resbank_Save
	: CalleeSavedRegs<(add FR0, FR1, FR2, FR3, FR4, FR5, FR6, FR7,
		FR8, FR9, FR10, FR11, FR12, FR13, FR14, FR15)>;

def CSR_SH_Hard_Save
	: CalleeSavedRegs<(add PR, R8, R9, R10, R11, R12, R13, R14,
		FR12, FR13, FR14, FR15)>;

def CSR_SH_FDPIC_Save
	: CalleeSavedRegs<(add PR, R8, R9, R10, R11, R13, R14)>;

def CSR_SH_FDPIC_Hard_Save
	: CalleeSavedRegs<(add PR, R8, R9, R10, R11, R13, R14,
		FR12, FR13, FR14, FR15)>;

def CSR_SH_EHRet_Save
	: CalleeSavedRegs<(add R4, R5, R6, R7, CSR_SH_Save)>;

def CSR_SH_Hard_EHRet_Save
	: CalleeSavedRegs<(add R4, R5, R6, R7, CSR_SH_Hard_Save)>;

def CSR_SH_FDPIC_EHRet_Save
	: CalleeSavedRegs<(add R4, R5, R6, R7, CSR_SH_FDPIC_Save)>;

def CSR_SH_FDPIC_Hard_EHRet_Save
	: CalleeSavedRegs<(add R4, R5, R6, R7, CSR_SH_FDPIC_Hard_Save)>;

def CSR_SH
    : CalleeSavedRegs<(add R8, R9, R10, R11, R12, R13, R14, R15,
                          GBR, VBR, SR)>;

def CSR_SH_Hard
	: CalleeSavedRegs<(add R8, R9, R10, R11, R12, R13, R14, R15,
		GBR, VBR, SR, FR12, FR13, FR14, FR15)>;

def CSR_SH_FDPIC
	: CalleeSavedRegs<(add R8, R9, R10, R11, R13, R14, R15,
		GBR, VBR, SR)>;

def CSR_SH_FDPIC_Hard
	: CalleeSavedRegs<(add R8, R9, R10, R11, R13, R14, R15,
		GBR, VBR, SR, FR12, FR13, FR14, FR15)>;

// The left and logical-right variable-shift helpers preserve every allocatable
// register except the r0 result. The call also clobbers PR and the helpers T.
def CSR_SH_Shift
	: CalleeSavedRegs<(add (sequence "R%u", 1, 15),
		FR0, FR1, FR2, FR3, FR4, FR5, FR6, FR7,
		FR8, FR9, FR10, FR11, FR12, FR13, FR14, FR15,
		XF0, XF1, XF2, XF3, XF4, XF5, XF6, XF7,
		XF8, XF9, XF10, XF11, XF12, XF13, XF14, XF15,
		GBR, VBR, MACH, MACL, FPUL, FPSCR)>;

// The arithmetic-right helper additionally destroys the r5 count register.
def CSR_SH_AShr : CalleeSavedRegs<(sub CSR_SH_Shift, R5)>;
