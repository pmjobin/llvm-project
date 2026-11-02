; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llvm-as %s -o %t.bc
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s

; CHECK-LABEL: repeat_2500:
; CHECK: bf [[NEAR:.LBB[0-9]+_[0-9]+]]
; CHECK: mov.w [[LITERAL:.Lsh_relative_branch[0-9]+]],r0
; CHECK-NEXT: braf r0
; CHECK-NEXT: nop
; CHECK: [[LITERAL]]:
; CHECK-NEXT: .short [[FAR:.LBB[0-9]+_[0-9]+]]-[[LITERAL]]
; CHECK: [[NEAR]]:
; CHECK: !APP
; CHECK-COUNT-2500: nop
; CHECK: !NO_APP
; CHECK: [[FAR]]:

define i32 @repeat_2500(i32 %condition) {
entry:
	%iszero = icmp eq i32 %condition, 0
	br i1 %iszero, label %return, label %large

large:
	call void asm sideeffect ".rept 2500\0A\09nop\0A\09.endr", ""()
	br label %return

return:
	%result = phi i32 [ 19, %entry ], [ 17, %large ]
	ret i32 %result
}

; CHECK-LABEL: nested_repeat_backward:
; CHECK: !APP
; CHECK-COUNT-2500: nop
; CHECK: !NO_APP
; CHECK: mov.w [[BACK_LITERAL:.Lsh_relative_branch[0-9]+]],r1
; CHECK: braf r1
; CHECK: [[BACK_LITERAL]]:
; CHECK-NEXT: .short .LBB{{[0-9]+}}_{{[0-9]+}}-[[BACK_LITERAL]]

define i32 @nested_repeat_backward(i32 %count) {
entry:
	br label %loop

loop:
	%value = phi i32 [ %count, %entry ], [ %next, %loop ]
	call void asm sideeffect ".rept 5; .rept 500; nop; .endr; .endr", ""()
	%next = add i32 %value, -1
	%again = icmp ne i32 %next, 0
	br i1 %again, label %loop, label %exit

exit:
	ret i32 %next
}

; CHECK-LABEL: unrelated_previous_repeat:
; CHECK: !APP
; CHECK-COUNT-2500: nop
; CHECK: !NO_APP
; CHECK-NOT: .Lsh_{{(long|relative)}}_branch
; CHECK: {{bt|bf}} .LBB{{[0-9]+}}_{{[0-9]+}}
; CHECK: .Lfunc_end

define i32 @unrelated_previous_repeat(i32 %condition) {
entry:
	call void asm sideeffect ".rept 2500\0A\09nop\0A\09.endr", ""()
	%iszero = icmp eq i32 %condition, 0
	br i1 %iszero, label %zero, label %one

zero:
	ret i32 0

one:
	ret i32 1
}
