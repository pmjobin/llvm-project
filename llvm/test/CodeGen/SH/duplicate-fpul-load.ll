; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s

define double @return_zero() {
entry:
	ret double 0.000000e+00
}

define double @return_zero_optsize() #0 {
entry:
	ret double 0.000000e+00
}

define double @return_zero_minsize() #1 {
entry:
	ret double 0.000000e+00
}

; CHECK-LABEL: return_zero:
; CHECK: mova
; CHECK-COUNT-2: fmov	@{{r[0-9]+}}+,fr{{[01]}}

; CHECK-LABEL: return_zero_optsize:
; CHECK-COUNT-1: lds	{{r[0-9]+}},fpul
; CHECK: fsts	fpul,fr0
; CHECK-NEXT: fsts	fpul,fr1

; CHECK-LABEL: return_zero_minsize:
; CHECK-COUNT-1: lds	{{r[0-9]+}},fpul
; CHECK: fsts	fpul,fr0
; CHECK-NEXT: fsts	fpul,fr1

attributes #0 = { optsize }
attributes #1 = { minsize optsize }
