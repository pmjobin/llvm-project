; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/fifth.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=STACK
; RUN: not --crash llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/varargs-call.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VARARGS-CALL
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/varargs-definition.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VARARGS
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i64-argument.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/float-argument.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/aggregate-argument.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/byval.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/sret.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i64-return.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=RETURN
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/float-return.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=RETURN
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/aggregate-return.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=RETURN
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/tail.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TAIL
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/musttail.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MUSTTAIL
; RUN: rm -f %t/external.o %t/external-le.o %t/range.o %t/range-le.o
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %t/external.ll -o %t/external.o 2>&1 | FileCheck %s --check-prefix=EXTERNAL
; RUN: not test -e %t/external.o
; RUN: not --crash llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %t/external.ll -o %t/external-le.o 2>&1 | FileCheck %s --check-prefix=EXTERNAL
; RUN: not test -e %t/external-le.o
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %t/out-of-range.ll -o %t/range.o 2>&1 | FileCheck %s --check-prefix=RANGE
; RUN: not test -e %t/range.o
; RUN: not llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %t/out-of-range.ll -o %t/range-le.o 2>&1 | FileCheck %s --check-prefix=RANGE
; RUN: not test -e %t/range-le.o
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/cross-section.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CROSS
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/large-frame.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=FRAME
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/calling-convention.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/inline-asm.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INLINE-ASM

; STACK: LLVM ERROR: SH stack-passed call arguments are not supported
; VARARGS-CALL: LLVM ERROR: SH varargs calls are not supported
; VARARGS: LLVM ERROR: SH varargs are not supported
; ARGUMENT: LLVM ERROR: SH calls only support scalar i32 and pointer arguments
; RETURN: LLVM ERROR: SH calls only support void, i32, and pointer return values
; TAIL: LLVM ERROR: SH tail calls are not supported
; MUSTTAIL: LLVM ERROR: SH musttail calls are not supported
; EXTERNAL: LLVM ERROR: SH unresolved or interposable direct calls are not supported
; EXTERNAL-NOT: assertion
; RANGE: error: SH branch target is out of range
; RANGE-NOT: assertion
; CROSS: LLVM ERROR: SH cross-section direct calls are not supported
; FRAME: LLVM ERROR: SH stack frame size cannot exceed 60 bytes
; CC: LLVM ERROR: SH only supports the C calling convention
; INLINE-ASM: LLVM ERROR: SH inline assembly is not supported

;--- fifth.ll
define i32 @fifth(ptr %fn) {
	%result = call i32 %fn(i32 1, i32 2, i32 3, i32 4, i32 5)
	ret i32 %result
}

;--- varargs-call.ll
define i32 @varargs_call(ptr %fn) {
	%result = call i32 (i32, ...) %fn(i32 1, i32 2)
	ret i32 %result
}

;--- varargs-definition.ll
define i32 @varargs_definition(ptr %fn, ...) {
	%result = call i32 %fn()
	ret i32 %result
}

;--- i64-argument.ll
define void @i64_argument(ptr %fn) {
	call void %fn(i64 1)
	ret void
}

;--- float-argument.ll
define void @float_argument(ptr %fn) {
	call void %fn(float 1.0)
	ret void
}

;--- aggregate-argument.ll
define void @aggregate_argument(ptr %fn) {
	call void %fn({ i32, i32 } { i32 1, i32 2 })
	ret void
}

;--- byval.ll
define void @byval_argument(ptr %fn, ptr %value) {
	call void %fn(ptr byval(i32) %value)
	ret void
}

;--- sret.ll
define void @sret_argument(ptr %fn, ptr %value) {
	call void %fn(ptr sret(i32) %value)
	ret void
}

;--- i64-return.ll
define i32 @i64_return(ptr %fn) {
	%value = call i64 %fn()
	%result = trunc i64 %value to i32
	ret i32 %result
}

;--- float-return.ll
define i32 @float_return(ptr %fn) {
	%value = call float %fn()
	%result = bitcast float %value to i32
	ret i32 %result
}

;--- aggregate-return.ll
define i32 @aggregate_return(ptr %fn) {
	%value = call { i32, i32 } %fn()
	%result = extractvalue { i32, i32 } %value, 0
	ret i32 %result
}

;--- tail.ll
define i32 @tail_call(ptr %fn, i32 %value) {
	%result = tail call i32 %fn(i32 %value)
	ret i32 %result
}

;--- musttail.ll
define i32 @musttail_call(ptr %fn, i32 %value) {
	%result = musttail call i32 %fn(ptr %fn, i32 %value)
	ret i32 %result
}

;--- external.ll
declare i32 @external_callee(i32)

define i32 @external_call(i32 %value) {
	%result = call i32 @external_callee(i32 %value)
	ret i32 %result
}

;--- cross-section.ll
define internal i32 @other_section(i32 %value) section ".text.other" {
	ret i32 %value
}

define i32 @cross_section(i32 %value) {
	%result = call i32 @other_section(i32 %value)
	ret i32 %result
}

;--- out-of-range.ll
define dso_local i32 @out_of_range_call(i32 %value) {
	%result = call i32 @far_aligned_callee(i32 %value)
	ret i32 %result
}

define internal i32 @far_aligned_callee(i32 %value) align 8192 {
	ret i32 %value
}

;--- large-frame.ll
define internal i32 @identity(i32 %value) {
	ret i32 %value
}

define i32 @large_call_frame(i32 %value) {
	%s0 = alloca i32, align 4
	%s1 = alloca i32, align 4
	%s2 = alloca i32, align 4
	%s3 = alloca i32, align 4
	%s4 = alloca i32, align 4
	%s5 = alloca i32, align 4
	%s6 = alloca i32, align 4
	%s7 = alloca i32, align 4
	%s8 = alloca i32, align 4
	%s9 = alloca i32, align 4
	%s10 = alloca i32, align 4
	%s11 = alloca i32, align 4
	%s12 = alloca i32, align 4
	%s13 = alloca i32, align 4
	%s14 = alloca i32, align 4
	store volatile i32 %value, ptr %s0, align 4
	store volatile i32 %value, ptr %s1, align 4
	store volatile i32 %value, ptr %s2, align 4
	store volatile i32 %value, ptr %s3, align 4
	store volatile i32 %value, ptr %s4, align 4
	store volatile i32 %value, ptr %s5, align 4
	store volatile i32 %value, ptr %s6, align 4
	store volatile i32 %value, ptr %s7, align 4
	store volatile i32 %value, ptr %s8, align 4
	store volatile i32 %value, ptr %s9, align 4
	store volatile i32 %value, ptr %s10, align 4
	store volatile i32 %value, ptr %s11, align 4
	store volatile i32 %value, ptr %s12, align 4
	store volatile i32 %value, ptr %s13, align 4
	store volatile i32 %value, ptr %s14, align 4
	%result = call i32 @identity(i32 %value)
	ret i32 %result
}

;--- calling-convention.ll
define i32 @unsupported_cc(ptr %fn) {
	%result = call fastcc i32 %fn()
	ret i32 %result
}

;--- inline-asm.ll
define void @inline_asm() {
	call void asm sideeffect "", ""()
	ret void
}
