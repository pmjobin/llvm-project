; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o

@global = global i32 1, align 4

define i32 @expanded_out_of_range(i32 %divisor) {
; CHECK-LABEL: expanded_out_of_range:
; CHECK: mov.l	[[POOL:.LCPI[0-9]+_0_0]],r{{[0-9]+}}
; CHECK-NEXT: bra	[[CONT:.LBB[0-9_]+]]
; CHECK-NEXT: nop
; CHECK: [[POOL]]:
; CHECK-NEXT: .long	global
; CHECK: [[CONT]]:
; CHECK: div0u
; CHECK: rts
; CHECK-NEXT: nop
	%seed = load volatile i32, ptr @global, align 4
	%q0 = udiv i32 %seed, %divisor
	%q1 = udiv i32 %q0, %divisor
	%q2 = udiv i32 %q1, %divisor
	%q3 = udiv i32 %q2, %divisor
	%q4 = udiv i32 %q3, %divisor
	%q5 = udiv i32 %q4, %divisor
	%q6 = udiv i32 %q5, %divisor
	%q7 = udiv i32 %q6, %divisor
	ret i32 %q7
}
