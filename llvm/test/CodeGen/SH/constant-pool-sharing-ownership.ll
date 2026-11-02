; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O0 -relocation-model=pic -verify-machineinstrs -stop-after=sh-isel -o - %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O0 -relocation-model=pic -verify-machineinstrs -stop-after=sh-isel -o - %s | FileCheck %s

; The two resolver calls reuse one machine constant-pool value. The pool must
; retain single ownership when the same value is encountered twice.
; CHECK-LABEL: name:{{ *}}two_tls_resolvers
; CHECK: body:
; CHECK-COUNT-2: SH_TLS_CALL
; CHECK-NOT: SH_TLS_CALL

@gd = external thread_local global i32
@ld = internal thread_local(localdynamic) global i32 1

define i32 @two_tls_resolvers() {
  %a = load volatile i32, ptr @gd, align 4
  %b = load volatile i32, ptr @ld, align 4
  %sum = add i32 %a, %b
  ret i32 %sum
}
