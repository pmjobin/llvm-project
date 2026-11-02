; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

declare void @positive_path()
declare void @negative_path()

define i32 @eq_zero_branch(i32 %value, i32 %true, i32 %false) {
; CHECK-LABEL: eq_zero_branch:
; CHECK: tst	r4,r4
; CHECK: bt
; CHECK-NOT: movt
	%condition = icmp eq i32 %value, 0
	%result = select i1 %condition, i32 %true, i32 %false
	ret i32 %result
}

define i32 @ne_zero_branch(i32 %value, i32 %true, i32 %false) {
; CHECK-LABEL: ne_zero_branch:
; CHECK: tst	r4,r4
; CHECK: bt
; CHECK-NOT: movt
	%condition = icmp ne i32 %value, 0
	%result = select i1 %condition, i32 %true, i32 %false
	ret i32 %result
}

define i32 @sge_zero_branch(i32 %value, i32 %true, i32 %false) {
; CHECK-LABEL: sge_zero_branch:
; CHECK: cmp/pz
; CHECK: bt
; CHECK-NOT: {{movt|tst}}
	%condition = icmp sge i32 %value, 0
	%result = select i1 %condition, i32 %true, i32 %false
	ret i32 %result
}

define i32 @sgt_zero_branch(i32 %value, i32 %true, i32 %false) {
; CHECK-LABEL: sgt_zero_branch:
; CHECK: cmp/pl	r4
; CHECK: bf
; CHECK-NOT: {{movt|tst}}
	%condition = icmp sgt i32 %value, 0
	%result = select i1 %condition, i32 %true, i32 %false
	ret i32 %result
}
define void @sge_zero_control(i32 %value) {
; CHECK-LABEL: sge_zero_control:
; CHECK: cmp/pz	r4
; CHECK-NEXT: bf
; CHECK-NOT: {{movt|tst}}
	%condition = icmp sge i32 %value, 0
	br i1 %condition, label %positive, label %negative

positive:
	call void @positive_path()
	ret void

negative:
	call void @negative_path()
	ret void
}

define i32 @direct_compare_select(i32 %lhs, i32 %rhs, i32 %true, i32 %false) {
; CHECK-LABEL: direct_compare_select:
; CHECK: cmp/hi	r5,r4
; CHECK: bt
; CHECK-NOT: {{movt|tst}}
	%condition = icmp ugt i32 %lhs, %rhs
	%result = select i1 %condition, i32 %true, i32 %false
	ret i32 %result
}

define i32 @materialized_compare(i32 %lhs, i32 %rhs) {
; CHECK-LABEL: materialized_compare:
; CHECK: cmp/ge
; CHECK: movt
	%condition = icmp sle i32 %lhs, %rhs
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @branch_and_value(ptr %out, i32 %lhs, i32 %rhs) {
; CHECK-LABEL: branch_and_value:
; CHECK: cmp/gt
; CHECK-NEXT: movt
; CHECK: mov.l
; CHECK-NEXT: bf
; CHECK-NOT: {{cmp/|tst}}
	%condition = icmp sgt i32 %lhs, %rhs
	%value = zext i1 %condition to i32
	store i32 %value, ptr %out, align 4
	br i1 %condition, label %true, label %false

true:
	ret i32 11

false:
	ret i32 22
}

define i32 @fallthrough_compare(i32 %lhs, i32 %rhs) {
; CHECK-LABEL: fallthrough_compare:
; CHECK: cmp/hs
; CHECK-NEXT: bt
; CHECK-NOT: bra
	%condition = icmp ult i32 %lhs, %rhs
	br i1 %condition, label %true, label %false

true:
	ret i32 %lhs

false:
	ret i32 %rhs
}
