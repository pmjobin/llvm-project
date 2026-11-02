; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,HARD
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,HARD

; Both values must be exact, but only one magnitude needs literal storage.
define double @paired_signs(double %x, double %y) nounwind optsize {
; CHECK-LABEL: paired_signs:
; CHECK: fneg
; CHECK-NOT: .uaquad
; CHECK: .uaquad 4612811918334230528
; CHECK-NOT: .uaquad
	%positive = fadd double %x, 2.500000e+00
	%negative = fmul double %y, -2.500000e+00
	%result = fdiv double %positive, %negative
	ret double %result
}

; A positive use in another function does not justify a sign instruction.
define double @unpaired_sign(double %x) nounwind optsize {
; CHECK-LABEL: unpaired_sign:
; CHECK-NOT: fneg
; CHECK: .uaquad -4610560118520545280
	%result = fadd double %x, -2.500000e+00
	ret double %result
}

; The policy is for size optimization.
define double @speed_signs(double %x, double %y) nounwind {
; CHECK-LABEL: speed_signs:
; CHECK-NOT: fneg
; CHECK: .uaquad
; CHECK: .uaquad
	%positive = fadd double %x, 2.500000e+00
	%negative = fmul double %y, -2.500000e+00
	%result = fdiv double %positive, %negative
	ret double %result
}

; Signed zero and nonfinite encodings keep their original representation.
declare void @consume(double, double)

define void @zero_signs() nounwind optsize {
; CHECK-LABEL: zero_signs:
; CHECK-NOT: fneg
; HARD: .uaquad -9223372036854775808
	call void @consume(double 0.000000e+00, double -0.000000e+00)
	ret void
}

define void @infinite_signs() nounwind optsize {
; CHECK-LABEL: infinite_signs:
; CHECK-NOT: fneg
; HARD-DAG: .uaquad 9218868437227405312
; HARD-DAG: .uaquad -4503599627370496
	call void @consume(double f0x7FF0000000000000, double f0xFFF0000000000000)
	ret void
}

define void @nan_signs() nounwind optsize {
; CHECK-LABEL: nan_signs:
; CHECK-NOT: fneg
; HARD-DAG: .uaquad 9221120237041090561
; HARD-DAG: .uaquad -2251799813685247
	call void @consume(double f0x7FF8000000000001, double f0xFFF8000000000001)
	ret void
}
