; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define internal i32 @multiply_callee(i32 %value) noinline {
	ret i32 %value
}

define i32 @multiply(i32 %a, i32 %b) {
; CHECK-LABEL: multiply:
; CHECK: mul.l	r4,r5
; CHECK-NEXT: sts	macl,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%result = mul i32 %a, %b
	ret i32 %result
}

define i32 @multiply_flags(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_flags:
; CHECK: mul.l
; CHECK-NEXT: sts	macl,
; CHECK: rts
	%result = mul nuw nsw i32 %a, %b
	ret i32 %result
}

define i32 @two_multiplies(i32 %a, i32 %b, i32 %c) {
; CHECK-LABEL: two_multiplies:
; CHECK: mul.l
; CHECK-NEXT: sts	macl,[[FIRST:r[0-9]+]]
; CHECK: mul.l
; CHECK-NEXT: sts	macl,[[SECOND:r[0-9]+]]
; CHECK: add	[[FIRST]],[[SECOND]]
; CHECK: rts
	%first = mul i32 %a, %b
	%second = mul i32 %a, %c
	%result = add i32 %first, %second
	ret i32 %result
}

define i32 @multiply_chain(i32 %a, i32 %b, i32 %c) {
; CHECK-LABEL: multiply_chain:
; CHECK: mul.l
; CHECK-NEXT: sts	macl,[[FIRST:r[0-9]+]]
; CHECK: mul.l
; CHECK-NEXT: sts	macl,
; CHECK: rts
	%first = mul i32 %a, %b
	%result = mul i32 %first, %c
	ret i32 %result
}

define i32 @multiply_around_call(i32 %a, i32 %b) {
; CHECK-LABEL: multiply_around_call:
; CHECK: mul.l
; CHECK-NEXT: sts	macl,
; CHECK: bsr	multiply_callee
; CHECK-NEXT: nop
; CHECK: mul.l
; CHECK-NEXT: sts	macl,
; CHECK: rts
	%before = mul i32 %a, %b
	%called = call i32 @multiply_callee(i32 %before)
	%after = mul i32 %called, %b
	ret i32 %after
}
