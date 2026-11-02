; RUN: split-file %s %t
; RUN: llc -mtriple=shle-unknown-linux-gnu -verify-machineinstrs %t/float.ll -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -verify-machineinstrs %t/float.ll -o - | FileCheck %s
; RUN: not llc -mtriple=shle-unknown-linux-gnu %t/double.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ERROR
; RUN: not llc -mtriple=sh-unknown-linux-gnu %t/double.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ERROR
; E-float has a 32-bit C double: its rint entry cannot implement LLVM f64.
; CHECK-LABEL: e_rint:
; CHECK: jsr
; CHECK: .long rintf
; ERROR: error: no libcall available for frint

;--- float.ll
declare float @llvm.rint.f32(float)
define float @e_rint(float %x) #0 {
  %r = call float @llvm.rint.f32(float %x)
  ret float %r
}
attributes #0 = { "target-cpu"="sh4" "target-abi"="gnu-e-float" "target-features"="+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode" }

;--- double.ll
declare double @llvm.rint.f64(double)
define double @e_rint_double(double %x) #0 {
  %r = call double @llvm.rint.f64(double %x)
  ret double %r
}
attributes #0 = { "target-cpu"="sh4" "target-abi"="gnu-e-float" "target-features"="+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode" }
