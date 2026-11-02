; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O3 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O3 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -enable-new-pm -mtriple=shle-unknown-elf -mcpu=sh2 -O3 -verify-machineinstrs %s -o - | FileCheck %s

; The specialized load-speculation pass is omitted. Preserve guarded entry,
; the early-match exit, and the decrement in the ordinary loop backedge.

define i32 @walk_until(ptr %data, i32 %n, i32 %key, i32 %base) {
entry:
	%empty = icmp eq i32 %n, 0
	%limit = sub i32 0, %n
	br i1 %empty, label %empty.exit, label %body
body:
	%p = phi ptr [ %data, %entry ], [ %next.p, %latch ]
	%current = phi i32 [ 0, %entry ], [ %next, %latch ]
	%value = load i32, ptr %p, align 4
	%equal = icmp eq i32 %value, %key
	%next.p = getelementptr i32, ptr %p, i32 1
	br i1 %equal, label %found, label %latch, !prof !0
latch:
	%next = add i32 %current, -1
	%done = icmp eq i32 %next, %limit
	br i1 %done, label %exhausted, label %body, !prof !0
found:
	%answer = add i32 %base, %current
	ret i32 %answer
exhausted:
	%last = add i32 %base, %next
	ret i32 %last
empty.exit:
	ret i32 %base
}

!0 = !{!"branch_weights", i32 1, i32 31}

; CHECK-LABEL: walk_until:
; CHECK: tst r5,r5
; CHECK: bt/s
; CHECK: mov.l @r4+,[[LOAD:r[0-9]+]]
; CHECK-NEXT: cmp/eq r6,[[LOAD]]
; CHECK-NEXT: bt
; CHECK: {{dt r5|add #-1,r[0-9]+}}
; CHECK: bf
