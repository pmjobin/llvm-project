; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define internal i32 @add_one(i32 %x) nounwind {
	%result = add i32 %x, 1
	ret i32 %result
}

define i32 @live_across(i32 %x, i32 %y) nounwind {
; CHECK-LABEL: live_across:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK-NEXT: add	#-4,r15
; CHECK: bsr	add_one
; CHECK-NEXT: nop
; CHECK: add	#4,r15
; CHECK-NEXT: lds.l	@r15+,pr
	%saved = add i32 %x, %y
	%called = call i32 @add_one(i32 %x)
	%result = add i32 %called, %saved
	ret i32 %result
}

define i32 @call_with_frame(i32 %x) nounwind {
; CHECK-LABEL: call_with_frame:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK-NEXT: add	#-4,r15
; CHECK: mov.l	r4,@r15
; CHECK: bsr	add_one
; CHECK-NEXT: nop
; CHECK: mov.l	@r15,{{r[0-9]+}}
; CHECK: add	#4,r15
; CHECK-NEXT: lds.l	@r15+,pr
	%slot = alloca i32, align 4
	store volatile i32 %x, ptr %slot, align 4
	%called = call i32 @add_one(i32 %x)
	%saved = load volatile i32, ptr %slot, align 4
	%result = add i32 %called, %saved
	ret i32 %result
}

define i32 @call_with_60_byte_frame(i32 %value) nounwind {
; CHECK-LABEL: call_with_60_byte_frame:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK-NEXT: add	#-56,r15
; CHECK-NEXT: mov.l	r4,@(52,r15)
; CHECK: mov.l	r4,@r15
; CHECK: bsr	add_one
; CHECK-NEXT: nop
; CHECK: add	#56,r15
; CHECK-NEXT: lds.l	@r15+,pr
	%s0 = alloca i32, align 4
	%s1 = alloca i32, align 4
	%s2 = alloca i32, align 4
	%s3 = alloca i32, align 4
	%s4 = alloca i32, align 4
	%s5 = alloca i32, align 4
	%s6 = alloca i32, align 4
	%s7 = alloca i32, align 4
	%s8 = alloca i32, align 4
	%s9 = alloca i32, align 4
	%s10 = alloca i32, align 4
	%s11 = alloca i32, align 4
	%s12 = alloca i32, align 4
	%s13 = alloca i32, align 4
	store volatile i32 %value, ptr %s0, align 4
	store volatile i32 %value, ptr %s1, align 4
	store volatile i32 %value, ptr %s2, align 4
	store volatile i32 %value, ptr %s3, align 4
	store volatile i32 %value, ptr %s4, align 4
	store volatile i32 %value, ptr %s5, align 4
	store volatile i32 %value, ptr %s6, align 4
	store volatile i32 %value, ptr %s7, align 4
	store volatile i32 %value, ptr %s8, align 4
	store volatile i32 %value, ptr %s9, align 4
	store volatile i32 %value, ptr %s10, align 4
	store volatile i32 %value, ptr %s11, align 4
	store volatile i32 %value, ptr %s12, align 4
	store volatile i32 %value, ptr %s13, align 4
	%result = call i32 @add_one(i32 %value)
	ret i32 %result
}

define i32 @call_with_spill_pressure(ptr %base, i32 %value) nounwind {
; CHECK-LABEL: call_with_spill_pressure:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK-NEXT: add	#-{{[1-5][0-9]}},r15
; CHECK: mov.l	r8,
; CHECK: bsr	add_one
; CHECK-NEXT: nop
; CHECK: mov.l	{{.*}},r8
; CHECK: lds.l	@r15+,pr
	%p0 = getelementptr i8, ptr %base, i32 0
	%p1 = getelementptr i8, ptr %base, i32 4
	%p2 = getelementptr i8, ptr %base, i32 8
	%p3 = getelementptr i8, ptr %base, i32 12
	%p4 = getelementptr i8, ptr %base, i32 16
	%p5 = getelementptr i8, ptr %base, i32 20
	%p6 = getelementptr i8, ptr %base, i32 24
	%p7 = getelementptr i8, ptr %base, i32 28
	%p8 = getelementptr i8, ptr %base, i32 32
	%p9 = getelementptr i8, ptr %base, i32 36
	%p10 = getelementptr i8, ptr %base, i32 40
	%p11 = getelementptr i8, ptr %base, i32 44
	%v0 = load volatile i32, ptr %p0, align 4
	%v1 = load volatile i32, ptr %p1, align 4
	%v2 = load volatile i32, ptr %p2, align 4
	%v3 = load volatile i32, ptr %p3, align 4
	%v4 = load volatile i32, ptr %p4, align 4
	%v5 = load volatile i32, ptr %p5, align 4
	%v6 = load volatile i32, ptr %p6, align 4
	%v7 = load volatile i32, ptr %p7, align 4
	%v8 = load volatile i32, ptr %p8, align 4
	%v9 = load volatile i32, ptr %p9, align 4
	%v10 = load volatile i32, ptr %p10, align 4
	%v11 = load volatile i32, ptr %p11, align 4
	%called = call i32 @add_one(i32 %value)
	%s0 = add i32 %called, %v0
	%s1 = add i32 %s0, %v1
	%s2 = add i32 %s1, %v2
	%s3 = add i32 %s2, %v3
	%s4 = add i32 %s3, %v4
	%s5 = add i32 %s4, %v5
	%s6 = add i32 %s5, %v6
	%s7 = add i32 %s6, %v7
	%s8 = add i32 %s7, %v8
	%s9 = add i32 %s8, %v9
	%s10 = add i32 %s9, %v10
	%result = add i32 %s10, %v11
	ret i32 %result
}

define internal i32 @recursive(i32 %n) noinline nounwind {
; CHECK-LABEL: recursive:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK-NOT: sts.l
; CHECK: {{b[tf]}}
; CHECK-DAG: bsr	recursive
; CHECK-DAG: lds.l	@r15+,pr
; CHECK-DAG: lds.l	@r15+,pr
entry:
	%done = icmp eq i32 %n, 0
	br i1 %done, label %base, label %recurse
base:
	ret i32 0
recurse:
	%next = add i32 %n, -1
	%value = call i32 @recursive(i32 %next)
	%result = add i32 %value, 1
	ret i32 %result
}

define i32 @call_in_loop(i32 %n) nounwind {
; CHECK-LABEL: call_in_loop:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK-NOT: sts.l
; CHECK: bsr	add_one
; CHECK-NEXT: nop
; CHECK: {{b[tf]}}
; CHECK: lds.l	@r15+,pr
entry:
	br label %loop
loop:
	%value = phi i32 [ %n, %entry ], [ %next, %loop ]
	%next = call i32 @add_one(i32 %value)
	%continue = icmp ne i32 %next, 3
	br i1 %continue, label %loop, label %exit
exit:
	ret i32 %next
}
