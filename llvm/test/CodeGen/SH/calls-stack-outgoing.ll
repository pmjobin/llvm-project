; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define internal i32 @outgoing4(i32 %a0, i32 %a1, i32 %a2, i32 %a3) noinline {
	%sum0 = add i32 %a0, %a1
	%sum1 = add i32 %a2, %a3
	%result = add i32 %sum0, %sum1
	ret i32 %result
}

define internal i32 @outgoing5(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4) noinline {
	ret i32 %a4
}

define internal i32 @outgoing8(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6, i32 %a7) noinline {
	%sum0 = add i32 %a0, %a1
	%sum1 = add i32 %a2, %a3
	%sum2 = add i32 %a4, %a5
	%sum3 = add i32 %a6, %a7
	%left = add i32 %sum0, %sum1
	%right = add i32 %sum2, %sum3
	%result = add i32 %left, %right
	ret i32 %result
}

define internal ptr @outgoing5_ptr(i32 %a0, i32 %a1, i32 %a2, i32 %a3, ptr %a4) noinline {
	ret ptr %a4
}

define i32 @call_return_fifth(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4) {
; CHECK-LABEL: call_return_fifth:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK: mov.l	@(4,r15),[[FIFTH:r[0-9]+]]
; CHECK: add	#-4,r15
; CHECK-NEXT: mov.l	[[FIFTH]],@r15
; CHECK: bsr	outgoing5
; CHECK-NEXT: nop
; CHECK-NEXT: add	#4,r15
; CHECK-NEXT: lds.l	@r15+,pr
	%result = call i32 @outgoing5(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4)
	ret i32 %result
}

define i32 @call_eight_mixed(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6, i32 %a7) {
; CHECK-LABEL: call_eight_mixed:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK: add	#-16,r15
; CHECK-DAG: mov.l	{{r[0-9]+}},@r15
; CHECK-DAG: mov.l	{{r[0-9]+}},@(4,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(8,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(12,r15)
; CHECK: mov	#-7,r5
; CHECK: bsr	outgoing8
; CHECK-NEXT: nop
; CHECK-NEXT: add	#16,r15
; CHECK-NEXT: lds.l	@r15+,pr
	%result = call i32 @outgoing8(i32 %a7, i32 -7, i32 %a2, i32 %a0, i32 %a4, i32 12, i32 %a0, i32 %a7)
	ret i32 %result
}

define i32 @stack_value_before_register_overwrite(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4) {
; CHECK-LABEL: stack_value_before_register_overwrite:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK: add	#-4,r15
; CHECK: mov.l	{{r[0-9]+}},@r15
; CHECK: bsr	outgoing5
; CHECK-NEXT: nop
; CHECK: add	#4,r15
	%result = call i32 @outgoing5(i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a0)
	ret i32 %result
}

define i32 @register_cycle_with_stack(i32 %a0, i32 %a1, i32 %a2, i32 %a3) {
; CHECK-LABEL: register_cycle_with_stack:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK: add	#-16,r15
; CHECK-DAG: mov.l	{{r[0-9]+}},@r15
; CHECK-DAG: mov.l	{{r[0-9]+}},@(4,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(8,r15)
; CHECK-DAG: mov.l	{{r[0-9]+}},@(12,r15)
; CHECK: bsr	outgoing8
; CHECK-NEXT: nop
; CHECK-NEXT: add	#16,r15
	%result = call i32 @outgoing8(i32 %a1, i32 %a2, i32 %a3, i32 %a0, i32 %a0, i32 %a1, i32 %a2, i32 %a3)
	ret i32 %result
}

define i32 @multiple_call_frame_sizes(i32 %value) {
; CHECK-LABEL: multiple_call_frame_sizes:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK-NOT: add	#-,r15
; CHECK: bsr	outgoing4
; CHECK-NEXT: nop
; CHECK: add	#-4,r15
; CHECK: bsr	outgoing5
; CHECK-NEXT: nop
; CHECK: add	#4,r15
; CHECK: add	#-16,r15
; CHECK: bsr	outgoing8
; CHECK-NEXT: nop
; CHECK-NEXT: add	#16,r15
; CHECK-NOT: bsr
; CHECK: lds.l	@r15+,pr
	%four = call i32 @outgoing4(i32 %value, i32 2, i32 3, i32 4)
	%five = call i32 @outgoing5(i32 %four, i32 2, i32 3, i32 4, i32 %value)
	%eight = call i32 @outgoing8(i32 %five, i32 2, i32 3, i32 4, i32 %value, i32 6, i32 7, i32 8)
	ret i32 %eight
}

define ptr @pointer_stack_argument(ptr %value) {
; CHECK-LABEL: pointer_stack_argument:
; CHECK: add	#-4,r15
; CHECK: mov.l	{{r[0-9]+}},@r15
; CHECK: bsr	outgoing5_ptr
; CHECK-NEXT: nop
; CHECK-NEXT: add	#4,r15
	%result = call ptr @outgoing5_ptr(i32 1, i32 2, i32 3, i32 4, ptr %value)
	ret ptr %result
}
