; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/twenty.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=OUTGOING64
; RUN: not --crash llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/twenty.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=OUTGOING64
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %t/incoming64.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INCOMING64
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i1-stack.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i8-stack.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i16-stack.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector-stack.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/inalloca.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/dynamic-alloca-call.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=DYNAMIC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/stack-realignment.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=REALIGN
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/nest.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/returned.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/swiftself.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT

; OUTGOING64: LLVM ERROR: SH outgoing call frame size cannot exceed 60 bytes
; INCOMING64: LLVM ERROR: SH finalized frame reference offset 64 must be four-byte aligned and in [0, 60] from r15
; ARGUMENT: LLVM ERROR: SH calls only support scalar i32, i64, and pointer arguments
; DYNAMIC: LLVM ERROR: SH dynamic alloca is not supported
; REALIGN: LLVM ERROR: SH stack realignment is not supported

;--- twenty.ll
define internal i32 @take20(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6, i32 %a7, i32 %a8, i32 %a9, i32 %a10, i32 %a11, i32 %a12, i32 %a13, i32 %a14, i32 %a15, i32 %a16, i32 %a17, i32 %a18, i32 %a19) {
	ret i32 %a19
}

define i32 @call20() {
	%result = call i32 @take20(i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
	ret i32 %result
}

;--- incoming64.ll
define internal void @incoming_leaf() noinline {
	ret void
}

define i32 @incoming64(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6, i32 %a7, i32 %a8, i32 %a9, i32 %a10, i32 %a11, i32 %a12, i32 %a13, i32 %a14, i32 %a15, i32 %a16, i32 %a17, i32 %a18, i32 %a19) {
entry:
	%take_call = icmp eq i32 %a0, 0
	br i1 %take_call, label %with_call, label %without_call

with_call:
	call void @incoming_leaf()
	ret i32 0

without_call:
	ret i32 %a19
}

;--- i1-stack.ll
define void @i1_stack(ptr %fn) {
	call void %fn(i32 1, i32 2, i32 3, i32 4, i1 true)
	ret void
}

;--- i8-stack.ll
define void @i8_stack(ptr %fn) {
	call void %fn(i32 1, i32 2, i32 3, i32 4, i8 5)
	ret void
}

;--- i16-stack.ll
define void @i16_stack(ptr %fn) {
	call void %fn(i32 1, i32 2, i32 3, i32 4, i16 5)
	ret void
}

;--- vector-stack.ll
define void @vector_stack(ptr %fn) {
	call void %fn(i32 1, i32 2, i32 3, i32 4, <2 x i32> <i32 5, i32 6>)
	ret void
}

;--- inalloca.ll
define void @inalloca_argument(ptr %fn, ptr %args) {
	call void %fn(i32 1, i32 2, i32 3, i32 4, ptr inalloca(i32) %args)
	ret void
}

;--- dynamic-alloca-call.ll
define internal void @dynamic_callee(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4) {
	ret void
}

define void @dynamic_alloca_call(i32 %count) {
	%slot = alloca i32, i32 %count, align 4
	call void @dynamic_callee(i32 1, i32 2, i32 3, i32 4, i32 5)
	ret void
}

;--- stack-realignment.ll
define void @stack_realignment() alignstack(8) {
	ret void
}

;--- nest.ll
define void @nest_argument(ptr %fn, ptr %context) {
	call void %fn(i32 1, i32 2, i32 3, i32 4, ptr nest %context)
	ret void
}

;--- returned.ll
define ptr @returned_argument(ptr %fn, ptr %value) {
	%result = call ptr %fn(i32 1, i32 2, i32 3, i32 4, ptr returned %value)
	ret ptr %result
}

;--- swiftself.ll
define void @swiftself_argument(ptr %fn, ptr %value) {
	call void %fn(i32 1, i32 2, i32 3, i32 4, ptr swiftself %value)
	ret void
}
