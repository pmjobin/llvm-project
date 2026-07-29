; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/unaligned-load.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNALIGNED-LOAD
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/unaligned-store.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNALIGNED-STORE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/dynamic-alloca.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=DYNAMIC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/large-frame.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=LARGE-FRAME
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/overaligned.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=OVERALIGNED
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/alloca-escape.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ALLOCA-ESCAPE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/register-offset.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS

; UNALIGNED-LOAD: LLVM ERROR: SH requires 4-byte alignment for 32- and 64-bit loads
; UNALIGNED-STORE: LLVM ERROR: SH requires 4-byte alignment for 32- and 64-bit stores
; DYNAMIC: LLVM ERROR: SH dynamic alloca is not supported
; LARGE-FRAME: LLVM ERROR: SH stack frame size cannot exceed 60 bytes
; OVERALIGNED: LLVM ERROR: SH stack object alignment cannot exceed 4 bytes
; ALLOCA-ESCAPE: LLVM ERROR: SH stack object address escape is not supported
; ADDRESS: LLVM ERROR: SH memory address must be a register or supported 32-bit constant address addition

;--- unaligned-load.ll
define i32 @unaligned_load(ptr %p) {
	%value = load volatile i32, ptr %p, align 2
	ret i32 %value
}

;--- unaligned-store.ll
define void @unaligned_store(ptr %p, i32 %value) {
	store volatile i32 %value, ptr %p, align 1
	ret void
}

;--- dynamic-alloca.ll
define i32 @dynamic_alloca(i32 %count, i32 %value) {
	%slot = alloca i32, i32 %count, align 4
	store volatile i32 %value, ptr %slot, align 4
	%result = load volatile i32, ptr %slot, align 4
	ret i32 %result
}

;--- large-frame.ll
define void @large_frame(i32 %value) {
	%s0 = alloca i32, align 4
	%s1 = alloca i32, align 4
	%s2 = alloca i32, align 4
	%s3 = alloca i32, align 4
	%s4 = alloca i32, align 4
	%s5 = alloca i32, align 4
	%s6 = alloca i32, align 4
	%s7 = alloca i32, align 4
	%s8 = alloca i32, align 4
	%s9 = alloca i32, align 4
	%s10 = alloca i32, align 4
	%s11 = alloca i32, align 4
	%s12 = alloca i32, align 4
	%s13 = alloca i32, align 4
	%s14 = alloca i32, align 4
	%s15 = alloca i32, align 4
	store volatile i32 %value, ptr %s0, align 4
	store volatile i32 %value, ptr %s1, align 4
	store volatile i32 %value, ptr %s2, align 4
	store volatile i32 %value, ptr %s3, align 4
	store volatile i32 %value, ptr %s4, align 4
	store volatile i32 %value, ptr %s5, align 4
	store volatile i32 %value, ptr %s6, align 4
	store volatile i32 %value, ptr %s7, align 4
	store volatile i32 %value, ptr %s8, align 4
	store volatile i32 %value, ptr %s9, align 4
	store volatile i32 %value, ptr %s10, align 4
	store volatile i32 %value, ptr %s11, align 4
	store volatile i32 %value, ptr %s12, align 4
	store volatile i32 %value, ptr %s13, align 4
	store volatile i32 %value, ptr %s14, align 4
	store volatile i32 %value, ptr %s15, align 4
	ret void
}

;--- overaligned.ll
define i32 @overaligned(i32 %value) {
	%slot = alloca i32, align 8
	store volatile i32 %value, ptr %slot, align 4
	%result = load volatile i32, ptr %slot, align 4
	ret i32 %result
}

;--- alloca-escape.ll
define ptr @alloca_escape() {
	%slot = alloca i32, align 4
	ret ptr %slot
}

;--- offset-64.ll
define i32 @offset_64(ptr %p) {
	%address = getelementptr i8, ptr %p, i32 64
	%value = load volatile i32, ptr %address, align 4
	ret i32 %value
}

;--- negative-offset.ll
define i32 @negative_offset(ptr %p) {
	%address = getelementptr i8, ptr %p, i32 -4
	%value = load volatile i32, ptr %address, align 4
	ret i32 %value
}

;--- register-offset.ll
define i32 @register_offset(ptr %p, i32 %offset) {
	%address = getelementptr i8, ptr %p, i32 %offset
	%value = load volatile i32, ptr %address, align 4
	ret i32 %value
}
