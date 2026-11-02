; RUN: llc -O3 -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs -stop-before=finalize-isel %s -o - | FileCheck %s
; RUN: llc -O3 -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs %s -o /dev/null

; CHECK-LABEL: name: size
; CHECK: SELECT_F32_PSEUDO
define i32 @size(float %x) #0 {
	%r = fptoui float %x to i32
	ret i32 %r
}
; CHECK-LABEL: name: minimum
; CHECK: SELECT_F64_PSEUDO
define i32 @minimum(double %x) #1 {
	%r = fptoui double %x to i32
	ret i32 %r
}
; CHECK-LABEL: name: none
; CHECK: SELECT_F32_PSEUDO
define i32 @none(float %x) #2 {
	%r = fptoui float %x to i32
	ret i32 %r
}
; CHECK-LABEL: name: strict
; CHECK: SELECT_F64_PSEUDO
define i32 @strict(double %x) #3 {
	%r = fptoui double %x to i32
	ret i32 %r
}
; CHECK-LABEL: name: trapping
; CHECK: SELECT_F32_PSEUDO
define i32 @trapping(float %x) #4 {
	%r = fptoui float %x to i32
	ret i32 %r
}
; CHECK-LABEL: name: denormal
; CHECK: SELECT_F64_PSEUDO
define i32 @denormal(double %x) #5 {
	%r = fptoui double %x to i32
	ret i32 %r
}
; CHECK-LABEL: name: assembly
; CHECK: SELECT_F32_PSEUDO
define i32 @assembly(float %x) #6 {
	call void asm sideeffect "", "~{fpscr}"()
	%r = fptoui float %x to i32
	ret i32 %r
}
; CHECK-LABEL: name: observation
; CHECK: SELECT_F64_PSEUDO
define i32 @observation(double %x) #6 {
	%r = fptoui double %x to i32
	%flags = call i32 @fetestexcept(i32 31)
	%result = xor i32 %r, %flags
	ret i32 %result
}
declare i32 @fetestexcept(i32)

; An absent no-trapping contract must retain the conservative lowering.
; CHECK-LABEL: name: unspecified_trapping
; CHECK: SELECT_F32_PSEUDO
define i32 @unspecified_trapping(float %x) {
  %r = fptoui float %x to i32
  ret i32 %r
}

attributes #0 = { optsize "no-trapping-math"="true" }
attributes #1 = { minsize optsize "no-trapping-math"="true" }
attributes #2 = { noinline optnone "no-trapping-math"="true" }
attributes #3 = { strictfp "no-trapping-math"="true" }
attributes #4 = { "no-trapping-math"="false" }
attributes #5 = { "no-trapping-math"="true" "denormal-fp-math"="preserve-sign,preserve-sign" }
attributes #6 = { "no-trapping-math"="true" }
