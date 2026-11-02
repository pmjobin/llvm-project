; RUN: split-file %s %t
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i128.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=I128
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/atomic-load.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/atomic-rmw.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/atomic-cmpxchg.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/materialized-compare.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/select.ll -o /dev/null
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/byval.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=BYVAL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector.ll -o - | FileCheck %s --check-prefix=VECTOR
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/calling-convention.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CALLING-CONVENTION

; I128: error: {{.*}}SH function return type is not supported by the ABI
; BYVAL: error: {{.*}}SH byval requires a fixed aggregate containing only integers, address-space-zero pointers, f32, and f64
; VECTOR-LABEL: vector_i64:
; VECTOR: add	r5,r0
; CALLING-CONVENTION: error: {{.*}}SH only supports the C and fast calling conventions

;--- i128.ll
define i128 @unsupported_i128(i128 %value) {
	ret i128 %value
}

;--- atomic-load.ll
define i64 @atomic_load_i64(ptr %address) {
	%value = load atomic i64, ptr %address monotonic, align 8
	ret i64 %value
}

;--- atomic-rmw.ll
define i64 @atomic_rmw_i64(ptr %address, i64 %value) {
	%old = atomicrmw add ptr %address, i64 %value monotonic, align 8
	ret i64 %old
}

;--- atomic-cmpxchg.ll
define i64 @atomic_cmpxchg_i64(ptr %address, i64 %expected, i64 %desired) {
	%pair = cmpxchg ptr %address, i64 %expected, i64 %desired monotonic monotonic, align 8
	%old = extractvalue { i64, i1 } %pair, 0
	ret i64 %old
}

;--- materialized-compare.ll
define i32 @materialized_i64_compare(i64 %a, i64 %b) {
	%condition = icmp ult i64 %a, %b
	%value = zext i1 %condition to i32
	ret i32 %value
}

;--- select.ll
define i64 @select_i64(i64 %a, i64 %b) {
	%value = select i1 true, i64 %a, i64 %b
	ret i64 %value
}

;--- byval.ll
declare void @byval_callee(ptr byval(i64))

define void @byval_i64(ptr %address) {
	call void @byval_callee(ptr byval(i64) %address)
	ret void
}

;--- vector.ll
define i32 @vector_i64(i32 %a, i32 %b) {
	%a64 = zext i32 %a to i64
	%b64 = zext i32 %b to i64
	%va = insertelement <2 x i64> poison, i64 %a64, i32 0
	%vb = insertelement <2 x i64> poison, i64 %b64, i32 0
	%sum = add <2 x i64> %va, %vb
	%wide = extractelement <2 x i64> %sum, i32 0
	%result = trunc i64 %wide to i32
	ret i32 %result
}

;--- calling-convention.ll
define coldcc i64 @unsupported_cc_i64(i64 %value) {
	ret i64 %value
}
