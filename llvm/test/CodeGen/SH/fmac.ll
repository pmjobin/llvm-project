; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=HARD,SH4
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=HARD,SH4
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2e -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=HARD,E
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3e -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=HARD,E
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SOFT

declare float @llvm.fmuladd.f32(float, float, float)
declare double @llvm.fmuladd.f64(double, double, double)

; HARD-LABEL: fused_single:
; HARD: fmac fr0,{{fr[0-9]+}},{{fr[0-9]+}}
; HARD-NOT: fmul
; HARD-NOT: fadd
; SOFT-LABEL: fused_single:
; SOFT: __mulsf3
; SOFT: __addsf3
; SOFT-NOT: fmac
define float @fused_single(float %lhs, float %rhs, float %addend) {
entry:
	%result = call float @llvm.fmuladd.f32(float %lhs, float %rhs, float %addend)
	ret float %result
}

; SH4-LABEL: unfused_double:
; SH4: fmul
; SH4: fadd
; SH4-NOT: fmac
; E-LABEL: unfused_double:
; E: __muldf3
; E: __adddf3
define double @unfused_double(double %lhs, double %rhs, double %addend) {
entry:
	%result = call double @llvm.fmuladd.f64(double %lhs, double %rhs, double %addend)
	ret double %result
}
