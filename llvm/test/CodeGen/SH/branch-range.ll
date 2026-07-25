; RUN: rm -f %t-be.o %t-le.o
; RUN: not llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj < %s -o %t-be.o 2>&1 | FileCheck %s
; RUN: not test -e %t-be.o
; RUN: not llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj < %s -o %t-le.o 2>&1 | FileCheck %s
; RUN: not test -e %t-le.o

; CHECK: error: SH branch target is out of range
; CHECK-NOT: LLVM ERROR
; CHECK-NOT: assertion

define i32 @conditional_too_far(ptr %p, i32 %a, i32 %b) #0 {
entry:
	%equal = icmp eq i32 %a, %b
	br i1 %equal, label %far, label %padding
padding:
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	store volatile i32 %a, ptr %p, align 4
	ret i32 %a
far:
	ret i32 %b
}

attributes #0 = { noinline optnone }
