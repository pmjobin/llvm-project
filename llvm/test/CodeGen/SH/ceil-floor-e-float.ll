; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-linux-gnu -O0 -verify-machineinstrs %t/positive.ll -o - | FileCheck %s --check-prefix=POSITIVE
; RUN: llc -mtriple=sh-unknown-linux-gnu -O3 -verify-machineinstrs %t/positive.ll -o - | FileCheck %s --check-prefix=POSITIVE
; RUN: not llc -mtriple=sh-unknown-linux-gnu -verify-machineinstrs %t/negative.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=NEGATIVE
; RUN: llc -mtriple=shle-unknown-linux-gnu -O0 -verify-machineinstrs %t/positive.ll -o - | FileCheck %s --check-prefix=POSITIVE
; RUN: llc -mtriple=shle-unknown-linux-gnu -O3 -verify-machineinstrs %t/positive.ll -o - | FileCheck %s --check-prefix=POSITIVE
; RUN: not llc -mtriple=shle-unknown-linux-gnu -verify-machineinstrs %t/negative.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=NEGATIVE

; E-float C double is f32: only the float library entry points are used.
; f64 must stay unavailable rather than calling the incompatible ceil/floor ABI.
; POSITIVE-LABEL: e_ceil:
; POSITIVE: jsr
; POSITIVE: .long	ceilf
; POSITIVE-LABEL: e_floor:
; POSITIVE: jsr
; POSITIVE: .long	floorf
; NEGATIVE: error: no libcall available for fceil
; NEGATIVE: error: no libcall available for ffloor

;--- positive.ll
declare float @llvm.ceil.f32(float)
define void @e_ceil(float %x, ptr %out) #0 {
	%r = call float @llvm.ceil.f32(float %x)
	store volatile float %r, ptr %out, align 4
	ret void
}
declare float @llvm.floor.f32(float)
define void @e_floor(float %x, ptr %out) #0 {
	%r = call float @llvm.floor.f32(float %x)
	store volatile float %r, ptr %out, align 4
	ret void
}
attributes #0 = { "target-cpu"="sh4" "target-abi"="gnu-e-float" "target-features"="+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode" }

;--- negative.ll
declare double @llvm.ceil.f64(double)
define double @e_ceil_double(double %x) #0 {
	%r = call double @llvm.ceil.f64(double %x)
	ret double %r
}
declare double @llvm.floor.f64(double)
define double @e_floor_double(double %x) #0 {
	%r = call double @llvm.floor.f64(double %x)
	ret double %r
}
attributes #0 = { "target-cpu"="sh4" "target-abi"="gnu-e-float" "target-features"="+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode" }
