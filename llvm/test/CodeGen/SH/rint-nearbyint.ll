; RUN: not llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 < %s -o /dev/null 2>&1 | FileCheck %s
; Nearbyint has a different exception contract and remains unsupported.
; CHECK-COUNT-2: error: no libcall available for fnearbyint
declare float @llvm.nearbyint.f32(float)
declare double @llvm.nearbyint.f64(double)
define float @near_f32(float %x) {
  %r = call float @llvm.nearbyint.f32(float %x)
  ret float %r
}
define double @near_f64(double %x) {
  %r = call double @llvm.nearbyint.f64(double %x)
  ret double %r
}
