; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s

; CHECK-LABEL: far_phi:
; CHECK: mov.w	[[BRANCH_LITERAL:.Lsh_relative_branch[0-9]+]],r0
; CHECK-NEXT: braf	r0
; CHECK: [[BRANCH_LITERAL]]:
; CHECK-NEXT: .short	.LBB{{[0-9]+}}_{{[0-9]+}}-[[BRANCH_LITERAL]]

define i32 @far_phi(i32 %condition, i32 %true_value, i32 %false_value) {
entry:
	%is_true = icmp ne i32 %condition, 0
	br i1 %is_true, label %far, label %large

large:
	call void asm sideeffect ".space 5000", ""()
	br label %merge

far:
	br label %merge

merge:
	%result = phi i32 [ %true_value, %far ], [ %false_value, %large ]
	ret i32 %result
}
