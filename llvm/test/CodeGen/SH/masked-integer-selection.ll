; RUN: llc -mtriple=shle -mcpu=sh2 -O3 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh -mcpu=sh2 -O3 -verify-machineinstrs %s -o - | FileCheck %s
; The optional late mask rewrite is omitted. Ordinary shifts retain the result.
; The source window reaches the store exactly; memory accesses retain width.
define void @scaled_window(ptr %data, ptr %out, i32 %n) {
entry:
	%empty = icmp eq i32 %n, 0
	br i1 %empty, label %exit, label %loop
loop:
	%i = phi i32 [ 0, %entry ], [ %next, %loop ]
	%p = getelementptr i32, ptr %data, i32 %i
	%x = load i32, ptr %p, align 4
	%s = lshr exact i32 %x, 6
	%m = and i32 %s, 124
	%q = getelementptr i32, ptr %out, i32 %i
	store i32 %m, ptr %q, align 4
	%next = add nuw i32 %i, 1
	%done = icmp eq i32 %next, %n
	br i1 %done, label %exit, label %loop
exit:
	ret void
}
; CHECK-LABEL: scaled_window:
; CHECK: shlr2	[[R:r[0-9]+]]
; CHECK-COUNT-2: shlr2	[[R]]
; CHECK: and	{{.*}}[[R]]
; CHECK: mov.l	[[R]],@
