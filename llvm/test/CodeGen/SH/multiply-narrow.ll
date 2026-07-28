; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define void @multiply_i8(ptr %lhs, ptr %rhs, ptr %out) {
; CHECK-LABEL: multiply_i8:
; CHECK: mov.b
; CHECK: mov.b
; CHECK: mul.l
; CHECK-NEXT: sts	macl,
; CHECK: mov.b
; CHECK: rts
	%a = load i8, ptr %lhs, align 1
	%b = load i8, ptr %rhs, align 1
	%product = mul i8 %a, %b
	store i8 %product, ptr %out, align 1
	ret void
}

define void @multiply_i16(ptr %lhs, ptr %rhs, ptr %out) {
; CHECK-LABEL: multiply_i16:
; CHECK: mov.w
; CHECK: mov.w
; CHECK: mul.l
; CHECK-NEXT: sts	macl,
; CHECK: mov.w
; CHECK: rts
	%a = load i16, ptr %lhs, align 2
	%b = load i16, ptr %rhs, align 2
	%product = mul i16 %a, %b
	store i16 %product, ptr %out, align 2
	ret void
}

define void @multiply_i8_mixed_extensions(ptr %lhs, ptr %rhs, ptr %out) {
; CHECK-LABEL: multiply_i8_mixed_extensions:
; CHECK: mul.l
; CHECK-NEXT: sts	macl,
; CHECK: mov.b
; CHECK: rts
	%a = load i8, ptr %lhs, align 1
	%b = load i8, ptr %rhs, align 1
	%a32 = sext i8 %a to i32
	%b32 = zext i8 %b to i32
	%product = mul i32 %a32, %b32
	%low = trunc i32 %product to i8
	store i8 %low, ptr %out, align 1
	ret void
}
