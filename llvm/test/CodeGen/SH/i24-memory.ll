; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s
; R27 explicitly spells out three bytes in prepared IR. Direct admission to
; DAG legalization remains disabled; volatile/vector forms remain rejected.
; CHECK-LABEL: load_u24:
; CHECK: mov.b
define i32 @load_u24(ptr %p) {
  %v = load i24, ptr %p, align 1
  %r = zext i24 %v to i32
  ret i32 %r
}
; CHECK-LABEL: load_s24:
; CHECK: mov.b
define i32 @load_s24(ptr %p) {
  %v = load i24, ptr %p, align 1
  %r = sext i24 %v to i32
  ret i32 %r
}
; CHECK-LABEL: store_u24:
; CHECK: mov.b
define void @store_u24(ptr %p, i32 %x) {
  %v = trunc i32 %x to i24
  store i24 %v, ptr %p, align 1
  ret void
}
