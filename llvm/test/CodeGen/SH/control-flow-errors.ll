; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/return-i1.ll -o - | FileCheck %s --check-prefix=RETURN
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/store-i1.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/zext-i1.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/sext-i1.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/select.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/phi-i1.ll -o /dev/null

; RETURN-LABEL: return_i1:
; RETURN: cmp/eq	r5,r4
; RETURN-NEXT: rts
; RETURN-NEXT: movt	r0

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
