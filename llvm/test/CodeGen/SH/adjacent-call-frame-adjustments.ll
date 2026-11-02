; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s

declare void @callee5(i32, i32, i32, i32, i32)

define void @consecutive_calls(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4) nounwind {
entry:
	call void @callee5(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4)
	call void @callee5(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4)
	ret void
}

; CHECK-LABEL: consecutive_calls:
; CHECK: add #-4,r15
; CHECK: jsr
; CHECK-NOT: add #4,r15
; CHECK-NOT: add #-4,r15
; CHECK: jsr
; CHECK: add #4,r15
