; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s

; GNU/compiler-rt __eq*f2 return zero for equality, rather than boolean true.
define i32 @equal_double(double %a, double %b) {
  %cmp = fcmp oeq double %a, %b
  %r = zext i1 %cmp to i32
  ret i32 %r
}
; CHECK-LABEL: equal_double:
; CHECK: jsr @r0
; CHECK: tst r0,r0
; CHECK-NEXT: movt r0
; CHECK: .long __eqdf2

define i32 @equal_float(float %a, float %b) {
  %cmp = fcmp oeq float %a, %b
  %r = zext i1 %cmp to i32
  ret i32 %r
}
; CHECK-LABEL: equal_float:
; CHECK: jsr @r0
; CHECK: tst r0,r0
; CHECK-NEXT: movt r0
; CHECK: .long __eqsf2
