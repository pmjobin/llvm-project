; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s

define i32 @sext_i8(i32 %value) nounwind {
; CHECK-LABEL: sext_i8:
; CHECK-NEXT: exts.b	r4,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%narrow = trunc i32 %value to i8
	%result = sext i8 %narrow to i32
	ret i32 %result
}

define i32 @zext_i8(i32 %value) nounwind {
; CHECK-LABEL: zext_i8:
; CHECK-NEXT: extu.b	r4,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%narrow = trunc i32 %value to i8
	%result = zext i8 %narrow to i32
	ret i32 %result
}

define i32 @sext_i16(i32 %value) nounwind {
; CHECK-LABEL: sext_i16:
; CHECK-NEXT: exts.w	r4,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%narrow = trunc i32 %value to i16
	%result = sext i16 %narrow to i32
	ret i32 %result
}

define i32 @zext_i16(i32 %value) nounwind {
; CHECK-LABEL: zext_i16:
; CHECK-NEXT: extu.w	r4,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%narrow = trunc i32 %value to i16
	%result = zext i16 %narrow to i32
	ret i32 %result
}
