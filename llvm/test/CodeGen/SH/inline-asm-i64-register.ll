; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s
; The GNU i64 r constraint is represented by two real i32 GPRs.
define i64 @guard_pair(i64 %value) {
  %r = call i64 asm "", "=r,0"(i64 %value)
  ret i64 %r
}
; CHECK-LABEL: guard_pair:
; CHECK: mov
; CHECK: rts

define i64 @guard_pair_add(i64 %value) {
  %r = call i64 asm sideeffect "", "=r,0"(i64 %value)
  %next = add i64 %r, 4294967297
  ret i64 %next
}
; CHECK-LABEL: guard_pair_add:
; CHECK: addc
; CHECK: rts
