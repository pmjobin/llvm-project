; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s

declare void @callee()
declare void @escape(ptr)
declare ptr @llvm.frameaddress.p0(i32 immarg)

define void @leaf_gpr_saves() nounwind {
entry:
	call void asm sideeffect "", "~{r8},~{r9}"()
	ret void
}

; CHECK-LABEL: leaf_gpr_saves:
; CHECK-NOT: add {{.*}}r15
; CHECK: mov.l r8,@-r15
; CHECK-NEXT: mov.l r9,@-r15
; CHECK: mov.l @r15+,r9
; CHECK-NOT: add {{.*}}r15
; CHECK-NEXT: rts
; CHECK-NEXT: mov.l @r15+,r8

define ptr @frame_pointer_gpr_saves() nounwind {
entry:
	call void asm sideeffect "", "~{r8},~{r9}"()
	%frame = call ptr @llvm.frameaddress.p0(i32 0)
	ret ptr %frame
}

; CHECK-LABEL: frame_pointer_gpr_saves:
; CHECK: mov.l r8,@-r15
; CHECK-NEXT: mov.l r9,@-r15
; CHECK-NEXT: mov.l r14,@-r15
; CHECK-NEXT: mov r15,r14
; CHECK: mov.l @r15+,r14
; CHECK: mov.l @r15+,r9
; CHECK-NEXT: rts
; CHECK-NEXT: mov.l @r15+,r8

define ptr @local_frame_pointer_gpr_saves() nounwind {
entry:
	%local = alloca i32, align 4
	call void asm sideeffect "", "~{r8},~{r9}"()
	call void @escape(ptr %local)
	%frame = call ptr @llvm.frameaddress.p0(i32 0)
	ret ptr %frame
}

; CHECK-LABEL: local_frame_pointer_gpr_saves:
; CHECK: sts.l pr,@-r15
; CHECK-NEXT: mov.l r8,@-r15
; CHECK-NEXT: mov.l r9,@-r15
; CHECK-NEXT: mov.l r14,@-r15
; CHECK-NEXT: add #-4,r15
; CHECK: mov r15,r14
; CHECK: mov r14,r15
; CHECK-NEXT: add #4,r15
; CHECK-NEXT: mov.l @r15+,r14
; CHECK-NEXT: mov.l @r15+,r9
; CHECK-NEXT: mov.l @r15+,r8
; CHECK-NEXT: lds.l @r15+,pr
; CHECK-NEXT: rts

define void @call_gpr_saves() nounwind {
entry:
	call void asm sideeffect "", "~{r8},~{r9}"()
	call void @callee()
	ret void
}

; CHECK-LABEL: call_gpr_saves:
; CHECK: sts.l pr,@-r15
; CHECK-NEXT: mov.l r8,@-r15
; CHECK-NEXT: mov.l r9,@-r15
; CHECK-NOT: add {{.*}}r15
; CHECK: mov.l @r15+,r9
; CHECK-NOT: add {{.*}}r15
; CHECK-NEXT: mov.l @r15+,r8
; CHECK-NEXT: lds.l @r15+,pr
; CHECK-NEXT: rts

define void @unwind_gpr_saves() uwtable {
entry:
	call void asm sideeffect "", "~{r8},~{r9}"()
	ret void
}

; CHECK-LABEL: unwind_gpr_saves:
; CHECK: add #-8,r15
; CHECK: mov.l r8,@(4,r15)
; CHECK: mov.l r9,@r15
; CHECK: mov.l @r15,r9
; CHECK: mov.l @(4,r15),r8
; CHECK: add #8,r15

define void @local_and_gpr_saves() nounwind {
entry:
	%local = alloca i32, align 4
	call void asm sideeffect "", "~{r8},~{r9}"()
	call void @escape(ptr %local)
	ret void
}

; CHECK-LABEL: local_and_gpr_saves:
; CHECK: sts.l pr,@-r15
; CHECK-NEXT: mov.l r8,@-r15
; CHECK-NEXT: mov.l r9,@-r15
; CHECK-NEXT: add #-4,r15
; CHECK: add #4,r15
; CHECK-NEXT: mov.l @r15+,r9
; CHECK-NEXT: mov.l @r15+,r8
; CHECK-NEXT: lds.l @r15+,pr
; CHECK-NEXT: rts
