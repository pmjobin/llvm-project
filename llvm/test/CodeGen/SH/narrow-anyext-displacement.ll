; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -o - %s | FileCheck %s

; An any-extending narrow load may use the sign-extending SH displacement
; instruction because its high bits are deliberately unspecified. Keep the
; value in r0 when the following displacement store needs it there too.

target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define void @copy_adjacent_bytes(ptr %dest, ptr %source) #0 {
entry:
	%value0 = load i8, ptr %source, align 1
	store i8 %value0, ptr %dest, align 1
	%source1 = getelementptr i8, ptr %source, i32 1
	%value1 = load i8, ptr %source1, align 1
	%dest1 = getelementptr i8, ptr %dest, i32 1
	store i8 %value1, ptr %dest1, align 1
	ret void
}

; CHECK-LABEL: copy_adjacent_bytes:
; CHECK: mov.b @r5,r0
; CHECK-NEXT: mov.b r0,@r4
; CHECK-NEXT: mov.b @(1,r5),r0
; CHECK-NEXT: rts
; CHECK-NEXT: mov.b r0,@(1,r4)

attributes #0 = { optsize }
