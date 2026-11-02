; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector-mul.ll -o - | FileCheck %s --check-prefix=MUL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector-div.ll -o - | FileCheck %s --check-prefix=DIV
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/saturating.ll -o /dev/null

; MUL-LABEL: vector_mul:
; MUL: mul.l	r4,r5
; DIV-LABEL: vector_div:
; DIV: div0u
;--- vector-mul.ll
define i32 @vector_mul(i32 %a, i32 %b) {
	%va = insertelement <2 x i32> poison, i32 %a, i32 0
	%vb = insertelement <2 x i32> poison, i32 %b, i32 0
	%product = mul <2 x i32> %va, %vb
	%result = extractelement <2 x i32> %product, i32 0
	ret i32 %result
}

;--- vector-div.ll
define i32 @vector_div(i32 %a, i32 %b) {
	%va = insertelement <2 x i32> poison, i32 %a, i32 0
	%vb = insertelement <2 x i32> poison, i32 %b, i32 0
	%quotient = udiv <2 x i32> %va, %vb
	%result = extractelement <2 x i32> %quotient, i32 0
	ret i32 %result
}

;--- saturating.ll
declare i32 @llvm.smul.fix.sat.i32(i32, i32, i32 immarg)

define i32 @saturating_multiply(i32 %a, i32 %b) {
	%result = call i32 @llvm.smul.fix.sat.i32(i32 %a, i32 %b, i32 0)
	ret i32 %result
}
