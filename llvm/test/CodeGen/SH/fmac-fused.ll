; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2e -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3e -O2 -verify-machineinstrs < %s | FileCheck %s

declare float @llvm.fma.f32(float, float, float)

; CHECK-LABEL: fused_single:
; CHECK: fmac fr0,{{fr[0-9]+}},{{fr[0-9]+}}
; CHECK-NOT: fmul
; CHECK-NOT: fadd
define float @fused_single(float %lhs, float %rhs, float %addend) {
entry:
	%result = call float @llvm.fma.f32(float %lhs, float %rhs, float %addend)
	ret float %result
}
