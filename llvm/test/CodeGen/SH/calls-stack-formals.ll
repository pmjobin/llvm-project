; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefixes=CHECK,O2
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefixes=CHECK,O2

define internal void @formals_leaf() noinline nounwind {
	ret void
}

define i32 @return_fifth(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4) nounwind {
; CHECK-LABEL: return_fifth:
; CHECK-NEXT: mov.l	@r15,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	ret i32 %a4
}

define ptr @return_sixth_pointer(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, ptr %a5) nounwind {
; CHECK-LABEL: return_sixth_pointer:
; CHECK-NEXT: mov.l	@(4,r15),r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	ret ptr %a5
}

define i32 @return_nineteenth(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6, i32 %a7, i32 %a8, i32 %a9, i32 %a10, i32 %a11, i32 %a12, i32 %a13, i32 %a14, i32 %a15, i32 %a16, i32 %a17, i32 %a18) nounwind {
; CHECK-LABEL: return_nineteenth:
; CHECK-NEXT: mov.l	@(56,r15),r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	ret i32 %a18
}

define i32 @nonleaf_only_pr_fifth(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4) nounwind {
; CHECK-LABEL: nonleaf_only_pr_fifth:
; CHECK-NEXT: sts.l	pr,@-r15
; O2-NOT: add	#-4,r15
; O2: {{b[tf]}}
; O2: mov.l	@(4,r15),[[FIFTH:r[0-9]+]]
; CHECK: bsr	formals_leaf
; CHECK-NEXT: nop
; CHECK: lds.l	@r15+,pr
; CHECK: rts
; CHECK-NEXT: nop
entry:
	%take_call = icmp eq i32 %a0, 0
	br i1 %take_call, label %with_call, label %without_call

with_call:
	call void @formals_leaf()
	ret i32 0

without_call:
	ret i32 %a4
}

define i32 @fixed_frame_twelve_fifth(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4) nounwind {
; CHECK-LABEL: fixed_frame_twelve_fifth:
; CHECK-NEXT: sts.l	pr,@-r15
; O2-NEXT: add	#-8,r15
; O2: mov.l	@(12,r15),[[FRAME_FIFTH:r[0-9]+]]
; CHECK: bsr	formals_leaf
; CHECK-NEXT: nop
; O2: add	#8,r15
; CHECK: lds.l	@r15+,pr
	%slot = alloca i32, align 4
	store volatile i32 %a1, ptr %slot, align 4
	call void @formals_leaf()
	%loaded = load volatile i32, ptr %slot, align 4
	%result = add i32 %loaded, %a4
	ret i32 %result
}
