; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define i32 @divide_unsigned(i32 %dividend, i32 %divisor) {
; CHECK-LABEL: divide_unsigned:
; CHECK: mov	r4,[[QUOTIENT:r[0-9]+]]
; CHECK: mov	#0,[[PARTIAL:r[0-9]+]]
; CHECK-NEXT: div0u
; CHECK-COUNT-32: rotcl	[[QUOTIENT]]
; CHECK-NEXT: div1	[[DIVISOR:r[0-9]+]],[[PARTIAL]]
; CHECK-NEXT: rotcl	[[QUOTIENT]]
; CHECK-NOT: div1
; CHECK-NOT: mul.l
; CHECK-NOT: bsr
; CHECK-NOT: jsr
; CHECK: rts
; CHECK-NEXT: nop
	%quotient = udiv i32 %dividend, %divisor
	ret i32 %quotient
}

define i32 @divide_unsigned_exact(i32 %dividend, i32 %divisor) {
; CHECK-LABEL: divide_unsigned_exact:
; CHECK: div0u
; CHECK-COUNT-32: div1
; CHECK: rts
	%quotient = udiv exact i32 %dividend, %divisor
	ret i32 %quotient
}
