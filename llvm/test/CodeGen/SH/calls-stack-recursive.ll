; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define internal i32 @recursive_stack_call(i32 %n, i32 %a1, i32 %a2, i32 %a3, i32 %a4) noinline nounwind {
; CHECK-LABEL: recursive_stack_call:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK-NOT: sts.l
; CHECK: mov.l	{{.*}}r15
; CHECK: {{b[tf]}}
; CHECK: add	#-4,r15
; CHECK: mov.l	{{r[0-9]+}},@r15
; CHECK: bsr	recursive_stack_call
; CHECK-NEXT: nop
; CHECK: add	#4,r15
; CHECK-NOT: sts.l
; CHECK: lds.l	@r15+,pr
; CHECK: rts
; CHECK-NEXT: nop
entry:
	%done = icmp eq i32 %n, 0
	br i1 %done, label %base, label %recurse

base:
	ret i32 %a4

recurse:
	%next = add i32 %n, -1
	%next_stack = add i32 %a4, 1
	%called = call i32 @recursive_stack_call(i32 %next, i32 %a1, i32 %a2, i32 %a3, i32 %next_stack)
	%result = add i32 %called, %a4
	ret i32 %result
}

define i32 @start_recursive_stack_call(i32 %n) nounwind {
; CHECK-LABEL: start_recursive_stack_call:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK: add	#-4,r15
; CHECK: mov.l	{{r[0-9]+}},@r15
; CHECK: bsr	recursive_stack_call
; CHECK-NEXT: nop
; CHECK-NEXT: add	#4,r15
; CHECK-NEXT: lds.l	@r15+,pr
	%result = call i32 @recursive_stack_call(i32 %n, i32 1, i32 2, i32 3, i32 4)
	ret i32 %result
}
