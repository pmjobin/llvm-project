; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i128.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=I128
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/atomic-load.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ATOMIC-LOAD
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/atomic-rmw.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ATOMIC-RMW
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/atomic-cmpxchg.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ATOMIC-RMW
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/unaligned-load.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNALIGNED-LOAD
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/unaligned-store.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNALIGNED-STORE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/overflow.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INTRINSIC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/materialized-compare.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MATERIALIZED
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/select.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SELECT
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/varargs.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VARARGS
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/byval.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=BYVAL
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VECTOR
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/calling-convention.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CALLING-CONVENTION
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/large-outgoing.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=OUTGOING

; I128: LLVM ERROR: SH functions only support void, i32, i64, and pointer return values
; ATOMIC-LOAD: LLVM ERROR: SH atomic loads are not supported
; ATOMIC-RMW: LLVM ERROR: SH atomic read-modify-write operations are not supported
; UNALIGNED-LOAD: LLVM ERROR: SH requires 4-byte alignment for 32- and 64-bit loads
; UNALIGNED-STORE: LLVM ERROR: SH requires 4-byte alignment for 32- and 64-bit stores
; INTRINSIC: LLVM ERROR: SH intrinsics are not supported
; MATERIALIZED: LLVM ERROR: SH comparison results may only be used by conditional branches
; SELECT: LLVM ERROR: SH select is not supported
; VARARGS: LLVM ERROR: SH varargs are not supported
; BYVAL: LLVM ERROR: SH calls only support scalar i32, i64, and pointer arguments
; VECTOR: LLVM ERROR: SH only supports i8, i16, i32, and selected i64 integer operations
; CALLING-CONVENTION: LLVM ERROR: SH only supports the C calling convention
; OUTGOING: LLVM ERROR: SH outgoing call frame size cannot exceed 60 bytes

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

;--- unaligned-load.ll
define i64 @unaligned_load_i64(ptr %address) {
	%value = load i64, ptr %address, align 2
	ret i64 %value
}

;--- unaligned-store.ll
define void @unaligned_store_i64(ptr %address, i64 %value) {
	store i64 %value, ptr %address, align 1
	ret void
}

;--- overflow.ll
declare { i64, i1 } @llvm.uadd.with.overflow.i64(i64, i64)

define i64 @overflow_i64(i64 %a, i64 %b) {
	%pair = call { i64, i1 } @llvm.uadd.with.overflow.i64(i64 %a, i64 %b)
	%value = extractvalue { i64, i1 } %pair, 0
	ret i64 %value
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

;--- varargs.ll
define i64 @varargs_i64(i64 %value, ...) {
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
define fastcc i64 @unsupported_cc_i64(i64 %value) {
	ret i64 %value
}

;--- large-outgoing.ll
define internal void @large_i64_callee(i32 %a, i32 %b, i32 %c, i32 %d, i64 %e, i64 %f, i64 %g, i64 %h, i64 %i, i64 %j, i64 %k, i64 %l) {
	ret void
}

define void @large_i64_outgoing() {
	call void @large_i64_callee(i32 0, i32 1, i32 2, i32 3, i64 4, i64 5, i64 6, i64 7, i64 8, i64 9, i64 10, i64 11)
	ret void
}
