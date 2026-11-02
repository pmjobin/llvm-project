; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -filetype=obj < %s | llvm-readobj -r - | FileCheck %s --check-prefix=OBJ
; OBJ: R_SH_DIR32 roundf
; OBJ: R_SH_DIR32 round 0x0
; OBJ: R_SH_DIR32 fminf
; OBJ: R_SH_DIR32 fmin 0x0
; OBJ: R_SH_DIR32 fmaxf
; OBJ: R_SH_DIR32 fmax 0x0

; Wren's unmodified C sources lower these public libm operations to intrinsics.
; Volatile stores preserve the result without a tail call. The f64 cases also
; exercise size optimization, which must retain the ordinary GNU double ABI.
declare float @llvm.round.f32(float)
declare double @llvm.round.f64(double)
declare float @llvm.minnum.f32(float, float)
declare double @llvm.minnum.f64(double, double)
declare float @llvm.maxnum.f32(float, float)
declare double @llvm.maxnum.f64(double, double)

; CHECK-LABEL: round_f32:
; CHECK: jsr
; CHECK: .long roundf{{$}}
define void @round_f32(float %x, ptr %out) #0 {
  %r = call float @llvm.round.f32(float %x)
  store volatile float %r, ptr %out
  ret void
}
; CHECK-LABEL: round_f64:
; CHECK: jsr
; CHECK: .long round{{$}}
define void @round_f64(double %x, ptr %out) #1 {
  %r = call double @llvm.round.f64(double %x)
  store volatile double %r, ptr %out
  ret void
}
; CHECK-LABEL: min_f32:
; CHECK: jsr
; CHECK: .long fminf{{$}}
define void @min_f32(float %x, float %y, ptr %out) #0 {
  %r = call float @llvm.minnum.f32(float %x, float %y)
  store volatile float %r, ptr %out
  ret void
}
; CHECK-LABEL: min_f64:
; CHECK: jsr
; CHECK: .long fmin{{$}}
define void @min_f64(double %x, double %y, ptr %out) #1 {
  %r = call double @llvm.minnum.f64(double %x, double %y)
  store volatile double %r, ptr %out
  ret void
}
; CHECK-LABEL: max_f32:
; CHECK: jsr
; CHECK: .long fmaxf{{$}}
define void @max_f32(float %x, float %y, ptr %out) #0 {
  %r = call float @llvm.maxnum.f32(float %x, float %y)
  store volatile float %r, ptr %out
  ret void
}
; CHECK-LABEL: max_f64:
; CHECK: jsr
; CHECK: .long fmax{{$}}
define void @max_f64(double %x, double %y, ptr %out) #1 {
  %r = call double @llvm.maxnum.f64(double %x, double %y)
  store volatile double %r, ptr %out
  ret void
}
attributes #0 = { noinline }
attributes #1 = { noinline optsize minsize }
