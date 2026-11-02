; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define internal i32 @add_one(i32 %x) nounwind {
; CHECK-LABEL: add_one:
; CHECK-NOT: sts.l
; CHECK: rts
; CHECK-NEXT: add	#1,r0
	%result = add i32 %x, 1
	ret i32 %result
}

define internal i32 @forty_two() nounwind {
	ret i32 42
}

define internal void @consume(i32 %x) nounwind {
	ret void
}

define i32 @call_add_one(i32 %x) nounwind {
; CHECK-LABEL: call_add_one:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK-NEXT: mov.l	{{.*}},[[CALLEE:r[0-9]+]]
; CHECK-NEXT: jsr	@[[CALLEE]]
; CHECK-NEXT: nop
; CHECK-NEXT: lds.l	@r15+,pr
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%result = call i32 @add_one(i32 %x)
	ret i32 %result
}

define i32 @call_twice(i32 %x) nounwind {
; CHECK-LABEL: call_twice:
; CHECK: sts.l	pr,@-r15
; CHECK-NOT: sts.l
; CHECK: mov.l	{{.*}},{{r[0-9]+}}
; CHECK-COUNT-2: jsr	@{{r[0-9]+}}
; CHECK-NOT: bsr
; CHECK: lds.l	@r15+,pr
; CHECK-NEXT: rts
; CHECK-NEXT: {{(nop|mov\.l[[:space:]]+@r15\+,r8)}}
	%first = call i32 @add_one(i32 %x)
	%second = call i32 @add_one(i32 %first)
	ret i32 %second
}

define i32 @call_zero_arguments() nounwind {
; CHECK-LABEL: call_zero_arguments:
; CHECK: sts.l	pr,@-r15
; CHECK: jsr	@{{r[0-9]+}}
; CHECK-NEXT: nop
; CHECK: lds.l	@r15+,pr
	%result = call i32 @forty_two()
	ret i32 %result
}

define void @call_void(i32 %x) nounwind {
; CHECK-LABEL: call_void:
; CHECK: sts.l	pr,@-r15
; CHECK: jsr	@{{r[0-9]+}}
; CHECK-NEXT: nop
; CHECK-NOT: mov	r0
; CHECK: lds.l	@r15+,pr
	call void @consume(i32 %x)
	ret void
}

define i32 @call_in_control_flow(i32 %x, i32 %condition) nounwind {
; CHECK-LABEL: call_in_control_flow:
; CHECK: sts.l	pr,@-r15
; CHECK: tst
; CHECK: {{b[tf]}}
; CHECK: jsr	@{{r[0-9]+}}
; CHECK-NEXT: {{(nop|mov\.l[[:space:]]+@r15,r4)}}
; CHECK: lds.l	@r15+,pr
; CHECK: rts
; CHECK-NEXT: nop
entry:
	%is_zero = icmp eq i32 %condition, 0
	br i1 %is_zero, label %without_call, label %with_call
without_call:
	ret i32 %x
with_call:
	%result = call i32 @add_one(i32 %x)
	ret i32 %result
}
