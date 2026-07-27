; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define internal void @boundary_leaf() noinline {
	ret void
}

define i32 @nonleaf_nineteenth_at_sixty(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6, i32 %a7, i32 %a8, i32 %a9, i32 %a10, i32 %a11, i32 %a12, i32 %a13, i32 %a14, i32 %a15, i32 %a16, i32 %a17, i32 %a18) {
; CHECK-LABEL: nonleaf_nineteenth_at_sixty:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK-NOT: add	#-
; CHECK: {{b[tf]}}
; CHECK: mov.l	@(60,r15),r0
; CHECK: bsr	boundary_leaf
; CHECK-NEXT: nop
; CHECK: lds.l	@r15+,pr
; CHECK: rts
; CHECK-NEXT: nop
entry:
	%take_call = icmp eq i32 %a0, 0
	br i1 %take_call, label %with_call, label %without_call

with_call:
	call void @boundary_leaf()
	ret i32 0

without_call:
	ret i32 %a18
}
