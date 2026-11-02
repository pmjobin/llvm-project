; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s

define i32 @zero_test_i8(ptr %input) {
entry:
	%value = load i8, ptr %input, align 1
	%iszero = icmp eq i8 %value, 0
	%result = zext i1 %iszero to i32
	ret i32 %result
}

define i32 @zero_test_i16(ptr %input) {
entry:
	%value = load i16, ptr %input, align 2
	%iszero = icmp eq i16 %value, 0
	%result = zext i1 %iszero to i32
	ret i32 %result
}

define i32 @branch_zero_test_i8(ptr %input) {
entry:
	%value = load i8, ptr %input, align 1
	%iszero = icmp eq i8 %value, 0
	br i1 %iszero, label %zero, label %nonzero

zero:
	ret i32 1

nonzero:
	ret i32 0
}

define i32 @shared_unsigned_load(ptr %input) {
entry:
	%value = load i8, ptr %input, align 1
	%iszero = icmp eq i8 %value, 0
	%extended = zext i8 %value to i32
	br i1 %iszero, label %zero, label %nonzero

zero:
	ret i32 256

nonzero:
	ret i32 %extended
}

; CHECK-LABEL: zero_test_i8:
; CHECK: mov.b
; CHECK-NEXT: tst
; CHECK-NOT: extu.b

; CHECK-LABEL: zero_test_i16:
; CHECK: mov.w
; CHECK-NEXT: tst
; CHECK-NOT: extu.w

; CHECK-LABEL: branch_zero_test_i8:
; CHECK: mov.b
; CHECK-NEXT: tst
; CHECK-NOT: extu.b

; CHECK-LABEL: shared_unsigned_load:
; CHECK: mov.b
; CHECK-NEXT: extu.b
; CHECK-NEXT: tst
