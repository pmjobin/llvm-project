; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llvm-dwarfdump --verify %t.be.o %t.le.o
; RUN: llvm-extract -S --delete --func=interrupt_call %s -o %t.fdpic.ll
; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -O2 -verify-machineinstrs -filetype=obj %t.fdpic.ll -o %t.fdpic.le.o
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -O2 -verify-machineinstrs -filetype=obj %t.fdpic.ll -o %t.fdpic.be.o
; RUN: llvm-dwarfdump --verify %t.fdpic.le.o %t.fdpic.be.o

; Hardware-handler entry has no FDPIC descriptor context, so that deliberately
; unsupported combination is excluded from the two FDPIC object checks.

declare i32 @five(i32, i32, i32, i32, i32)
declare i32 @six(i32, i32, i32, i32, i32, i32)
declare void @escape(ptr)

; One local word above eight outgoing bytes, with PR above both. The local is
; disjoint from each callee's stack arguments and SP stays fixed between calls.
define i32 @fixed_and_calls() optsize uwtable {
	%local = alloca i32, align 4
	store volatile i32 7, ptr %local, align 4
	%first = call i32 @five(i32 1, i32 2, i32 3, i32 4, i32 5)
	store volatile i32 %first, ptr %local, align 4
	%second = call i32 @six(i32 6, i32 7, i32 8, i32 9, i32 10, i32 11)
	%saved = load volatile i32, ptr %local, align 4
	%result = add i32 %second, %saved
	ret i32 %result
}

; CHECK-LABEL: fixed_and_calls:
; CHECK: sts.l pr,@-r15
; CHECK: add #-12,r15
; CHECK: .cfi_def_cfa_offset 16
; CHECK: mov.l {{r[0-9]+}},@(8,r15)
; CHECK: jsr
; CHECK-NOT: add {{.*}},r15
; CHECK: mov.l r0,@(8,r15)
; CHECK-NOT: add {{.*}},r15
; CHECK: jsr
; CHECK-NOT: add {{.*}},r15
; CHECK: mov.l @(8,r15),{{r[0-9]+}}
; CHECK: add #12,r15
; CHECK: .cfi_def_cfa_offset 4
; CHECK: lds.l @r15+,pr

; A dynamic stack still uses an established FP and per-call SP adjustments.
define i32 @dynamic_and_call(i32 %count) optsize uwtable {
	%local = alloca i32, i32 %count, align 4
	call void @escape(ptr %local)
	%result = call i32 @five(i32 1, i32 2, i32 3, i32 4, i32 5)
	ret i32 %result
}

; CHECK-LABEL: dynamic_and_call:
; CHECK: mov r15,r14
; CHECK: add #-4,r15
; CHECK: jsr
; CHECK: add #4,r15
; CHECK: mov r14,r15

define i32 @realigned_call() optsize uwtable {
	%local = alloca i32, align 16
	call void @escape(ptr %local)
	%result = call i32 @five(i32 1, i32 2, i32 3, i32 4, i32 5)
	ret i32 %result
}

; CHECK-LABEL: realigned_call:
; CHECK: mov r15,r13
; CHECK: add #-4,r15
; CHECK: jsr
; CHECK: add #4,r15

define void @interrupt_call() optsize uwtable "sh-interrupt-handler"="interrupt" {
	%result = call i32 @five(i32 1, i32 2, i32 3, i32 4, i32 5)
	ret void
}

; CHECK-LABEL: interrupt_call:
; CHECK: add #-4,r15
; CHECK: jsr
; CHECK: add #4,r15
; CHECK: rte

; Explicit FP requests also retain the existing call-frame convention.
define i32 @forced_fp_call() optsize uwtable "frame-pointer"="all" {
	%result = call i32 @five(i32 1, i32 2, i32 3, i32 4, i32 5)
	ret i32 %result
}

; CHECK-LABEL: forced_fp_call:
; CHECK: mov r15,r14
; CHECK: add #-4,r15
; CHECK: jsr
; CHECK: add #4,r15

; Exercise materialized SP adjustments and incoming aggregate words after
; reserving an outgoing call area larger than both immediate ranges.
declare i32 @take_large([84 x i32])

define i32 @large_outgoing([84 x i32] %value) optsize uwtable {
	%result = call i32 @take_large([84 x i32] %value)
	ret i32 %result
}
