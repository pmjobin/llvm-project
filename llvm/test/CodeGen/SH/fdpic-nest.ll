; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -O2 -filetype=obj -verify-machineinstrs < %s -o %t.le.o
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -O2 -filetype=obj -verify-machineinstrs < %s -o %t.be.o
; RUN: llvm-readobj --file-headers --relocations %t.le.o %t.be.o | FileCheck %s --check-prefix=OBJECT

define internal i32 @nested_target(ptr nest %chain, i32 %value) noinline {
entry:
	%context = load i32, ptr %chain, align 4
	%result = add i32 %context, %value
	ret i32 %result
}

define i32 @call_nested(ptr %chain, i32 %value) {
entry:
	%result = call i32 @nested_target(ptr nest %chain, i32 %value)
	ret i32 %result
}

define i32 @call_nested_indirect(ptr %target, ptr %chain, i32 %value) {
entry:
	%result = call i32 %target(ptr nest %chain, i32 %value)
	ret i32 %result
}

define i32 @main() {
entry:
	%chain = alloca i32, align 4
	store i32 35, ptr %chain, align 4
	%direct = call i32 @call_nested(ptr %chain, i32 7)
	%indirect = call i32 @call_nested_indirect(ptr @nested_target, ptr %chain, i32 7)
	%direct.ok = icmp eq i32 %direct, 42
	%indirect.ok = icmp eq i32 %indirect, 42
	%ok = and i1 %direct.ok, %indirect.ok
	%status = select i1 %ok, i32 0, i32 1
	ret i32 %status
}

; ASM-LABEL: nested_target:
; ASM: mov.l	@r3,

; ASM-LABEL: call_nested:
; ASM: mov	r4,r3
; ASM: mov	r5,r4
; ASM: bsrf

; ASM-LABEL: call_nested_indirect:
; ASM: mov.l	@r4,r1
; ASM-NEXT: mov.l	@(4,r4),r12
; ASM: mov	r5,r3
; ASM: jsr	@r1
; ASM-NEXT: mov	r6,r4

; OBJECT-COUNT-2: Flags [ (0x8002)
