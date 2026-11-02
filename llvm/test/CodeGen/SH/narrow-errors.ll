; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/atomic-byte-load.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/atomic-byte-store.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/atomic-word-load.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/atomic-word-store.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i8-argument.ll -o - | FileCheck %s --check-prefix=I8-ARGUMENT
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i16-argument.ll -o - | FileCheck %s --check-prefix=I16-ARGUMENT
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i8-return.ll -o - | FileCheck %s --check-prefix=I8-RETURN
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i16-return.ll -o - | FileCheck %s --check-prefix=I16-RETURN
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/narrow-stack-call.ll -o - | FileCheck %s --check-prefix=I8-CALL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/narrow-stack-call-i16.ll -o - | FileCheck %s --check-prefix=I16-CALL
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/narrow-vararg.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VARARG
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/narrow-vararg-i16.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VARARG
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector.ll -o - | FileCheck %s --check-prefix=VECTOR
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/materialized-compare.ll -o /dev/null

; I8-ARGUMENT-LABEL: i8_argument:
; I8-ARGUMENT: extu.b	r4,r0
; I16-ARGUMENT-LABEL: i16_argument:
; I16-ARGUMENT: extu.w	r4,r0
; I8-RETURN-LABEL: i8_return:
; I8-RETURN: mov	r4,r0
; I16-RETURN-LABEL: i16_return:
; I16-RETURN: mov	r4,r0
; I8-CALL-LABEL: narrow_stack_call:
; I8-CALL: mov.l	r4,@r15
; I8-CALL: jsr
; I16-CALL-LABEL: narrow_stack_call_i16:
; I16-CALL: mov.l	r4,@r15
; I16-CALL: jsr
; VARARG: error: {{.*}}SH variadic arguments must use supported default-promoted ABI types
; VECTOR-LABEL: vector_narrow:
; VECTOR: mov.b	@r1,r0
; VECTOR: mov.b	r0,@r1

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
define void @narrow_stack_call(i32 %value) {
	%narrow = trunc i32 %value to i8
	call void @stack_callee(i32 0, i32 1, i32 2, i32 3, i8 %narrow)
	ret void
}

define internal void @stack_callee(i32 %a, i32 %b, i32 %c, i32 %d, i8 %e) {
	ret void
}

;--- narrow-stack-call-i16.ll
define void @narrow_stack_call_i16(i32 %value) {
	%narrow = trunc i32 %value to i16
	call void @stack_callee_i16(i32 0, i32 1, i32 2, i32 3, i16 %narrow)
	ret void
}

define internal void @stack_callee_i16(i32 %a, i32 %b, i32 %c, i32 %d, i16 %e) {
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
