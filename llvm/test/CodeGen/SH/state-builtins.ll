; RUN: llc -mtriple=shle -mcpu=sh4 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh -mcpu=sh4 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle -mcpu=sh4 -force-new-pm-codegen -verify-machineinstrs %s -o /dev/null
; RUN: opt -passes='default<O3>' -S %s | FileCheck %s --check-prefix=IR

declare ptr @llvm.sh.get.thread.pointer()
declare void @llvm.sh.set.thread.pointer(ptr)
declare i32 @llvm.sh.get.fpscr()
declare void @llvm.sh.set.fpscr(i32)
declare float @llvm.sh.fipr(<4 x float>, <4 x float>)
declare void @writer(ptr)

; CHECK-LABEL: gbr:
; CHECK: stc gbr,
; CHECK: ldc {{.*}},gbr
; CHECK: stc gbr,
; CHECK: jsr
; CHECK: stc gbr,
; IR-LABEL: define ptr @gbr(
; IR: call ptr @llvm.sh.get.thread.pointer()
; IR: call void @llvm.sh.set.thread.pointer(
; IR: call ptr @llvm.sh.get.thread.pointer()
; IR: call void @writer(
; IR: call ptr @llvm.sh.get.thread.pointer()
define ptr @gbr(ptr %p, ptr %out) {
  %a = call ptr @llvm.sh.get.thread.pointer()
  call void @llvm.sh.set.thread.pointer(ptr %p)
  %b = call ptr @llvm.sh.get.thread.pointer()
  call void @writer(ptr %a)
  %c = call ptr @llvm.sh.get.thread.pointer()
  store ptr %a, ptr %out
  %next = getelementptr ptr, ptr %out, i32 1
  store ptr %b, ptr %next
  ret ptr %c
}

; Getter alone must not establish entry precision/bank state.
; CHECK-LABEL: get_fpscr:
; CHECK: sts fpscr,r0
; CHECK-NOT: lds
; CHECK: rts
; IR-LABEL: define i32 @get_fpscr(
; IR: call i32 @llvm.sh.get.fpscr()
define i32 @get_fpscr() {
  %v = call i32 @llvm.sh.get.fpscr()
  ret i32 %v
}

; Requested nonmode fields replace old state, while 0x00380000 is retained.
; CHECK-LABEL: set_fpscr:
; CHECK: sts fpscr,[[OLD:r[0-9]+]]
; CHECK-DAG: xor r4,[[OLD]]
; CHECK-DAG: mov #56,[[MASK:r[0-9]+]]
; CHECK-DAG: shll16 [[MASK]]
; CHECK: and [[MASK]],[[OLD]]
; CHECK: xor r4,[[OLD]]
; CHECK: lds [[OLD]],fpscr
; CHECK: sts fpscr,
; CHECK: rts
define i32 @set_fpscr(i32 %v) {
  call void @llvm.sh.set.fpscr(i32 %v)
  %r = call i32 @llvm.sh.get.fpscr()
  ret i32 %r
}

; Discarded graphics/status operations remain ordered with FP control access.
; IR-LABEL: define i32 @status(
; IR: call i32 @llvm.sh.get.fpscr()
; IR: call float @llvm.sh.fipr(
; IR: call void @llvm.sh.set.fpscr(
; IR: call float @llvm.sh.fipr(
; IR: call i32 @llvm.sh.get.fpscr()
define i32 @status(<4 x float> %a, <4 x float> %b) {
  %before = call i32 @llvm.sh.get.fpscr()
  %discard = call float @llvm.sh.fipr(<4 x float> %a, <4 x float> %b)
  call void @llvm.sh.set.fpscr(i32 %before)
  %discard2 = call float @llvm.sh.fipr(<4 x float> %b, <4 x float> %a)
  %after = call i32 @llvm.sh.get.fpscr()
  ret i32 %after
}
