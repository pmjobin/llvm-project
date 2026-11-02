; RUN: opt -mtriple=shle-unknown-none-elf -mcpu=sh3 -passes='default<O3>' -S %s | llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -o - | FileCheck %s

; SH can access the i32 field with a scaled displacement. Report its narrow
; addressing modes accurately when optimizing for speed so LSR does not bias
; the induction variable to the i32 field and make the i16 field expensive.

target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i64 @speed(ptr %data, i32 %n) {
entry:
	br label %loop

loop:
	%value = phi i32 [ 0, %entry ], [ %next, %body ]
	%i = phi i32 [ 0, %entry ], [ %inc, %body ]
	%done = icmp eq i32 %i, %n
	br i1 %done, label %exit, label %body

body:
	%record = getelementptr [8 x i8], ptr %data, i32 %i
	%value.addr = getelementptr i8, ptr %record, i32 4
	%field = load i32, ptr %value.addr, align 4
	%sum = or i32 %value, %field
	%tag = load i16, ptr %record, align 4
	%tag.ext = zext i16 %tag to i32
	%next = xor i32 %sum, %tag.ext
	%inc = add i32 %i, 1
	br label %loop

exit:
	%result = zext i32 %value to i64
	ret i64 %result
}

; CHECK-LABEL: speed:
; CHECK-NOT: add #4,r4
; CHECK-DAG: mov.l @(4,r4),{{r[0-9]+}}
; CHECK-DAG: mov.w @r4,{{r[0-9]+}}
; CHECK-NOT: add #4,r4

; Preserve the established generic policy under -Os, where changing LSR's
; decisions causes a net corpus size regression.
define i64 @size(ptr %data, i32 %n, i32 %seed) #0 {
entry:
	br label %loop

loop:
	%value = phi i32 [ %seed, %entry ], [ %next, %body ]
	%i = phi i32 [ 0, %entry ], [ %inc, %body ]
	%done = icmp eq i32 %i, %n
	br i1 %done, label %exit, label %body

body:
	%record = getelementptr [8 x i8], ptr %data, i32 %i
	%value.addr = getelementptr i8, ptr %record, i32 4
	%field = load i32, ptr %value.addr, align 4
	%sum = or i32 %value, %field
	%tag = load i16, ptr %record, align 4
	%tag.ext = zext i16 %tag to i32
	%next = xor i32 %sum, %tag.ext
	%inc = add i32 %i, 1
	br label %loop

exit:
	%result = zext i32 %value to i64
	ret i64 %result
}

; CHECK-LABEL: size:
; CHECK: add #4,r4
; CHECK: mov.l @r4,r1
; CHECK: mov r4,[[BASE:r[0-9]+]]
; CHECK-NEXT: add #-4,[[BASE]]
; CHECK-NEXT: mov.w @[[BASE]],{{r[0-9]+}}

attributes #0 = { optsize }
