; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O0 -enable-new-pm=0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O0 -enable-new-pm=0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O0 -enable-new-pm=1 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O0 -enable-new-pm=1 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O2 -enable-new-pm=0 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O2 -enable-new-pm=0 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O2 -enable-new-pm=1 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O2 -enable-new-pm=1 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O3 -enable-new-pm=0 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O3 -enable-new-pm=0 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O3 -enable-new-pm=1 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O3 -enable-new-pm=1 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=OPT

; The first call's result must survive the second call, whose result supplies
; the stored and returned pointer. A singleton R0 compare operand allowed the
; scheduler to put the comparison before the second result's COPY from R0.
; Reloading the spilled first result then required overwriting the still-live
; physical R0 result, so allocation failed. Exercise the complete pipeline:
; the comparison must instead expose its local R0 clobber before scheduling.
; The first result is saved in the second call's delay slot. After that call,
; copy its distinct result out of R0 before restoring the older value for the
; immediate compare. Only the newer result may advance the stored pointer.
; The saved value uses a callee-saved register; the short-lived newer value
; uses a caller-saved temporary. Keep their captures disjoint.
; OPT-LABEL: older_call_return:
; OPT: mov r6,[[ENC:r[0-9]+]]
; OPT: mov.l @(20,[[ENC]]),r0
; OPT: mov r7,[[OUT:r[0-9]+]]
; OPT: jsr @r0
; OPT-NEXT: mov r4,[[BASE:r[0-9]+]]
; OPT: mov.l @[[ENC]],[[CALLEE:r[0-9]+]]
; OPT: jsr @[[CALLEE]]
; OPT-NEXT: mov r0,[[OLD:r(8|9|1[0-4])]]
; OPT-NEXT: mov r0,[[NEW:r[1-7]]]
; OPT-NEXT: mov [[OLD]],r0
; OPT-NEXT: cmp/eq #44,r0
; OPT-NEXT: bf [[OTHER:.LBB[0-9]+_[0-9]+]]
; OPT-NEXT: add [[NEW]],[[BASE]]
; OPT-NEXT: mov.l [[BASE]],@[[OUT]]
; OPT-NEXT: [[OTHER]]:
; OPT: mov [[BASE]],r0
; OPT: rts
define ptr @older_call_return(ptr %p, ptr %end, ptr %enc, ptr %out) {
entry:
  %fptr = getelementptr i8, ptr %enc, i32 20
  %f = load ptr, ptr %fptr, align 4
  %old = call i32 %f(ptr %p, ptr %end)
  %g = load ptr, ptr %enc, align 4
  %new = call i32 %g(ptr %p)
  %next = getelementptr i8, ptr %p, i32 %new
  %cmp = icmp eq i32 %old, 44
  br i1 %cmp, label %match, label %other

match:
  store ptr %next, ptr %out, align 4
  ret ptr %next

other:
  ret ptr %p
}
