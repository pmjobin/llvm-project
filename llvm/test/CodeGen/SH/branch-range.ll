; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

; CHECK-LABEL: conditional_too_far:
; CHECK: cmp/eq
; CHECK: bf	[[NEAR:.LBB[0-9_]+]]
; CHECK-NEXT: bra	[[FAR:.LBB[0-9_]+]]
; CHECK-NEXT: nop
; CHECK: [[NEAR]]:
; CHECK: [[FAR]]:
; CHECK: rts
; CHECK-NEXT: nop

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
