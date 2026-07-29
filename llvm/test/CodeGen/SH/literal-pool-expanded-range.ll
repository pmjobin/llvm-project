; RUN: rm -f %t
; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -filetype=obj < %s -o %t 2>&1 | FileCheck %s
; RUN: not test -e %t

@global = global i32 1, align 4

define i32 @expanded_out_of_range(i32 %divisor) {
; CHECK: LLVM ERROR: SH literal pool entry is out of range: function expanded_out_of_range
; CHECK-SAME: allowed range 0..1020
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
