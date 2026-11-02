; RUN: opt -mtriple=shle-unknown-none-elf -mcpu=sh2 -passes=loop-reduce -S %s | FileCheck %s
; RUN: opt -mtriple=shle-unknown-none-elf -mcpu=sh2 -passes=loop-reduce %s -o - | llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -verify-machineinstrs -o /dev/null

; LSR can replace two same-stride narrow pointer IVs with one integer IV. That
; requires SH to rebuild both addresses around the shared offset. The current
; generic LSR chooses that form; keep the exact eight narrow accesses and stride
; recurrence as a witness for a future measured cost-model improvement.

target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define void @narrow_byte_chains(ptr %dest, ptr %ref, i32 %stride, i32 %height) #0 {
entry:
	br label %loop

loop:
	%ref.iv = phi ptr [ %ref, %entry ], [ %ref.next, %loop ]
	%dest.iv = phi ptr [ %dest, %entry ], [ %dest.next, %loop ]
	%height.iv = phi i32 [ %height, %entry ], [ %height.next, %loop ]
	%value0 = load i8, ptr %ref.iv, align 1
	store i8 %value0, ptr %dest.iv, align 1
	%ref1 = getelementptr i8, ptr %ref.iv, i32 1
	%value1 = load i8, ptr %ref1, align 1
	%dest1 = getelementptr i8, ptr %dest.iv, i32 1
	store i8 %value1, ptr %dest1, align 1
	%ref2 = getelementptr i8, ptr %ref.iv, i32 2
	%value2 = load i8, ptr %ref2, align 1
	%dest2 = getelementptr i8, ptr %dest.iv, i32 2
	store i8 %value2, ptr %dest2, align 1
	%ref3 = getelementptr i8, ptr %ref.iv, i32 3
	%value3 = load i8, ptr %ref3, align 1
	%dest3 = getelementptr i8, ptr %dest.iv, i32 3
	store i8 %value3, ptr %dest3, align 1
	%ref4 = getelementptr i8, ptr %ref.iv, i32 4
	%value4 = load i8, ptr %ref4, align 1
	%dest4 = getelementptr i8, ptr %dest.iv, i32 4
	store i8 %value4, ptr %dest4, align 1
	%ref5 = getelementptr i8, ptr %ref.iv, i32 5
	%value5 = load i8, ptr %ref5, align 1
	%dest5 = getelementptr i8, ptr %dest.iv, i32 5
	store i8 %value5, ptr %dest5, align 1
	%ref6 = getelementptr i8, ptr %ref.iv, i32 6
	%value6 = load i8, ptr %ref6, align 1
	%dest6 = getelementptr i8, ptr %dest.iv, i32 6
	store i8 %value6, ptr %dest6, align 1
	%ref7 = getelementptr i8, ptr %ref.iv, i32 7
	%value7 = load i8, ptr %ref7, align 1
	%dest7 = getelementptr i8, ptr %dest.iv, i32 7
	store i8 %value7, ptr %dest7, align 1
	%ref.next = getelementptr i8, ptr %ref.iv, i32 %stride
	%dest.next = getelementptr i8, ptr %dest.iv, i32 %stride
	%height.next = add nsw i32 %height.iv, -1
	%done = icmp eq i32 %height.next, 0
	br i1 %done, label %exit, label %loop

exit:
	ret void
}

; CHECK-LABEL: define void @narrow_byte_chains(
; CHECK: entry:
; CHECK: loop:
; CHECK: [[IV:%[^ ]+]] = phi i32
; CHECK: [[REF:%[^ ]+]] = getelementptr i8, ptr %ref, i32 [[IV]]
; CHECK: [[DEST:%[^ ]+]] = getelementptr i8, ptr %dest, i32 [[IV]]
; CHECK: [[V0:%[^ ]+]] = load i8, ptr [[REF]]
; CHECK: store i8 [[V0]], ptr [[DEST]]
; CHECK: getelementptr i8, ptr [[REF]], i32 7
; CHECK: [[V7:%[^ ]+]] = load i8,
; CHECK: getelementptr i8, ptr [[DEST]], i32 7
; CHECK: store i8 [[V7]],
; CHECK: add i32 [[IV]], %stride
; CHECK: br i1 %done, label %exit, label %loop

attributes #0 = { optsize }
