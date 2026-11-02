; RUN: llc -mtriple=x86_64-unknown-linux-gnu -mattr=-sse4.1 < %s | FileCheck %s
; Public scalar libm lowering must remain independent of SH's libcall registry.
declare float @llvm.round.f32(float)
declare double @llvm.round.f64(double)
declare float @llvm.minnum.f32(float, float)
declare double @llvm.maxnum.f64(double, double)
; CHECK-LABEL: round_f32:
; CHECK: callq roundf@PLT
define void @round_f32(float %x, ptr %out) {
  %r = call float @llvm.round.f32(float %x)
  store volatile float %r, ptr %out
  ret void
}
; CHECK-LABEL: round_f64:
; CHECK: callq round@PLT
define void @round_f64(double %x, ptr %out) {
  %r = call double @llvm.round.f64(double %x)
  store volatile double %r, ptr %out
  ret void
}
; CHECK-LABEL: min_f32:
; CHECK: minss
define float @min_f32(float %x, float %y) {
  %r = call float @llvm.minnum.f32(float %x, float %y)
  ret float %r
}
; CHECK-LABEL: max_f64:
; CHECK: maxsd
define double @max_f64(double %x, double %y) {
  %r = call double @llvm.maxnum.f64(double %x, double %y)
  ret double %r
}
