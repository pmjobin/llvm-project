; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define i32 @choose_equal_optimized(i32 %a, i32 %b) {
; CHECK-LABEL: choose_equal_optimized:
; CHECK: cmp/eq	r5,r4
; CHECK-NEXT: bf	[[DIFFERENT:.LBB[0-9_]+]]
; CHECK: [[DIFFERENT]]:
	%equal = icmp eq i32 %a, %b
	br i1 %equal, label %same, label %different
same:
	ret i32 %a
different:
	ret i32 %b
}

define i32 @max_signed_optimized(i32 %a, i32 %b) {
; CHECK-LABEL: max_signed_optimized:
; CHECK: cmp/gt	r5,{{r[04]}}
; CHECK-NEXT: bt	[[MERGE:.LBB[0-9_]+]]
; CHECK-NEXT: mov	r5,r0
; CHECK: [[MERGE]]:
	%greater = icmp sgt i32 %a, %b
	br i1 %greater, label %take_a, label %take_b
take_a:
	br label %merge
take_b:
	br label %merge
merge:
	%result = phi i32 [ %a, %take_a ], [ %b, %take_b ]
	ret i32 %result
}

define i32 @count_down_optimized(i32 %n) {
; CHECK-LABEL: count_down_optimized:
; CHECK: [[LOOP:.LBB[0-9_]+]]:
; CHECK: add	#-1,{{r[0-9]+}}
; CHECK: tst
; CHECK: bf	[[LOOP]]
entry:
	br label %loop
loop:
	%value = phi i32 [ %n, %entry ], [ %next, %loop ]
	%next = add i32 %value, -1
	%continue = icmp ne i32 %next, 0
	br i1 %continue, label %loop, label %exit
exit:
	ret i32 %next
}
