; RUN: opt -mtriple=shle-unknown-none-elf -passes='default<O0>' -S %s -o - | FileCheck %s --check-prefix=NO-SINK
; RUN: sed 's/ optsize//' %s | opt -mtriple=shle-unknown-none-elf -passes='default<O2>' -S -o - | FileCheck %s --check-prefix=NO-SINK
; RUN: opt -mtriple=shle-unknown-none-elf -passes='default<O2>' -S %s -o - | FileCheck %s --check-prefix=NO-SINK

target triple = "shle-unknown-none-elf"

; The specialized size pipeline is omitted. The ordinary optimizer keeps this
; loop-invariant multiplication outside the loop at both optimization levels.
;
; NO-SINK-LABEL: define void @single_use_block(
; NO-SINK: entry:
; NO-SINK-NEXT: %square = mul i32 %value, %value
;
; SINK-LABEL: define void @single_use_block(
; SINK: entry:
; SINK-NOT: mul i32
; SINK: body:
; SINK-NEXT: %square = mul i32 %value, %value

define void @single_use_block(ptr %output, i32 %length, i32 %value) noinline optsize {
entry:
	%square = mul i32 %value, %value
	br label %loop

loop:
	%index = phi i32 [ 0, %entry ], [ %next, %body ]
	%done = icmp eq i32 %index, %length
	br i1 %done, label %exit, label %body

body:
	%element = getelementptr i32, ptr %output, i32 %index
	store volatile i32 %square, ptr %element, align 4
	%next = add i32 %index, 1
	br label %loop

exit:
	ret void
}
