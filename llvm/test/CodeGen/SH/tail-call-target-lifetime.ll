; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

declare i32 @callee(i32)
declare void @observe(ptr)

; Reusing the same symbol across calls can put its address in a callee-saved
; register. The final target must survive restoring that register's entry value.
define i32 @reused_target(i32 %value) optsize {
; CHECK-LABEL: reused_target:
; CHECK: jsr @
; CHECK: lds.l @r15+,pr
; CHECK: jmp @{{r[0124567]}}
; CHECK-NOT: rts
	%first = call i32 @callee(i32 %value)
	%result = tail call i32 @callee(i32 %first)
	ret i32 %result
}

; The target must also survive the r3 scratch used to dismantle a large frame.
define i32 @large_frame_target(i32 %value) optsize {
; CHECK-LABEL: large_frame_target:
; CHECK: jsr @
; CHECK: add r3,r15
; CHECK: jmp @{{r[0124567]}}
; CHECK-NOT: rts
	%buffer = alloca [1024 x i32], align 4
	call void @observe(ptr %buffer)
	%first = call i32 @callee(i32 %value)
	%result = tail call i32 @callee(i32 %first)
	ret i32 %result
}
