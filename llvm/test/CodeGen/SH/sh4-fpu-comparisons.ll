; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s --implicit-check-not=__eqsf2 --implicit-check-not=__unordsf2 --implicit-check-not=__eqdf2 --implicit-check-not=__unorddf2
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s --implicit-check-not=__eqsf2 --implicit-check-not=__unordsf2 --implicit-check-not=__eqdf2 --implicit-check-not=__unorddf2

; CHECK-LABEL: f32_unordered:
; CHECK: fcmp/eq [[FB:fr[0-9]+]],[[FB]]
; CHECK: movt
; CHECK: fcmp/eq [[FA:fr[0-9]+]],[[FA]]
; CHECK: movt
; CHECK: and
; CHECK: xor

define i32 @f32_unordered(float %a, float %b) {
	%comparison = fcmp uno float %a, %b
	%result = zext i1 %comparison to i32
	ret i32 %result
}

; CHECK-LABEL: f32_ordered:
; CHECK: fcmp/eq [[FB:fr[0-9]+]],[[FB]]
; CHECK: fcmp/eq [[FA:fr[0-9]+]],[[FA]]
; CHECK: and

define i32 @f32_ordered(float %a, float %b) {
	%comparison = fcmp ord float %a, %b
	%result = zext i1 %comparison to i32
	ret i32 %result
}

; CHECK-LABEL: f32_unordered_equal:
; CHECK: fcmp/eq
; CHECK: fcmp/eq [[FA:fr[0-9]+]],[[FA]]
; CHECK: fcmp/eq [[FB:fr[0-9]+]],[[FB]]
; CHECK: and
; CHECK: xor
; CHECK: or

define i32 @f32_unordered_equal(float %a, float %b) {
	%comparison = fcmp ueq float %a, %b
	%result = zext i1 %comparison to i32
	ret i32 %result
}

; CHECK-LABEL: f32_ordered_not_equal:
; CHECK: fcmp/eq
; CHECK: xor
; CHECK: fcmp/eq [[FB:fr[0-9]+]],[[FB]]
; CHECK: fcmp/eq [[FA:fr[0-9]+]],[[FA]]
; CHECK: and
; CHECK: and

define i32 @f32_ordered_not_equal(float %a, float %b) {
	%comparison = fcmp one float %a, %b
	%result = zext i1 %comparison to i32
	ret i32 %result
}

; CHECK-LABEL: f32_unordered_greater:
; CHECK: fcmp/eq
; CHECK: fcmp/gt
; CHECK: or
; CHECK: xor

define i32 @f32_unordered_greater(float %a, float %b) {
	%comparison = fcmp ugt float %a, %b
	%result = zext i1 %comparison to i32
	ret i32 %result
}

; CHECK-LABEL: f64_unordered:
; CHECK: fcmp/eq [[DB:dr[0-9]+]],[[DB]]
; CHECK: fcmp/eq [[DA:dr[0-9]+]],[[DA]]
; CHECK: and
; CHECK: xor

define i32 @f64_unordered(double %a, double %b) {
	%comparison = fcmp uno double %a, %b
	%result = zext i1 %comparison to i32
	ret i32 %result
}

; CHECK-LABEL: f64_ordered_not_equal:
; CHECK: fcmp/eq
; CHECK: xor
; CHECK: fcmp/eq [[DB:dr[0-9]+]],[[DB]]
; CHECK: fcmp/eq [[DA:dr[0-9]+]],[[DA]]
; CHECK: and
; CHECK: and

define i32 @f64_ordered_not_equal(double %a, double %b) {
	%comparison = fcmp one double %a, %b
	%result = zext i1 %comparison to i32
	ret i32 %result
}

; CHECK-LABEL: f64_unordered_less_equal:
; CHECK: fcmp/gt
; CHECK: xor

define i32 @f64_unordered_less_equal(double %a, double %b) {
	%comparison = fcmp ule double %a, %b
	%result = zext i1 %comparison to i32
	ret i32 %result
}
