; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s

define void @store_i8(ptr %p, i32 %value) nounwind {
; CHECK-LABEL: store_i8:
; CHECK-NEXT: mov.b	r5,@r4
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%narrow = trunc i32 %value to i8
	store i8 %narrow, ptr %p, align 1
	ret void
}

define void @store_i16(ptr %p, i32 %value) nounwind {
; CHECK-LABEL: store_i16:
; CHECK-NEXT: mov.w	r5,@r4
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%narrow = trunc i32 %value to i16
	store i16 %narrow, ptr %p, align 2
	ret void
}

define void @store_i8_offset(ptr %p, i32 %value) nounwind {
; CHECK-LABEL: store_i8_offset:
; CHECK-NEXT: add	#7,r4
; CHECK-NEXT: mov.b	r5,@r4
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%address = getelementptr i8, ptr %p, i32 7
	%narrow = trunc i32 %value to i8
	store i8 %narrow, ptr %address, align 1
	ret void
}

define void @store_i16_offset(ptr %p, i32 %value) nounwind {
; CHECK-LABEL: store_i16_offset:
; CHECK-NEXT: add	#10,r4
; CHECK-NEXT: mov.w	r5,@r4
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%address = getelementptr i8, ptr %p, i32 10
	%narrow = trunc i32 %value to i16
	store i16 %narrow, ptr %address, align 2
	ret void
}
