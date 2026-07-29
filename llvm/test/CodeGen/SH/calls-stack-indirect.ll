; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define i32 @call_ptr8(ptr %fn, i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6, i32 %a7) nounwind {
; CHECK-LABEL: call_ptr8:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK: mov	r4,[[CALLEE:r(0|1|2|3|8|9|10|11|12|13|14)]]
; CHECK: add	#-16,r15
; CHECK-DAG: mov.l	{{r[0-9]+}},@r15
; CHECK-DAG: mov.l	{{r[0-9]+}},@(4,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(8,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(12,r15)
; CHECK: jsr	@[[CALLEE]]
; CHECK-NEXT: nop
; CHECK-NEXT: add	#16,r15
; CHECK: lds.l	@r15+,pr
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%result = call i32 %fn(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6, i32 %a7)
	ret i32 %result
}

define ptr @callee_pointer_is_fifth_stack_value(ptr %fn, i32 %a0, i32 %a1, i32 %a2, i32 %a3, ptr %a5, i32 %a6, i32 %a7) nounwind {
; CHECK-LABEL: callee_pointer_is_fifth_stack_value:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK: mov	r4,[[PTR_CALLEE:r(0|1|2|3|8|9|10|11|12|13|14)]]
; CHECK: add	#-16,r15
; CHECK-DAG: mov.l	[[PTR_CALLEE]],@r15
; CHECK-DAG: mov.l	{{r[0-9]+}},@(4,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(8,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(12,r15)
; CHECK: jsr	@[[PTR_CALLEE]]
; CHECK-NEXT: nop
; CHECK-NEXT: add	#16,r15
; CHECK: lds.l	@r15+,pr
	%result = call ptr %fn(i32 %a0, i32 %a1, i32 %a2, i32 %a3, ptr %fn, ptr %a5, i32 %a6, i32 %a7)
	ret ptr %result
}
