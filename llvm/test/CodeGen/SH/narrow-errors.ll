; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/unaligned-load.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNALIGNED-LOAD
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/unaligned-store.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNALIGNED-STORE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/atomic-byte-load.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ATOMIC-LOAD
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/atomic-byte-store.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ATOMIC-STORE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/atomic-word-load.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ATOMIC-LOAD
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/atomic-word-store.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ATOMIC-STORE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i8-argument.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i16-argument.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i8-return.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=RETURN
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i16-return.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=RETURN
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/narrow-stack-call.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CALL-ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/narrow-stack-call-i16.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CALL-ARGUMENT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/narrow-vararg.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VARARG
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/narrow-vararg-i16.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VARARG
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/variable-shift-i8.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARITHMETIC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/variable-shift-i16.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ARITHMETIC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VECTOR
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/materialized-compare.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MATERIALIZED

; UNALIGNED-LOAD: LLVM ERROR: SH requires 2-byte alignment for 16-bit loads
; UNALIGNED-STORE: LLVM ERROR: SH requires 2-byte alignment for 16-bit stores
; ATOMIC-LOAD: LLVM ERROR: SH generic atomic operation is not supported for this type
; ATOMIC-STORE: LLVM ERROR: SH generic atomic operation is not supported for this type
; ARGUMENT: LLVM ERROR: SH function arguments must be scalar i32, i64, or pointers
; RETURN: LLVM ERROR: SH functions only support void, i32, i64, and pointer return values
; CALL-ARGUMENT: LLVM ERROR: SH calls only support scalar i32, i64, and pointer arguments
; VARARG: LLVM ERROR: SH variadic arguments must use supported default-promoted ABI types
; ARITHMETIC: LLVM ERROR: SH variable narrow integer shifts are not supported
; VECTOR: LLVM ERROR: SH only supports 8-, 16-, 32-, and 64-bit integer and pointer loads
; MATERIALIZED: LLVM ERROR: SH comparison results may only be used by conditional branches

;--- unaligned-load.ll
define i32 @unaligned_load(ptr %p) {
	%value = load i16, ptr %p, align 1
	%result = zext i16 %value to i32
	ret i32 %result
}

;--- unaligned-store.ll
define void @unaligned_store(ptr %p, i32 %value) {
	%narrow = trunc i32 %value to i16
	store i16 %narrow, ptr %p, align 1
	ret void
}

;--- atomic-byte-load.ll
define i32 @atomic_byte_load(ptr %p) {
	%value = load atomic i8, ptr %p monotonic, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

;--- atomic-byte-store.ll
define void @atomic_byte_store(ptr %p, i32 %value) {
	%narrow = trunc i32 %value to i8
	store atomic i8 %narrow, ptr %p monotonic, align 1
	ret void
}

;--- atomic-word-load.ll
define i32 @atomic_word_load(ptr %p) {
	%value = load atomic i16, ptr %p monotonic, align 2
	%result = zext i16 %value to i32
	ret i32 %result
}

;--- atomic-word-store.ll
define void @atomic_word_store(ptr %p, i32 %value) {
	%narrow = trunc i32 %value to i16
	store atomic i16 %narrow, ptr %p monotonic, align 2
	ret void
}

;--- i8-argument.ll
define i32 @i8_argument(i8 %value) {
	%result = zext i8 %value to i32
	ret i32 %result
}

;--- i16-argument.ll
define i32 @i16_argument(i16 %value) {
	%result = zext i16 %value to i32
	ret i32 %result
}

;--- i8-return.ll
define i8 @i8_return(i32 %value) {
	%result = trunc i32 %value to i8
	ret i8 %result
}

;--- i16-return.ll
define i16 @i16_return(i32 %value) {
	%result = trunc i32 %value to i16
	ret i16 %result
}

;--- narrow-stack-call.ll
declare void @stack_callee(i32, i32, i32, i32, i8)

define void @narrow_stack_call(i32 %value) {
	%narrow = trunc i32 %value to i8
	call void @stack_callee(i32 0, i32 1, i32 2, i32 3, i8 %narrow)
	ret void
}

;--- narrow-stack-call-i16.ll
declare void @stack_callee_i16(i32, i32, i32, i32, i16)

define void @narrow_stack_call_i16(i32 %value) {
	%narrow = trunc i32 %value to i16
	call void @stack_callee_i16(i32 0, i32 1, i32 2, i32 3, i16 %narrow)
	ret void
}

;--- narrow-vararg.ll
declare void @vararg_callee(i32, ...)

define void @narrow_vararg(i32 %value) {
	%narrow = trunc i32 %value to i8
	call void (i32, ...) @vararg_callee(i32 0, i8 %narrow)
	ret void
}

;--- narrow-vararg-i16.ll
declare void @vararg_callee_i16(i32, ...)

define void @narrow_vararg_i16(i32 %value) {
	%narrow = trunc i32 %value to i16
	call void (i32, ...) @vararg_callee_i16(i32 0, i16 %narrow)
	ret void
}

;--- variable-shift-i8.ll
define void @variable_shift_i8(ptr %value_address, ptr %count_address, ptr %out) {
	%value = load i8, ptr %value_address, align 1
	%count = load i8, ptr %count_address, align 1
	%result = shl i8 %value, %count
	store i8 %result, ptr %out, align 1
	ret void
}

;--- variable-shift-i16.ll
define void @variable_shift_i16(ptr %value_address, ptr %count_address, ptr %out) {
	%value = load i16, ptr %value_address, align 2
	%count = load i16, ptr %count_address, align 2
	%result = ashr i16 %value, %count
	store i16 %result, ptr %out, align 2
	ret void
}

;--- vector.ll
define void @vector_narrow(ptr %p) {
	%value = load <4 x i8>, ptr %p, align 4
	store volatile <4 x i8> %value, ptr %p, align 4
	ret void
}

;--- materialized-compare.ll
define i32 @materialized_compare(ptr %p, ptr %q) {
	%a = load i8, ptr %p, align 1
	%b = load i8, ptr %q, align 1
	%compare = icmp eq i8 %a, %b
	%result = zext i1 %compare to i32
	ret i32 %result
}
