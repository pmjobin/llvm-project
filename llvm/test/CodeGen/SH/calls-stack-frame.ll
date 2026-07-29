; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define internal i32 @frame_callee8(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6, i32 %a7) noinline nounwind {
	ret i32 %a7
}

define i32 @fixed_local_and_dynamic_call_frame(i32 %value) nounwind {
; CHECK-LABEL: fixed_local_and_dynamic_call_frame:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK-NEXT: add	#-4,r15
; CHECK: mov.l	{{r[0-9]+}},@r15
; CHECK: add	#-16,r15
; CHECK-DAG: mov.l	{{r[0-9]+}},@r15
; CHECK-DAG: mov.l	{{r[0-9]+}},@(4,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(8,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(12,r15)
; CHECK: bsr	frame_callee8
; CHECK-NEXT: nop
; CHECK-NEXT: add	#16,r15
; CHECK: mov.l	@r15,{{r[0-9]+}}
; CHECK: add	#4,r15
; CHECK-NEXT: lds.l	@r15+,pr
	%slot = alloca i32, align 4
	store volatile i32 %value, ptr %slot, align 4
	%called = call i32 @frame_callee8(i32 %value, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8)
	%saved = load volatile i32, ptr %slot, align 4
	%result = add i32 %called, %saved
	ret i32 %result
}

define i32 @value_live_across_stack_call(i32 %a0, i32 %a1) nounwind {
; CHECK-LABEL: value_live_across_stack_call:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK: add	#-{{[4-9]|[1-5][0-9]}},r15
; CHECK: mov.l	{{r[0-9]+}},
; CHECK: add	#-16,r15
; CHECK: bsr	frame_callee8
; CHECK-NEXT: nop
; CHECK: add	#16,r15
; CHECK: lds.l	@r15+,pr
	%saved = add i32 %a0, %a1
	%called = call i32 @frame_callee8(i32 %a0, i32 %a1, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8)
	%result = add i32 %called, %saved
	ret i32 %result
}

define i32 @spill_live_across_stack_call(ptr %base, i32 %value) nounwind {
; CHECK-LABEL: spill_live_across_stack_call:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK: add	#-{{[1-5][0-9]}},r15
; CHECK: add	#-16,r15
; CHECK: bsr	frame_callee8
; CHECK-NEXT: nop
; CHECK: add	#16,r15
; CHECK: lds.l	@r15+,pr
	%p0 = getelementptr i8, ptr %base, i32 0
	%p1 = getelementptr i8, ptr %base, i32 4
	%p2 = getelementptr i8, ptr %base, i32 8
	%p3 = getelementptr i8, ptr %base, i32 12
	%p4 = getelementptr i8, ptr %base, i32 16
	%p5 = getelementptr i8, ptr %base, i32 20
	%p6 = getelementptr i8, ptr %base, i32 24
	%p7 = getelementptr i8, ptr %base, i32 28
	%v0 = load volatile i32, ptr %p0, align 4
	%v1 = load volatile i32, ptr %p1, align 4
	%v2 = load volatile i32, ptr %p2, align 4
	%v3 = load volatile i32, ptr %p3, align 4
	%v4 = load volatile i32, ptr %p4, align 4
	%v5 = load volatile i32, ptr %p5, align 4
	%v6 = load volatile i32, ptr %p6, align 4
	%v7 = load volatile i32, ptr %p7, align 4
	%called = call i32 @frame_callee8(i32 %value, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8)
	%s0 = add i32 %called, %v0
	%s1 = add i32 %s0, %v1
	%s2 = add i32 %s1, %v2
	%s3 = add i32 %s2, %v3
	%s4 = add i32 %s3, %v4
	%s5 = add i32 %s4, %v5
	%s6 = add i32 %s5, %v6
	%result = add i32 %s6, %v7
	ret i32 %result
}
