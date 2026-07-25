; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s

define i32 @load_i32(ptr %p) {
; CHECK-LABEL: load_i32:
; CHECK-NEXT: mov.l	@r4,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%value = load i32, ptr %p, align 4
	ret i32 %value
}

define void @store_i32(ptr %p, i32 %value) {
; CHECK-LABEL: store_i32:
; CHECK-NEXT: mov.l	r5,@r4
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	store i32 %value, ptr %p, align 4
	ret void
}

define i32 @load_offset_12(ptr %p) {
; CHECK-LABEL: load_offset_12:
; CHECK-NEXT: mov.l	@(12,r4),r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%address = getelementptr i8, ptr %p, i32 12
	%value = load i32, ptr %address, align 4
	ret i32 %value
}

define void @store_offset_12(ptr %p, i32 %value) {
; CHECK-LABEL: store_offset_12:
; CHECK-NEXT: mov.l	r5,@(12,r4)
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%address = getelementptr i8, ptr %p, i32 12
	store i32 %value, ptr %address, align 4
	ret void
}

define i32 @load_volatile_i32(ptr %p) {
; CHECK-LABEL: load_volatile_i32:
; CHECK-NEXT: mov.l	@r4,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%value = load volatile i32, ptr %p, align 4
	ret i32 %value
}

define void @store_volatile_i32(ptr %p, i32 %value) {
; CHECK-LABEL: store_volatile_i32:
; CHECK-NEXT: mov.l	r5,@r4
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	store volatile i32 %value, ptr %p, align 4
	ret void
}

define ptr @load_ptr(ptr %p) {
; CHECK-LABEL: load_ptr:
; CHECK-NEXT: mov.l	@r4,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%value = load ptr, ptr %p, align 4
	ret ptr %value
}

define void @store_ptr(ptr %p, ptr %value) {
; CHECK-LABEL: store_ptr:
; CHECK-NEXT: mov.l	r5,@r4
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	store ptr %value, ptr %p, align 4
	ret void
}

define i32 @load_offset_60(ptr %p) {
; CHECK-LABEL: load_offset_60:
; CHECK-NEXT: mov.l	@(60,r4),r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%address = getelementptr i8, ptr %p, i32 60
	%value = load i32, ptr %address, align 4
	ret i32 %value
}
