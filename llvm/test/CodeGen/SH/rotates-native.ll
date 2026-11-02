; Native-rotate admission is omitted; retain these contexts, poison/freeze
; witnesses and negative cases through the conventional helper lowering.
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=SELECT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -verify-machine-dom-info -verify-loop-info < %s > /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -enable-new-pm=1 -verify-machineinstrs -verify-machine-dom-info -verify-loop-info < %s > /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=FALLBACK
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O2 -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=FALLBACK
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=FALLBACK
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -relocation-model=pic -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=FALLBACK

; FALLBACK-NOT: NATIVE_PSEUDO
declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i32 @llvm.fshr.i32(i32, i32, i32)

define i32 @left(i32 %x, i32 %count) {
entry:
; SELECT-LABEL: name: left
; SELECT: JSR
; ASM-LABEL: left:
; ASM-COUNT-2: jsr
; ASM: .long __lshrsi3
; ASM: .long __ashlsi3

	br label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %loop ]
	%iteration = phi i32 [ %x, %entry ], [ %r, %loop ]
	%r = call i32 @llvm.fshl.i32(i32 %iteration, i32 %iteration, i32 %count)
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, 32
	br i1 %done, label %exit, label %loop
exit:
	ret i32 %r
}

define i32 @right_loaded(i32 %x, ptr %p) {
entry:
; SELECT-LABEL: name: right_loaded
; SELECT: JSR
; ASM-LABEL: right_loaded:
; ASM-COUNT-2: jsr
; ASM: .long __ashlsi3
; ASM: .long __lshrsi3

	%count = load i32, ptr %p
	br label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %loop ]
	%iteration = phi i32 [ %x, %entry ], [ %r, %loop ]
	%r = call i32 @llvm.fshr.i32(i32 %iteration, i32 %iteration, i32 %count)
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, 32
	br i1 %done, label %exit, label %loop
exit:
	ret i32 %r
}

define i32 @live_count(i32 %x, i32 %count) {
; SELECT-LABEL: name: live_count
; SELECT-NOT: NATIVE_PSEUDO
; SELECT: CALL

	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 %count)
	%v = xor i32 %r, %count
	ret i32 %v
}

define i32 @live_source(i32 %x, i32 %count) {
; SELECT-LABEL: name: live_source
; SELECT-NOT: NATIVE_PSEUDO
; SELECT: CALL

	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 %count)
	%v = xor i32 %r, %x
	ret i32 %v
}

define i32 @short(i32 %x, i32 %count) {
; SELECT-LABEL: name: short
; SELECT: ROTL_SHORT_PSEUDO

	%c = and i32 %count, 15
	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 %c)
	ret i32 %r
}

define i32 @constant_source(i32 %count) {
; SELECT-LABEL: name: constant_source
; SELECT-NOT: NATIVE_PSEUDO
; SELECT: CALL

	%r = call i32 @llvm.fshl.i32(i32 -2, i32 -2, i32 %count)
	ret i32 %r
}

define i32 @funnel(i32 %x, i32 %y, i32 %count) {
; SELECT-LABEL: name: funnel
; SELECT-NOT: NATIVE_PSEUDO
; SELECT: CALL

	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %y, i32 %count)
	ret i32 %r
}

define i32 @size(i32 %x, i32 %count) optsize {
; SELECT-LABEL: name: size
; SELECT-NOT: NATIVE_PSEUDO
; SELECT: CALL

	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 %count)
	ret i32 %r
}

define i32 @unoptimized(i32 %x, i32 %count) noinline optnone {
; SELECT-LABEL: name: unoptimized
; SELECT-NOT: NATIVE_PSEUDO
; SELECT: ROTLrr

	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 %count)
	ret i32 %r
}

define i32 @after_optnone(i32 %x, i32 %count) {
entry:
; SELECT-LABEL: name: after_optnone
; SELECT: JSR

	br label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %loop ]
	%iteration = phi i32 [ %x, %entry ], [ %r, %loop ]
	%r = call i32 @llvm.fshl.i32(i32 %iteration, i32 %iteration, i32 %count)
	%next = add i32 %i, 1
	%done = icmp eq i32 %next, 32
	br i1 %done, label %exit, label %loop
exit:
	ret i32 %r
}
