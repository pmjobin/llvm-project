; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define i32 @divrem_unsigned(i32 %dividend, i32 %divisor) {
; CHECK-LABEL: divrem_unsigned:
; CHECK: div0u
; CHECK-COUNT-32: div1
; CHECK: rotcl	[[QUOTIENT:r[0-9]+]]
; CHECK-NEXT: mul.l	[[DIVISOR:r[0-9]+]],[[QUOTIENT]]
; CHECK-NEXT: sts	macl,[[PRODUCT:r[0-9]+]]
; CHECK-NEXT: sub	[[PRODUCT]],[[REMAINDER:r[0-9]+]]
; CHECK-NEXT: add	[[REMAINDER]],[[QUOTIENT]]
; CHECK-NOT: div0u
; CHECK-NOT: div1
; CHECK: rts
	%quotient = udiv i32 %dividend, %divisor
	%remainder = urem i32 %dividend, %divisor
	%result = add i32 %quotient, %remainder
	ret i32 %result
}

define i32 @divrem_signed(i32 %dividend, i32 %divisor) {
; CHECK-LABEL: divrem_signed:
; CHECK: div0s
; CHECK-COUNT-32: div1
; CHECK: rotcl	[[QUOTIENT:r[0-9]+]]
; CHECK-NEXT: addc	{{r[0-9]+}},[[QUOTIENT]]
; CHECK-NEXT: mul.l	[[DIVISOR:r[0-9]+]],[[QUOTIENT]]
; CHECK-NEXT: sts	macl,[[PRODUCT:r[0-9]+]]
; CHECK-NEXT: sub	[[PRODUCT]],[[REMAINDER:r[0-9]+]]
; CHECK-NEXT: add	[[REMAINDER]],[[QUOTIENT]]
; CHECK-NOT: div0s
; CHECK-NOT: div1
; CHECK: rts
	%quotient = sdiv i32 %dividend, %divisor
	%remainder = srem i32 %dividend, %divisor
	%result = add i32 %quotient, %remainder
	ret i32 %result
}
