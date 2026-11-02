; Native-rotate admission is omitted; retain these contexts, poison/freeze
; witnesses and negative cases through the conventional helper lowering.
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=SELECT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -verify-machine-dom-info -verify-loop-info < %s > /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -enable-new-pm=1 -verify-machineinstrs -verify-machine-dom-info -verify-loop-info < %s > /dev/null

declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i32 @llvm.fshr.i32(i32, i32, i32)
declare void @clobber()

define i32 @unselected_poison(i32 %x, ptr %p, i1 %flag) {
entry:
; SELECT-LABEL: name: unselected_poison
; SELECT: JSR

	%c = load i32, ptr %p
	%bad = add nsw i32 %c, 2147483647
	br label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %loop ]
	%iteration = phi i32 [ %x, %entry ], [ %r, %loop ]
	%r = call i32 @llvm.fshl.i32(i32 %iteration, i32 %iteration, i32 %bad)
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, 32
	br i1 %done, label %exit, label %loop
exit:
	%v = select i1 %flag, i32 %r, i32 7
	ret i32 %v
}

define i32 @frozen_result(i32 %x, ptr %p) {
entry:
; SELECT-LABEL: name: frozen_result
; SELECT: JSR

	%c = load i32, ptr %p
	%bad = add nsw i32 %c, 2147483647
	br label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %loop ]
	%iteration = phi i32 [ %x, %entry ], [ %r, %loop ]
	%r = call i32 @llvm.fshr.i32(i32 %iteration, i32 %iteration, i32 %bad)
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, 32
	br i1 %done, label %exit, label %loop
exit:
	%f = freeze i32 %r
	call void @clobber()
	%v = mul i32 %f, 17
	ret i32 %v
}

define i32 @frozen_inputs(i32 %x, i32 %c) {
entry:
; SELECT-LABEL: name: frozen_inputs
; SELECT: JSR

	%v = freeze i32 %x
	%count = freeze i32 %c
	br label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %loop ]
	%iteration = phi i32 [ %v, %entry ], [ %r, %loop ]
	%r = call i32 @llvm.fshl.i32(i32 %iteration, i32 %iteration, i32 %count)
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, 32
	br i1 %done, label %exit, label %loop
exit:
	ret i32 %r
}

define i32 @undef_count(i32 %x) {
; SELECT-LABEL: name: undef_count

	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 undef)
	%v = freeze i32 %r
	ret i32 %v
}

define i32 @poison_count(i32 %x) {
; SELECT-LABEL: name: poison_count

	%r = call i32 @llvm.fshr.i32(i32 %x, i32 %x, i32 poison)
	%v = freeze i32 %r
	ret i32 %v
}

define i32 @loop_phi(ptr %data, ptr %counts, i32 %n, i32 %seed) {
; SELECT-LABEL: name: loop_phi
; SELECT: JSR
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %latch ]
	%sum = phi i32 [ %seed, %entry ], [ %joined, %latch ]
	%xp = getelementptr i32, ptr %data, i32 %i
	%cp = getelementptr i32, ptr %counts, i32 %i
	%x = load i32, ptr %xp
	%c = load i32, ptr %cp
	%old = icmp ult i32 %sum, %n
	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 %c)
	br i1 %old, label %call, label %latch
call:
	call void @clobber()
	br label %latch
latch:
	%joined = phi i32 [ %r, %loop ], [ %sum, %call ]
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %exit, label %loop
exit:
	%result = phi i32 [ %seed, %entry ], [ %joined, %latch ]
	ret i32 %result
}
