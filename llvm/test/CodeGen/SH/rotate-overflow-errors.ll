; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector-rotate.ll -o - | FileCheck %s --check-prefix=VECTOR
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i128-rotate.ll -o - | FileCheck %s --check-prefix=I128
; RUN: not opt -passes=verify -disable-output %t/malformed-overflow.ll 2>&1 | FileCheck %s --check-prefix=MALFORMED

; VECTOR-LABEL: vector_rotate:
; VECTOR: rotl
; I128-LABEL: i128_rotate:
; I128: shll
; I128: shlr
; MALFORMED: intrinsic return type expected literal non-packed struct with 2 elements, but got i32

;--- vector-rotate.ll
declare <2 x i32> @llvm.fshl.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)

define i32 @vector_rotate(i32 %value, i32 %count) {
	%vector_value = insertelement <2 x i32> poison, i32 %value, i32 0
	%vector_count = insertelement <2 x i32> poison, i32 %count, i32 0
	%rotated = call <2 x i32> @llvm.fshl.v2i32(<2 x i32> %vector_value, <2 x i32> %vector_value, <2 x i32> %vector_count)
	%result = extractelement <2 x i32> %rotated, i32 0
	ret i32 %result
}

;--- i128-rotate.ll
declare i128 @llvm.fshl.i128(i128, i128, i128)

define i32 @i128_rotate(i32 %value, i32 %count) {
	%wide_value = zext i32 %value to i128
	%wide_count = zext i32 %count to i128
	%rotated = call i128 @llvm.fshl.i128(i128 %wide_value, i128 %wide_value, i128 %wide_count)
	%result = trunc i128 %rotated to i32
	ret i32 %result
}

;--- malformed-overflow.ll
declare i32 @llvm.uadd.with.overflow.i32(i32, i32)

define i32 @malformed_overflow(i32 %a, i32 %b) {
	%result = call i32 @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
	ret i32 %result
}
