; RUN: opt -passes=loop-vectorize -force-vector-width=4 -force-vector-interleave=2 -mtriple=shle-unknown-none-elf -mcpu=sh4 -S %s | FileCheck %s --check-prefix=LV
; RUN: opt -passes=loop-vectorize -force-vector-width=4 -force-vector-interleave=2 -mtriple=shle-unknown-none-elf -mcpu=sh4 %s -o - | llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -o /dev/null

; LoopVectorize remains independently enabled and its scalarizable vector IR
; remains compilable.
define void @store_zero(ptr nocapture %pointer, i32 %length) {
; LV-LABEL: define void @store_zero(
; LV: vector.body:
entry:
	br label %loop

loop:
	%pointer.current = phi ptr [ %pointer, %entry ], [ %pointer.next, %loop ]
	%length.current = phi i32 [ %length, %entry ], [ %length.next, %loop ]
	%pointer.next = getelementptr inbounds i8, ptr %pointer.current, i32 1
	store i8 0, ptr %pointer.current, align 1
	%length.next = add nsw i32 %length.current, -1
	%done = icmp eq i32 %length.current, 0
	br i1 %done, label %exit, label %loop

exit:
	ret void
}
