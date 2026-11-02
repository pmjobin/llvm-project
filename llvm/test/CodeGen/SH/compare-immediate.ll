; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,SH2
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,SH4

declare i32 @callee()

define i1 @compare_argument(i32 %value) {
entry:
	%result = icmp eq i32 %value, 7
	ret i1 %result
}

; CHECK-LABEL: compare_argument:
; CHECK: mov #7,r0
; CHECK-NEXT: cmp/eq r0,r4

define i1 @compare_size_argument(i32 %value) optsize {
entry:
	%result = icmp eq i32 %value, 7
	ret i1 %result
}

; CHECK-LABEL: compare_size_argument:
; CHECK: mov r4,r0
; CHECK-NEXT: cmp/eq #7,r0

define i1 @compare_call_result() {
entry:
	%value = call i32 @callee()
	%result = icmp eq i32 %value, -7
	ret i1 %result
}

; CHECK-LABEL: compare_call_result:
; CHECK: jsr @r0
; CHECK: cmp/eq #-7,r0
; CHECK-NEXT: movt r0

define i32 @branch_on_call_result() {
entry:
	%value = call i32 @callee()
	%result = icmp eq i32 %value, 42
	%selected = select i1 %result, i32 11, i32 17
	ret i32 %selected
}

; CHECK-LABEL: branch_on_call_result:
; CHECK: jsr @r0
; CHECK: cmp/eq #42,r0
; SH4-NEXT: mov #11,r0
; CHECK-NEXT: bt{{(/s)?}} [[TRUE:\.LBB[0-9]+_[0-9]+]]
; SH2-NEXT: mov #11,r0
; CHECK: mov #17,r0
; CHECK: [[TRUE]]:
