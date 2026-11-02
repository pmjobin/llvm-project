; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh1 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2a -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM

define ptr @frame_address_minimum() {
entry:
	%object = alloca i8, align 1
	%address = getelementptr i8, ptr %object, i32 -2147483648
	ret ptr %address
}

; ISEL-LABEL: name:            frame_address_minimum
; ISEL: LEA_FI %stack.0.object, -2147483648

define ptr @frame_address_negative_four() {
entry:
	%object = alloca i32, align 4
	%address = getelementptr i8, ptr %object, i32 -4
	ret ptr %address
}

; ISEL-LABEL: name:            frame_address_negative_four
; ISEL: LEA_FI %stack.0.object, -4
; ASM-LABEL: frame_address_negative_four:
; ASM: mov #4,[[OFFSET:r[0-9]+]]
; ASM-NEXT: neg [[OFFSET]],[[OFFSET]]
; ASM-NEXT: add r15,[[OFFSET]]

define ptr @frame_address_negative_one() {
entry:
	%object = alloca i8, align 1
	%address = getelementptr i8, ptr %object, i32 -1
	ret ptr %address
}

; ISEL-LABEL: name:            frame_address_negative_one
; ISEL: LEA_FI %stack.0.object, -1

define ptr @frame_address_zero() {
entry:
	%object = alloca i8, align 1
	ret ptr %object
}

; ISEL-LABEL: name:            frame_address_zero
; ISEL: LEA_FI %stack.0.object, 0

define ptr @frame_address_positive_one() {
entry:
	%object = alloca i8, align 1
	%address = getelementptr i8, ptr %object, i32 1
	ret ptr %address
}

; ISEL-LABEL: name:            frame_address_positive_one
; ISEL: LEA_FI %stack.0.object, 1

define ptr @frame_address_maximum() {
entry:
	%object = alloca i8, align 1
	%address = getelementptr i8, ptr %object, i32 2147483647
	ret ptr %address
}

; ISEL-LABEL: name:            frame_address_maximum
; ISEL: LEA_FI %stack.0.object, 2147483647

define i8 @load_i8_before_frame() {
entry:
	%object = alloca i8, align 1
	%address = getelementptr i8, ptr %object, i32 -1
	%value = load volatile i8, ptr %address, align 1
	ret i8 %value
}

; ISEL-LABEL: name:            load_i8_before_frame
; ISEL: MOVB_load_frame %stack.0.object, -1

define i16 @load_i16_before_frame() {
entry:
	%object = alloca i16, align 2
	%address = getelementptr i8, ptr %object, i32 -2
	%value = load volatile i16, ptr %address, align 2
	ret i16 %value
}

; ISEL-LABEL: name:            load_i16_before_frame
; ISEL: MOVW_load_frame %stack.0.object, -2

define i32 @load_i32_before_frame() {
entry:
	%object = alloca i32, align 4
	%address = getelementptr i8, ptr %object, i32 -4
	%value = load volatile i32, ptr %address, align 4
	ret i32 %value
}

; ISEL-LABEL: name:            load_i32_before_frame
; ISEL: LEA_FI %stack.0.object, -4

define i64 @load_i64_before_frame() {
entry:
	%object = alloca i64, align 4
	%address = getelementptr i8, ptr %object, i32 -8
	%value = load volatile i64, ptr %address, align 4
	ret i64 %value
}

; ISEL-LABEL: name:            load_i64_before_frame
; The second half reuses the first address: (frame - 8) + 4 = frame - 4.
; ISEL: [[I64BASE:%[0-9]+]]:gpr = LEA_FI %stack.0.object, -8
; ISEL: MOVL_load_reg [[I64BASE]] :: (volatile load (s32)
; ISEL: MOVL_load_disp [[I64BASE]], 4 :: (volatile load (s32)

define float @load_f32_before_frame() {
entry:
	%object = alloca float, align 4
	%address = getelementptr i8, ptr %object, i32 -4
	%value = load volatile float, ptr %address, align 4
	ret float %value
}

; ISEL-LABEL: name:            load_f32_before_frame
; ISEL: LEA_FI %stack.0.object, -4

define double @load_f64_before_frame() {
entry:
	%object = alloca double, align 4
	%address = getelementptr i8, ptr %object, i32 -8
	%value = load volatile double, ptr %address, align 4
	ret double %value
}


; ISEL-LABEL: name:            load_f64_before_frame
; ISEL: [[F64BASE:%[0-9]+]]:gpr = LEA_FI %stack.0.object, -8
; ISEL: MOVL_load_reg [[F64BASE]] :: (volatile load (s32)
; ISEL: MOVL_load_disp [[F64BASE]], 4 :: (volatile load (s32)
