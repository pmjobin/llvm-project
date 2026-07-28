; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define i32 @remainder_unsigned(i32 %dividend, i32 %divisor) {
; CHECK-LABEL: remainder_unsigned:
; CHECK: div0u
; CHECK-COUNT-32: div1
; CHECK: rotcl	[[QUOTIENT:r[0-9]+]]
; CHECK-NEXT: mul.l	[[DIVISOR:r[0-9]+]],[[QUOTIENT]]
; CHECK-NEXT: sts	macl,[[PRODUCT:r[0-9]+]]
; CHECK-NEXT: sub	[[PRODUCT]],[[DIVIDEND:r[0-9]+]]
; CHECK-NOT: div0u
; CHECK-NOT: div1
; CHECK: rts
	%remainder = urem i32 %dividend, %divisor
	ret i32 %remainder
}

define i32 @remainder_signed(i32 %dividend, i32 %divisor) {
; CHECK-LABEL: remainder_signed:
; CHECK: div0s
; CHECK-COUNT-32: div1
; CHECK: rotcl	[[QUOTIENT:r[0-9]+]]
; CHECK-NEXT: addc	{{r[0-9]+}},[[QUOTIENT]]
; CHECK-NEXT: mul.l	[[DIVISOR:r[0-9]+]],[[QUOTIENT]]
; CHECK-NEXT: sts	macl,[[PRODUCT:r[0-9]+]]
; CHECK-NEXT: sub	[[PRODUCT]],[[DIVIDEND:r[0-9]+]]
; CHECK-NOT: div0s
; CHECK-NOT: div1
; CHECK: rts
	%remainder = srem i32 %dividend, %divisor
	ret i32 %remainder
}
