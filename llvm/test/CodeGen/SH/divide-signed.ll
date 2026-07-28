; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define i32 @divide_signed(i32 %dividend, i32 %divisor) {
; CHECK-LABEL: divide_signed:
; CHECK: mov	r4,
; CHECK: shll	[[SIGN:r[0-9]+]]
; CHECK: subc	[[PARTIAL:r[0-9]+]],[[PARTIAL]]
; CHECK-NEXT: xor	[[SIGN]],[[SIGN]]
; CHECK-NEXT: subc	[[SIGN]],[[QUOTIENT:r[0-9]+]]
; CHECK-NEXT: div0s	[[DIVISOR:r[0-9]+]],[[PARTIAL]]
; CHECK-COUNT-32: rotcl	[[QUOTIENT]]
; CHECK-NEXT: div1	[[DIVISOR]],[[PARTIAL]]
; CHECK-NEXT: rotcl	[[QUOTIENT]]
; CHECK-NEXT: addc	[[SIGN]],[[QUOTIENT]]
; CHECK-NOT: div1
; CHECK-NOT: cmp/
; CHECK-NOT: bt
; CHECK-NOT: bf
; CHECK-NOT: bsr
; CHECK-NOT: jsr
; CHECK: rts
; CHECK-NEXT: nop
	%quotient = sdiv i32 %dividend, %divisor
	ret i32 %quotient
}

define i32 @divide_signed_exact(i32 %dividend, i32 %divisor) {
; CHECK-LABEL: divide_signed_exact:
; CHECK: shll
; CHECK: div0s
; CHECK-COUNT-32: div1
; CHECK: addc
; CHECK: rts
	%quotient = sdiv exact i32 %dividend, %divisor
	ret i32 %quotient
}
