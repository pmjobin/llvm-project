; RUN: opt -mtriple=shle-unknown-none-elf -mcpu=sh3 -passes='default<O3>' -S %s | llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -o - | FileCheck %s --check-prefix=SPEED
; RUN: sed 's/) { ; SPEED/) #0 {/' %s | opt -mtriple=shle-unknown-none-elf -mcpu=sh3 -passes='default<O3>' -S | llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -o - | FileCheck %s --check-prefix=SIZE

; Preserve this adverse pressure witness for the generic machine scheduler.
; Its local frame is 228 bytes, 12 more than the former top-down schedule.
; Size mode retains its separate schedule. This is a code-generation fixture;
; byte-offset rows asserting i32 alignment are not a runtime oracle.

target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i64 @schedule_pressure(ptr %data, ptr %aux, i32 %k, i32 %index, ptr %values) { ; SPEED
entry:
	%output = alloca [16 x i32], align 4
	br label %row.cond

row.cond:
	%row = phi i32 [ 0, %entry ], [ %row.next, %row.inc ]
	%row.done = icmp eq i32 %row, 4
	br i1 %row.done, label %checksum.cond, label %column.cond

column.cond:
	%column = phi i32 [ %column.next, %column.end ], [ 0, %row.cond ]
	%column.done = icmp eq i32 %column, 4
	br i1 %column.done, label %row.inc, label %multiply.cond

multiply.cond:
	%multiply.index = phi i32 [ %multiply.next, %multiply.body ], [ 0, %column.cond ]
	%sum = phi i32 [ %sum.next, %multiply.body ], [ 0, %column.cond ]
	%multiply.done = icmp eq i32 %multiply.index, 4
	br i1 %multiply.done, label %column.end, label %multiply.body

multiply.body:
	%row.address = getelementptr i8, ptr %data, i32 %row
	%left.address = getelementptr [4 x i8], ptr %row.address, i32 %multiply.index
	%left = load i32, ptr %left.address, align 4
	%left.bit = trunc i32 %left to i1
	%right.address = getelementptr [4 x i8], ptr %values, i32 %column
	%right = load i32, ptr %right.address, align 4
	%product = select i1 %left.bit, i32 %right, i32 0
	%sum.next = add i32 %sum, %product
	%multiply.next = add i32 %multiply.index, 1
	br label %multiply.cond

column.end:
	%row.offset = shl i32 %row, 4
	%output.row = getelementptr i8, ptr %output, i32 %row.offset
	%output.address = getelementptr [4 x i8], ptr %output.row, i32 %column
	store i32 %sum, ptr %output.address, align 4
	%column.next = add i32 %column, 1
	br label %column.cond

row.inc:
	%row.next = add i32 %row, 1
	br label %row.cond

checksum.cond:
	%checksum = phi i32 [ %checksum.next, %checksum.body ], [ 0, %row.cond ]
	%checksum.index = phi i32 [ %checksum.index.next, %checksum.body ], [ 0, %row.cond ]
	%checksum.done = icmp eq i32 %checksum.index, 16
	br i1 %checksum.done, label %exit, label %checksum.body

checksum.body:
	%checksum.address = getelementptr [4 x i8], ptr %output, i32 %checksum.index
	%element = load i32, ptr %checksum.address, align 4
	%checksum.next = xor i32 %checksum, %element
	%checksum.index.next = add i32 %checksum.index, 1
	br label %checksum.cond

exit:
	%result = zext i32 %checksum to i64
	ret i64 %result
}

; SPEED-LABEL: schedule_pressure:
; SPEED: add #-128,r15
; SPEED-NEXT: add #-100,r15

; SIZE-LABEL: schedule_pressure:
; SIZE: add #-68,r15

attributes #0 = { optsize }
