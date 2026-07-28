; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s

define i32 @load_s8(ptr %p) {
; CHECK-LABEL: load_s8:
; CHECK-NEXT: mov.b	@r4,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%value = load i8, ptr %p, align 1
	%extended = sext i8 %value to i32
	ret i32 %extended
}

define i32 @load_u8(ptr %p) {
; CHECK-LABEL: load_u8:
; CHECK-NEXT: mov.b	@r4,r0
; CHECK-NEXT: extu.b	r0,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%value = load i8, ptr %p, align 1
	%extended = zext i8 %value to i32
	ret i32 %extended
}

define i32 @load_s16(ptr %p) {
; CHECK-LABEL: load_s16:
; CHECK-NEXT: mov.w	@r4,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%value = load i16, ptr %p, align 2
	%extended = sext i16 %value to i32
	ret i32 %extended
}

define i32 @load_u16(ptr %p) {
; CHECK-LABEL: load_u16:
; CHECK-NEXT: mov.w	@r4,r0
; CHECK-NEXT: extu.w	r0,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%value = load i16, ptr %p, align 2
	%extended = zext i16 %value to i32
	ret i32 %extended
}

define i32 @load_s8_offset(ptr %p) {
; CHECK-LABEL: load_s8_offset:
; CHECK-NEXT: add	#3,r4
; CHECK-NEXT: mov.b	@r4,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%address = getelementptr i8, ptr %p, i32 3
	%value = load i8, ptr %address, align 1
	%extended = sext i8 %value to i32
	ret i32 %extended
}

define i32 @load_u16_offset(ptr %p) {
; CHECK-LABEL: load_u16_offset:
; CHECK-NEXT: add	#6,r4
; CHECK-NEXT: mov.w	@r4,r0
; CHECK-NEXT: extu.w	r0,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%address = getelementptr i8, ptr %p, i32 6
	%value = load i16, ptr %address, align 2
	%extended = zext i16 %value to i32
	ret i32 %extended
}
