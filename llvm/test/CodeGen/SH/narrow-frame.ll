; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s

define i32 @narrow_frame(i32 %byte, i32 %word) {
; CHECK-LABEL: narrow_frame:
; CHECK: add	#-{{[0-9]+}},r15
; CHECK: mov	{{r[0-9]+}},r0
; CHECK-NEXT: mov.b	r0,@({{[0-9]+}},r15)
; CHECK: mov.w	{{r[0-9]+}},@r15
; CHECK: mov.b	@({{[0-9]+}},r15),r0
; CHECK-NEXT: mov	r0,{{r[0-9]+}}
; CHECK: mov.w	@r15,{{r[0-9]+}}
; CHECK: add	#{{[0-9]+}},r15
	%byte_slot = alloca i8, align 1
	%word_slot = alloca i16, align 2
	%byte_value = trunc i32 %byte to i8
	%word_value = trunc i32 %word to i16
	store volatile i8 %byte_value, ptr %byte_slot, align 1
	store volatile i16 %word_value, ptr %word_slot, align 2
	%loaded_byte = load volatile i8, ptr %byte_slot, align 1
	%loaded_word = load volatile i16, ptr %word_slot, align 2
	%extended_byte = zext i8 %loaded_byte to i32
	%extended_word = zext i16 %loaded_word to i32
	%result = add i32 %extended_byte, %extended_word
	ret i32 %result
}

define internal i32 @narrow_callee(i32 %value) noinline {
	ret i32 %value
}

define i32 @narrow_nonleaf(i32 %value) {
; CHECK-LABEL: narrow_nonleaf:
; CHECK-COUNT-1: sts.l	pr,@-r15
; CHECK: mov.b	r0,@({{[0-9]+}},r15)
; CHECK: bsr	narrow_callee
; CHECK-NEXT: nop
; CHECK: mov.b	@({{[0-9]+}},r15),r0
; CHECK-COUNT-1: lds.l	@r15+,pr
	%slot = alloca i8, align 1
	%narrow = trunc i32 %value to i8
	store volatile i8 %narrow, ptr %slot, align 1
	%call = call i32 @narrow_callee(i32 %value)
	%loaded = load volatile i8, ptr %slot, align 1
	%extended = zext i8 %loaded to i32
	%result = add i32 %call, %extended
	ret i32 %result
}
