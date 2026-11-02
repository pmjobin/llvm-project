; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s

define i32 @add_sign_bit(i32 %accumulator, i32 %value) {
; CHECK-LABEL: add_sign_bit:
; CHECK-NOT: rotl
; CHECK-NOT: movt
; CHECK: mov	#-1,[[NEGATIVE_ONE:r[0-9]+]]
; CHECK-NEXT: cmp/pz	r5
; CHECK-NEXT: rts
; CHECK-NEXT: subc	[[NEGATIVE_ONE]],r0
	%sign = lshr i32 %value, 31
	%result = add i32 %sign, %accumulator
	ret i32 %result
}
