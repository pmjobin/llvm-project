; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i1-stack.ll -o - | FileCheck %s --check-prefix=I1
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i8-stack.ll -o - | FileCheck %s --check-prefix=I8
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i16-stack.ll -o - | FileCheck %s --check-prefix=I16
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector-stack.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CALL-TYPE
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/inalloca.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CALL-ATTRIBUTE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/nest.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/returned.ll -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/returned.ll -o /dev/null
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/swiftself.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CALL-ATTRIBUTE

; I1-LABEL: i1_stack:
; I1: mov	#1,r4
; I1: mov.l	r4,@r15
; I8-LABEL: i8_stack:
; I8: mov	#5,r1
; I8: mov.l	r1,@r15
; I16-LABEL: i16_stack:
; I16: mov	#5,r1
; I16: mov.l	r1,@r15
; CALL-TYPE: error: {{.*}}SH call argument type is not supported by the ABI
; CALL-ATTRIBUTE: error: {{.*}}SH call argument has unsupported ABI attributes

;--- thirty-seven.ll
declare i32 @take37(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6, i32 %a7, i32 %a8, i32 %a9, i32 %a10, i32 %a11, i32 %a12, i32 %a13, i32 %a14, i32 %a15, i32 %a16, i32 %a17, i32 %a18, i32 %a19, i32 %a20, i32 %a21, i32 %a22, i32 %a23, i32 %a24, i32 %a25, i32 %a26, i32 %a27, i32 %a28, i32 %a29, i32 %a30, i32 %a31, i32 %a32, i32 %a33, i32 %a34, i32 %a35, i32 %a36)

define i32 @call37() {
	%result = call i32 @take37(i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
	ret i32 %result
}

;--- incoming132.ll
define internal void @incoming_leaf() noinline {
	ret void
}

define i32 @incoming132(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6, i32 %a7, i32 %a8, i32 %a9, i32 %a10, i32 %a11, i32 %a12, i32 %a13, i32 %a14, i32 %a15, i32 %a16, i32 %a17, i32 %a18, i32 %a19, i32 %a20, i32 %a21, i32 %a22, i32 %a23, i32 %a24, i32 %a25, i32 %a26, i32 %a27, i32 %a28, i32 %a29, i32 %a30, i32 %a31, i32 %a32, i32 %a33, i32 %a34, i32 %a35, i32 %a36) {
entry:
	%take_call = icmp eq i32 %a0, 0
	br i1 %take_call, label %with_call, label %without_call

with_call:
	call void @incoming_leaf()
	ret i32 0

without_call:
	ret i32 %a36
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
