; RUN: llc -mtriple=shle -mcpu=sh4 -stop-after=sh-prepare-ir %s -o - | FileCheck %s
; RUN: llc -mtriple=shle -mcpu=sh4 -enable-new-pm=1 -stop-after=sh-prepare-ir-module %s -o - | FileCheck %s

declare void @llvm.sh.ftrv4(ptr, ptr, ptr)
declare void @llvm.sh.ftrv.array(ptr, ptr, i32, ptr)

; The product is observable through FP status even when count is zero.
; CHECK-LABEL: define void @zero_consumer(
; CHECK: call void @llvm.sh.ftrv4
; CHECK: call void @llvm.sh.ftrv.array
define void @zero_consumer(ptr %d, ptr %s, ptr %a, ptr %b) {
  %tmp = alloca [4 x <4 x float>], align 16
  call void @llvm.sh.ftrv4(ptr %tmp, ptr %b, ptr %a)
  call void @llvm.sh.ftrv.array(ptr %d, ptr %s, i32 0, ptr %tmp)
  ret void
}

; Operand bundles can expose an otherwise private temporary.
; CHECK-LABEL: define void @bundle(
; CHECK: call void @llvm.sh.ftrv4{{.*}}[ "deopt"(ptr %tmp) ]
; CHECK: call void @llvm.sh.ftrv4
define void @bundle(ptr %d, ptr %s, ptr %a, ptr %b) {
  %tmp = alloca [4 x <4 x float>], align 16
  call void @llvm.sh.ftrv4(ptr %tmp, ptr %b, ptr %a) [ "deopt"(ptr %tmp) ]
  call void @llvm.sh.ftrv4(ptr %d, ptr %s, ptr %tmp)
  ret void
}

; Intervening stores may change a matrix or array. Keep instruction ordering.
; CHECK-LABEL: define void @intervening_store(
; CHECK: call void @llvm.sh.ftrv4
; CHECK: store float
; CHECK: call void @llvm.sh.ftrv4
define void @intervening_store(ptr %d, ptr %s, ptr %a, ptr %b) {
  %tmp = alloca [4 x <4 x float>], align 16
  call void @llvm.sh.ftrv4(ptr %tmp, ptr %b, ptr %a)
  store float 0.0, ptr %a
  call void @llvm.sh.ftrv4(ptr %d, ptr %s, ptr %tmp)
  ret void
}
