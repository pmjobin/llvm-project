; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s

define i32 @branch_stored_compare(i32 %lhs, i32 %rhs, ptr %out) {
entry:
	%condition = icmp slt i32 %lhs, %rhs
	%stored = zext i1 %condition to i8
	store i8 %stored, ptr %out, align 1
	br i1 %condition, label %true, label %false

true:
	ret i32 1

false:
	ret i32 0
}

; CHECK-LABEL: branch_stored_compare:
; CHECK: cmp/gt
; CHECK-NEXT: movt
; CHECK: mov.b
; CHECK-NOT: tst
; CHECK: bt
