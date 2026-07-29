; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/varargs-call.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VARARGS-CALL
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/varargs-definition.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VARARGS
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/float-argument.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/byval.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/sret.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SRET
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/float-return.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=RETURN
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/tail.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TAIL
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/musttail.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MUSTTAIL
; RUN: rm -f %t/range.o %t/range-le.o
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %t/out-of-range.ll -o %t/range.o 2>&1 | FileCheck %s --check-prefix=RANGE
; RUN: not test -e %t/range.o
; RUN: not llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %t/out-of-range.ll -o %t/range-le.o 2>&1 | FileCheck %s --check-prefix=RANGE
; RUN: not test -e %t/range-le.o
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/large-frame.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=FRAME
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/calling-convention.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/inline-asm.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INLINE-ASM

; VARARGS-CALL: LLVM ERROR: SH varargs calls are not supported
; VARARGS: LLVM ERROR: SH varargs are not supported
; ARGUMENT: LLVM ERROR: SH calls only support scalar i32, i64, and pointer arguments
; SRET: LLVM ERROR: SH sret requires a fixed aggregate containing only integers and address-space-zero pointers
; RETURN: LLVM ERROR: SH calls only support void, i32, i64, and pointer return values
; TAIL: LLVM ERROR: SH tail calls are not supported
; MUSTTAIL: LLVM ERROR: SH musttail calls are not supported
; RANGE: error: SH branch target is out of range
; RANGE-NOT: assertion
; FRAME: LLVM ERROR: SH stack frame size cannot exceed 60 bytes
; CC: LLVM ERROR: SH only supports the C calling convention
; INLINE-ASM: LLVM ERROR: SH inline assembly is not supported

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

;--- float-argument.ll
define void @float_argument(ptr %fn) {
	call void %fn(float 1.0)
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

;--- float-return.ll
define i32 @float_return(ptr %fn) {
	%value = call float %fn()
	%result = bitcast float %value to i32
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
