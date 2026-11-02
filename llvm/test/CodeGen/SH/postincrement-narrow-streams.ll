; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O3 -verify-machineinstrs -stop-after=loop-reduce %s -o - | FileCheck %s --check-prefixes=CHECK,STREAM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O3 -verify-machineinstrs -stop-after=loop-reduce %s -o - | FileCheck %s --check-prefixes=CHECK,STREAM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs -stop-after=loop-reduce %s -o - | FileCheck %s --check-prefixes=CHECK,INDEX
; RUN: llc -enable-new-pm -mtriple=shle-unknown-elf -mcpu=sh3 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM

; The integer IV remains needed as data. Do not rebuild each byte address
; around it when the pointer's fixed update can be folded into the load.
define i32 @accumulate_bytes(ptr %data, i32 %n) {
; CHECK-LABEL: define i32 @accumulate_bytes(
; STREAM: phi ptr
; INDEX-NOT: phi ptr
; INDEX: getelementptr i8, ptr %data, i32
; ASM-LABEL: accumulate_bytes:
; ASM: mov.b @r{{[0-9]+}}+,r{{[0-9]+}}
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %body
body:
	%i = phi i32 [ 0, %entry ], [ %next, %body ]
	%sum = phi i32 [ 0, %entry ], [ %added, %body ]
	%p = getelementptr i8, ptr %data, i32 %i
	%x = load i8, ptr %p, align 1
	%low = trunc i32 %i to i8
	%value = add i8 %x, %low
	%wide = zext i8 %value to i32
	%added = add i32 %sum, %wide
	%next = add nuw i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %exit, label %body
exit:
	%result = phi i32 [ 0, %entry ], [ %added, %body ]
	ret i32 %result
}

define i32 @size_bytes(ptr %data, i32 %n) optsize {
; CHECK-LABEL: define i32 @size_bytes(
; CHECK-NOT: phi ptr
; CHECK: getelementptr i8, ptr %data, i32
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %body
body:
	%i = phi i32 [ 0, %entry ], [ %next, %body ]
	%sum = phi i32 [ 0, %entry ], [ %added, %body ]
	%p = getelementptr i8, ptr %data, i32 %i
	%x = load i8, ptr %p, align 1
	%low = trunc i32 %i to i8
	%value = add i8 %x, %low
	%wide = zext i8 %value to i32
	%added = add i32 %sum, %wide
	%next = add nuw i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %exit, label %body
exit:
	%result = phi i32 [ 0, %entry ], [ %added, %body ]
	ret i32 %result
}

; The hardware update must equal the access width. A stride-two byte access
; is not a byte postincrement stream, even though its SCEV is affine.
define i32 @gapped_bytes(ptr %data, i32 %n) {
; CHECK-LABEL: define i32 @gapped_bytes(
; ASM-LABEL: gapped_bytes:
; ASM-NOT: mov.b @r{{[0-9]+}}+
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %body
body:
	%i = phi i32 [ 0, %entry ], [ %next, %body ]
	%sum = phi i32 [ 0, %entry ], [ %added, %body ]
	%offset = shl i32 %i, 1
	%p = getelementptr i8, ptr %data, i32 %offset
	%x = load i8, ptr %p, align 1
	%low = trunc i32 %i to i8
	%value = add i8 %x, %low
	%wide = zext i8 %value to i32
	%added = add i32 %sum, %wide
	%next = add nuw i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %exit, label %body
exit:
	%result = phi i32 [ 0, %entry ], [ %added, %body ]
	ret i32 %result
}
