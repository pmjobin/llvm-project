; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s

@argument = external global i32, align 4

declare i32 @callee(i32)

define i32 @direct_call_target(i32 %unused) nounwind optsize {
	%arg = load i32, ptr @argument, align 4
	%called = call i32 @callee(i32 %arg)
	%result = add i32 %called, 1
	ret i32 %result
}

; CHECK-LABEL: direct_call_target:
; CHECK: mov.l {{.*}},r0
; CHECK-NEXT: mov.l {{.*}},r1
; CHECK-NEXT: jsr @r1
; CHECK-NEXT: mov.l @r0,r4
