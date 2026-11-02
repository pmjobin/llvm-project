; RUN: llc -mtriple=sh-unknown-elf -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -O2 -verify-machineinstrs %s -o - | FileCheck %s

define i32 @output_i8_input_i16(i32 %wide) {
	%input = trunc i32 %wide to i16
	%value = call i8 asm "add #1,$0", "=r,0"(i16 %input)
	%extended = zext i8 %value to i32
	ret i32 %extended
}

; CHECK-LABEL: output_i8_input_i16:
; CHECK: add #1,r4
; CHECK: extu.b r4,r0

define i32 @output_i16_input_i8(i32 %wide) {
	%input = trunc i32 %wide to i8
	%value = call i16 asm "add #1,$0", "=r,0"(i8 %input)
	%extended = zext i16 %value to i32
	ret i32 %extended
}

; CHECK-LABEL: output_i16_input_i8:
; CHECK: add #1,r4
; CHECK: extu.w r4,r0

define i32 @output_i32_input_i16(i32 %wide) {
	%input = trunc i32 %wide to i16
	%value = call i32 asm "add #1,$0", "=r,0"(i16 %input)
	ret i32 %value
}

; CHECK-LABEL: output_i32_input_i16:
; CHECK: mov r4,r0
; CHECK: add #1,r0
