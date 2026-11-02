; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s

define i32 @equal_i8_200(ptr %input) {
entry:
	%value = load i8, ptr %input, align 1
	%equal = icmp eq i8 %value, -56
	%result = zext i1 %equal to i32
	ret i32 %result
}

define i32 @branch_not_equal_i16_60000(ptr %input) {
entry:
	%value = load i16, ptr %input, align 2
	%equal = icmp ne i16 %value, -5536
	br i1 %equal, label %true, label %false

true:
	ret i32 1

false:
	ret i32 0
}

; CHECK-LABEL: equal_i8_200:
; CHECK: mov.b
; CHECK-NOT: extu.b
; CHECK: cmp/eq

; CHECK-LABEL: branch_not_equal_i16_60000:
; CHECK: mov.w
; CHECK-NOT: extu.w
; CHECK: cmp/eq
