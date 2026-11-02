; RUN: split-file %s %t
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/arithmetic.ll -o /dev/null 2>&1 | FileCheck %s
; RUN: not llc -mtriple=shle-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs %t/conversion.ll -o /dev/null 2>&1 | FileCheck %s
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/comparison.ll -o /dev/null 2>&1 | FileCheck %s
; RUN: not llc -mtriple=shle-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs %t/intrinsic.ll -o /dev/null 2>&1 | FileCheck %s

; CHECK: error: {{.*}}SH floating-point semantics only support f32 and f64

;--- arithmetic.ll
define i16 @half_arithmetic(i16 %bits) {
	%value = bitcast i16 %bits to half
	%result = fadd half %value, %value
	%result.bits = bitcast half %result to i16
	ret i16 %result.bits
}

;--- conversion.ll
define float @half_conversion(i16 %bits) {
	%value = bitcast i16 %bits to half
	%result = fpext half %value to float
	ret float %result
}

;--- comparison.ll
define i1 @half_comparison(i16 %bits) {
	%value = bitcast i16 %bits to half
	%result = fcmp oeq half %value, %value
	ret i1 %result
}

;--- intrinsic.ll
declare half @llvm.fabs.f16(half)

define i16 @half_intrinsic(i16 %bits) {
	%value = bitcast i16 %bits to half
	%result = call half @llvm.fabs.f16(half %value)
	%result.bits = bitcast half %result to i16
	ret i16 %result.bits
}
