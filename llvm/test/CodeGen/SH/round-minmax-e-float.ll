; RUN: split-file %s %t
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -verify-machineinstrs %t/positive.ll -o - | FileCheck %s
; RUN: not llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 %t/negative.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=NEG
; CHECK-LABEL: float_math:
; CHECK-DAG: .long roundf{{$}}
; CHECK-DAG: .long fminf{{$}}
; CHECK-DAG: .long fmaxf{{$}}
; NEG: error: no libcall available for fround
; NEG: error: no libcall available for fminnum
; NEG: error: no libcall available for fmaxnum

;--- positive.ll
declare float @llvm.round.f32(float)
declare float @llvm.minnum.f32(float, float)
declare float @llvm.maxnum.f32(float, float)
define void @float_math(float %x, float %y, ptr %out) #0 {
  %r = call float @llvm.round.f32(float %x)
  store volatile float %r, ptr %out
  %m = call float @llvm.minnum.f32(float %x, float %y)
  store volatile float %m, ptr %out
  %n = call float @llvm.maxnum.f32(float %x, float %y)
  store volatile float %n, ptr %out
  ret void
}
attributes #0 = { "target-cpu"="sh4" "target-abi"="gnu-e-float" "target-features"="+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode" }

;--- negative.ll
declare double @llvm.round.f64(double)
declare double @llvm.minnum.f64(double, double)
declare double @llvm.maxnum.f64(double, double)
define void @double_math(double %x, double %y, ptr %out) #0 {
  %r = call double @llvm.round.f64(double %x)
  store volatile double %r, ptr %out
  %m = call double @llvm.minnum.f64(double %x, double %y)
  store volatile double %m, ptr %out
  %n = call double @llvm.maxnum.f64(double %x, double %y)
  store volatile double %n, ptr %out
  ret void
}
attributes #0 = { "target-cpu"="sh4" "target-abi"="gnu-e-float" "target-features"="+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode" }
