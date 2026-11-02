; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh3 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -filetype=obj < %s | llvm-readobj -r - | FileCheck %s --check-prefix=OBJ

; Both ordinary and size-optimized scalar rint use their own C libm entry.
; Preserve floating results, including values outside the signed i32 range.
; OBJ: R_SH_DIR32 rintf
; OBJ: R_SH_DIR32 rint 0x0
declare float @llvm.rint.f32(float)
declare double @llvm.rint.f64(double)

; CHECK-LABEL: rint_f32:
; CHECK: jsr
; CHECK: .long rintf{{$}}
define void @rint_f32(float %x, ptr %out) {
  %r = call float @llvm.rint.f32(float %x)
  store volatile float %r, ptr %out
  ret void
}

; CHECK-LABEL: rint_f64:
; CHECK: jsr
; CHECK: .long rint{{$}}
define void @rint_f64(double %x, ptr %out) {
  %r = call double @llvm.rint.f64(double %x)
  store volatile double %r, ptr %out
  ret void
}

; CHECK-LABEL: rint_f32_size:
; CHECK: jsr
; CHECK: .long rintf{{$}}
define void @rint_f32_size(float %x, ptr %out) optsize minsize {
  %r = call float @llvm.rint.f32(float %x)
  store volatile float %r, ptr %out
  ret void
}

; CHECK-LABEL: rint_f64_size:
; CHECK: jsr
; CHECK: .long rint{{$}}
define void @rint_f64_size(double %x, ptr %out) optsize minsize {
  %r = call double @llvm.rint.f64(double %x)
  store volatile double %r, ptr %out
  ret void
}
