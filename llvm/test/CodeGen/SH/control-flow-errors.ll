; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/return-i1.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MATERIALIZE
; RUN: not --crash llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/store-i1.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MATERIALIZE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/zext-i1.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MATERIALIZE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/sext-i1.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MATERIALIZE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/select.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MATERIALIZE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/phi-i1.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=PHI
; RUN: not --crash llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/pointer.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=POINTER
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/switch.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SWITCH
; RUN: not --crash llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/indirectbr.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INDIRECT

; MATERIALIZE: LLVM ERROR: SH comparison results may only be used by conditional branches
; PHI: LLVM ERROR: SH i1 PHIs are not supported
; POINTER: LLVM ERROR: SH pointer comparisons are not supported
; SWITCH: LLVM ERROR: SH switch is not supported
; INDIRECT: LLVM ERROR: SH indirectbr is not supported

;--- return-i1.ll
define i1 @return_i1(i32 %a, i32 %b) {
	%condition = icmp eq i32 %a, %b
	ret i1 %condition
}

;--- store-i1.ll
define void @store_i1(ptr %p, i32 %a, i32 %b) {
	%condition = icmp eq i32 %a, %b
	store i1 %condition, ptr %p, align 1
	ret void
}

;--- zext-i1.ll
define i32 @zext_i1(i32 %a, i32 %b) {
	%condition = icmp eq i32 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

;--- sext-i1.ll
define i32 @sext_i1(i32 %a, i32 %b) {
	%condition = icmp eq i32 %a, %b
	%result = sext i1 %condition to i32
	ret i32 %result
}

;--- select.ll
define i32 @select_i32(i32 %a, i32 %b) {
	%condition = icmp eq i32 %a, %b
	%result = select i1 %condition, i32 %a, i32 %b
	ret i32 %result
}

;--- phi-i1.ll
define i32 @phi_i1() {
entry:
	br label %merge
merge:
	%condition = phi i1 [ true, %entry ]
	%result = zext i1 %condition to i32
	ret i32 %result
}

;--- pointer.ll
define i32 @pointer_cmp(ptr %a, ptr %b, i32 %x, i32 %y) {
	%condition = icmp eq ptr %a, %b
	br i1 %condition, label %same, label %different
same:
	ret i32 %x
different:
	ret i32 %y
}

;--- switch.ll
define i32 @switch_i32(i32 %value) {
entry:
	switch i32 %value, label %default [ i32 0, label %zero ]
zero:
	ret i32 0
default:
	ret i32 %value
}

;--- indirectbr.ll
define void @indirect_branch(ptr %address) {
entry:
	indirectbr ptr %address, [label %destination]
destination:
	ret void
}
