; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector-compare.ll -o - | FileCheck %s --check-prefix=VECTOR-COMPARE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i128-compare.ll -o - | FileCheck %s --check-prefix=I128-COMPARE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector-select.ll -o - | FileCheck %s --check-prefix=VECTOR-SELECT
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 %t/address-space-compare.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS-SPACE

; VECTOR-COMPARE-LABEL: vector_compare:
; VECTOR-COMPARE: mov	#0,r0
; I128-COMPARE-LABEL: i128_compare:
; I128-COMPARE: mov	#1,r0
; VECTOR-SELECT-LABEL: vector_select:
; VECTOR-SELECT: tst	r4,r4
; ADDRESS-SPACE: LLVM ERROR: SH nonzero address spaces are not supported

;--- vector-compare.ll
define i32 @vector_compare() {
	%condition = icmp eq <2 x i32> <i32 1, i32 2>, <i32 3, i32 4>
	%element = extractelement <2 x i1> %condition, i32 0
	%result = zext i1 %element to i32
	ret i32 %result
}

;--- i128-compare.ll
define i32 @i128_compare() {
	%condition = icmp ult i128 18446744073709551616, 340282366920938463463374607431768211455
	%result = zext i1 %condition to i32
	ret i32 %result
}

;--- vector-select.ll
define i32 @vector_select(i32 %selector) {
	%condition = icmp ne i32 %selector, 0
	%selected = select i1 %condition, <2 x i32> <i32 1, i32 2>, <2 x i32> <i32 3, i32 4>
	%result = extractelement <2 x i32> %selected, i32 0
	ret i32 %result
}

;--- address-space-compare.ll
@address_space_a = external addrspace(1) global i32
@address_space_b = external addrspace(1) global i32

define i32 @address_space_compare() {
	%condition = icmp eq ptr addrspace(1) @address_space_a, @address_space_b
	%result = zext i1 %condition to i32
	ret i32 %result
}
