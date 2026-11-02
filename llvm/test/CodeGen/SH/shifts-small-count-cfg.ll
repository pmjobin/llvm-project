; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=SELECT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=CFG
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -enable-new-pm=1 -verify-machineinstrs -verify-machine-dom-info -verify-loop-info < %s > /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O3 -enable-new-pm=1 -verify-machineinstrs -verify-machine-dom-info -verify-loop-info < %s > /dev/null

define i32 @weighted_successors(i32 %x, i32 %raw, i32 %flag) {
; SELECT-LABEL: name: weighted_successors
; SELECT: SHL_SMALL_PSEUDO {{.*}}, 0, 24
; CFG-LABEL: name: weighted_successors
; CFG: SHLL16
; CFG: PHI
; CFG: SHLL8
; CFG: successors: {{.*}}(0x73333333), {{.*}}(0x0ccccccd)
; CFG: PHI
	%c = and i32 %raw, 24
	%v = shl i32 %x, %c
	%cond = icmp eq i32 %flag, 0
	br i1 %cond, label %yes, label %no, !prof !0
yes:
	ret i32 %v
no:
	%z = xor i32 %v, 73
	ret i32 %z
}

define i32 @successor_phi(i32 %x, i32 %raw, i32 %flag) {
; SELECT-LABEL: name: successor_phi
; SELECT: SRL_SMALL_PSEUDO {{.*}}, 0, 24
; CFG-LABEL: name: successor_phi
; CFG: SHLR16
; CFG: PHI
; CFG: SHLR8
; CFG: PHI
	%cond = icmp eq i32 %flag, 0
	br i1 %cond, label %left, label %right
left:
	%c = and i32 %raw, 24
	%v = lshr i32 %x, %c
	br label %join
right:
	%other = xor i32 %x, -1
	br label %join
join:
	%result = phi i32 [ %v, %left ], [ %other, %right ]
	ret i32 %result
}

define i32 @loop_phi(i32 %x, i32 %raw, i32 %n) {
; SELECT-LABEL: name: loop_phi
; SELECT: SHL_SMALL_PSEUDO {{.*}}, 0, 24
; CFG-LABEL: name: loop_phi
; CFG: PHI
; CFG: SHLL16
; CFG: PHI
; CFG: SHLL8
; CFG: PHI
entry:
	br label %loop
loop:
	%value = phi i32 [ %x, %entry ], [ %next, %body ]
	%i = phi i32 [ 0, %entry ], [ %inc, %body ]
	%more = icmp ult i32 %i, %n
	br i1 %more, label %body, label %exit
body:
	%bits = xor i32 %raw, %i
	%c = and i32 %bits, 24
	%shifted = shl i32 %value, %c
	%next = add i32 %shifted, 19
	%inc = add i32 %i, 1
	br label %loop
exit:
	ret i32 %value
}

!0 = !{!"branch_weights", i32 9, i32 1}
