; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s

define i32 @add_comparison(i32 %accumulator, i32 %left, i32 %right) {
	%condition = icmp sgt i32 %left, %right
	%boolean = zext i1 %condition to i32
	%result = add i32 %boolean, %accumulator
	ret i32 %result
}

; CHECK-LABEL: add_comparison:
; CHECK-NOT: movt
; CHECK: mov	#0,[[ZERO:r[0-9]+]]
; CHECK-NEXT: cmp/gt
; CHECK-NEXT: rts
; CHECK-NEXT: addc	[[ZERO]],r0

define i32 @add_inverted_comparison(i32 %accumulator, i32 %left, i32 %right) {
	%condition = icmp ne i32 %left, %right
	%boolean = zext i1 %condition to i32
	%result = add i32 %boolean, %accumulator
	ret i32 %result
}

; CHECK-LABEL: add_inverted_comparison:
; CHECK-NOT: movt
; CHECK: mov	#-1,[[NEGATIVE_ONE:r[0-9]+]]
; CHECK-NEXT: cmp/eq
; CHECK-NEXT: rts
; CHECK-NEXT: subc	[[NEGATIVE_ONE]],r0
