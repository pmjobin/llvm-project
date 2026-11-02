; RUN: split-file %s %t
; RUN: llc -mtriple=shle-unknown-linux-gnu -verify-machineinstrs %t/float.ll -o - | FileCheck %s
; RUN: not llc -mtriple=shle-unknown-linux-gnu %t/trunc-double.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TRUNC
; RUN: not llc -mtriple=shle-unknown-linux-gnu %t/log-double.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=LOG
; A 32-bit C double cannot implement either LLVM f64 operation.
; CHECK-LABEL: e_trunc:
; CHECK: .long truncf
; CHECK-LABEL: e_log:
; CHECK: .long log10f
; TRUNC: error: no libcall available for ftrunc
; LOG: error: no libcall available for flog10
;--- float.ll
declare float @llvm.trunc.f32(float)
declare float @llvm.log10.f32(float)
define float @e_trunc(float %x) #0 {
  %r = call float @llvm.trunc.f32(float %x)
  ret float %r
}
define float @e_log(float %x) #0 {
  %r = call float @llvm.log10.f32(float %x)
  ret float %r
}
attributes #0 = { "target-cpu"="sh4" "target-abi"="gnu-e-float" "target-features"="+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode" }
;--- trunc-double.ll
declare double @llvm.trunc.f64(double)
define double @e_trunc_double(double %x) #0 {
  %r = call double @llvm.trunc.f64(double %x)
  ret double %r
}
attributes #0 = { "target-cpu"="sh4" "target-abi"="gnu-e-float" "target-features"="+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode" }
;--- log-double.ll
declare double @llvm.log10.f64(double)
define double @e_log_double(double %x) #0 {
  %r = call double @llvm.log10.f64(double %x)
  ret double %r
}
attributes #0 = { "target-cpu"="sh4" "target-abi"="gnu-e-float" "target-features"="+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode" }
