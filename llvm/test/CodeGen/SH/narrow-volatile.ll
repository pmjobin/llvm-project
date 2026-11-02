; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s

define void @volatile_i8(ptr %src, ptr %dst) {
; CHECK-LABEL: volatile_i8:
; CHECK: mov.b	@r4,[[FIRST:r[0-9]+]]
; CHECK: mov.b	[[FIRST]],@r5
; CHECK: mov.b	@r4,[[SECOND:r[0-9]+]]
; CHECK: mov.b	[[SECOND]],@r5
; CHECK: rts
	%first = load volatile i8, ptr %src, align 1
	store volatile i8 %first, ptr %dst, align 1
	%second = load volatile i8, ptr %src, align 1
	store volatile i8 %second, ptr %dst, align 1
	ret void
}

define void @volatile_i16(ptr %src, ptr %dst) {
; CHECK-LABEL: volatile_i16:
; CHECK: mov.w	@r4,[[FIRST:r[0-9]+]]
; CHECK: mov.w	[[FIRST]],@r5
; CHECK: mov.w	@r4,[[SECOND:r[0-9]+]]
; CHECK: mov.w	[[SECOND]],@r5
; CHECK: rts
	%first = load volatile i16, ptr %src, align 2
	store volatile i16 %first, ptr %dst, align 2
	%second = load volatile i16, ptr %src, align 2
	store volatile i16 %second, ptr %dst, align 2
	ret void
}
