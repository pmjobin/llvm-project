; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -filetype=obj < %s | llvm-readobj -r - | FileCheck %s --check-prefix=OBJ
; OBJ: R_SH_DIR32 truncf
; OBJ: R_SH_DIR32 trunc 0x0
; OBJ: R_SH_DIR32 log10f
; OBJ: R_SH_DIR32 log10 0x0
declare float @llvm.trunc.f32(float)
declare double @llvm.trunc.f64(double)
declare float @llvm.log10.f32(float)
declare double @llvm.log10.f64(double)
; CHECK-LABEL: trunc_f32:
; CHECK: .long truncf{{$}}
define float @trunc_f32(float %x) {
  %r = call float @llvm.trunc.f32(float %x)
  ret float %r
}
; CHECK-LABEL: trunc_f64:
; CHECK: .long trunc{{$}}
define double @trunc_f64(double %x) {
  %r = call double @llvm.trunc.f64(double %x)
  ret double %r
}
; CHECK-LABEL: log10_f32:
; CHECK: .long log10f{{$}}
define float @log10_f32(float %x) {
  %r = call float @llvm.log10.f32(float %x)
  ret float %r
}
; CHECK-LABEL: log10_f64:
; CHECK: .long log10{{$}}
define double @log10_f64(double %x) {
  %r = call double @llvm.log10.f64(double %x)
  ret double %r
}
