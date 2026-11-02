; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O2 < %s | FileCheck %s --check-prefixes=CHECK,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 < %s | FileCheck %s --check-prefixes=CHECK,LE

define float @add_literal(float %value) nounwind {
; CHECK-LABEL: add_literal:
; CHECK: mova [[LITERAL:.LCPI[0-9]+_[0-9]+_[0-9]+]],r0
; CHECK-NEXT: fmov @r0,[[CONSTANT:fr[0-9]+]]
; CHECK: fadd {{fr[0-9]+}},[[CONSTANT]]
; CHECK: [[LITERAL]]:
; CHECK-NEXT: .long 0x40200000
	%result = fadd float %value, 2.500000e+00
	ret float %result
}

define double @add_double_literal(double %value) nounwind {
; CHECK-LABEL: add_double_literal:
; CHECK: mova [[DOUBLE:.LCPI[0-9]+_[0-9]+_[0-9]+]],r0
; BE-NEXT: fmov @r0+,fr2
; BE-NEXT: fmov @r0+,fr3
; LE-NEXT: fmov @r0+,fr3
; LE-NEXT: fmov @r0+,fr2
; CHECK: fadd dr
; CHECK: .p2align 2
; CHECK: [[DOUBLE]]:
; CHECK-NEXT: .uaquad 4612811918334230528
	%result = fadd double %value, 2.500000e+00
	ret double %result
}
