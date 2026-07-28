; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define i32 @division_relaxes_conditional(i32 %a, i32 %b, i32 %c) {
; CHECK-LABEL: division_relaxes_conditional:
; CHECK: cmp/eq
; CHECK: bf	[[DIVIDE:.LBB[0-9_]+]]
; CHECK-NEXT: bra	[[FAR:.LBB[0-9_]+]]
; CHECK-NEXT: nop
; CHECK: [[DIVIDE]]:
; CHECK: div0u
; CHECK-COUNT-32: div1
; CHECK: rotcl
; CHECK: div0s
; CHECK-COUNT-32: div1
; CHECK: rotcl
; CHECK-NEXT: addc
; CHECK: bra	[[FAR]]
; CHECK-NEXT: nop
; CHECK: [[FAR]]:
; CHECK: rts
; CHECK-NEXT: nop
entry:
	%equal = icmp eq i32 %a, %b
	br i1 %equal, label %far, label %divide

divide:
	%unsigned = udiv i32 %a, %b
	%signed = sdiv i32 %unsigned, %c
	br label %far

far:
	%result = phi i32 [ %b, %entry ], [ %signed, %divide ]
	ret i32 %result
}
