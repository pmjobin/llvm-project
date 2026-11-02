; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector.ll -o - | FileCheck %s --check-prefix=LOGICAL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector-shift.ll -o - | FileCheck %s --check-prefix=SHIFT

; LOGICAL-LABEL: vector_logical:
; LOGICAL: and	r5,r0
; SHIFT-LABEL: vector_shift:
; SHIFT: shll

;--- vector.ll
define i32 @vector_logical(i32 %a, i32 %b) {
	%vector_a = insertelement <2 x i32> poison, i32 %a, i32 0
	%vector_b = insertelement <2 x i32> poison, i32 %b, i32 0
	%logical = and <2 x i32> %vector_a, %vector_b
	%result = extractelement <2 x i32> %logical, i32 0
	ret i32 %result
}

;--- vector-shift.ll
define i32 @vector_shift(i32 %value, i32 %count) {
	%vector_value = insertelement <2 x i32> poison, i32 %value, i32 0
	%vector_count = insertelement <2 x i32> poison, i32 %count, i32 0
	%shifted = shl <2 x i32> %vector_value, %vector_count
	%result = extractelement <2 x i32> %shifted, i32 0
	ret i32 %result
}
