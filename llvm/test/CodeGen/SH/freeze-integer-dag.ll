; REQUIRES: asserts
; RUN: llc -mtriple=shle -mcpu=sh2 -O0 -debug-only=isel %s -o /dev/null 2>&1 | FileCheck %s
; RUN: llc -mtriple=sh -mcpu=sh2 -O3 -debug-only=isel %s -o /dev/null 2>&1 | FileCheck %s

; Check the boundary before promotion/splitting, rather than depending on an
; incidental allocated register or only MachineVerifier acceptance.
; CHECK: Initial selection DAG:
; CHECK: i64 = freeze poison:i64
; CHECK: Optimized lowered selection DAG:
; CHECK-NOT: freeze
; CHECK: Constant:i64<0>
; CHECK-NOT: freeze
; CHECK: Type-legalized selection DAG:

define i64 @wide(ptr %out) {
	%f = freeze i64 poison
	store volatile i64 %f, ptr %out
	ret i64 %f
}
