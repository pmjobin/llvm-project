; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh1 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2a -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s

define i32 @load_i32_constant_address() {
entry:
	%value = load volatile i32, ptr inttoptr (i32 28 to ptr), align 4
	ret i32 %value
}

; CHECK-LABEL: load_i32_constant_address:
; CHECK: mov #28,[[ADDRESS:r[0-9]+]]
; CHECK-NEXT: mov.l @[[ADDRESS]],r0

define void @store_i32_constant_address(i32 %value) {
entry:
	store volatile i32 %value, ptr inttoptr (i32 28 to ptr), align 4
	ret void
}

define i8 @load_i8_constant_address() {
entry:
	%value = load volatile i8, ptr inttoptr (i32 29 to ptr), align 1
	ret i8 %value
}

define void @store_i8_constant_address(i8 %value) {
entry:
	store volatile i8 %value, ptr inttoptr (i32 29 to ptr), align 1
	ret void
}

define i16 @load_i16_constant_address() {
entry:
	%value = load volatile i16, ptr inttoptr (i32 30 to ptr), align 2
	ret i16 %value
}

define void @store_i16_constant_address(i16 %value) {
entry:
	store volatile i16 %value, ptr inttoptr (i32 30 to ptr), align 2
	ret void
}

define i64 @load_i64_constant_address() {
entry:
	%value = load volatile i64, ptr inttoptr (i32 32 to ptr), align 4
	ret i64 %value
}

define void @store_i64_constant_address(i64 %value) {
entry:
	store volatile i64 %value, ptr inttoptr (i32 32 to ptr), align 4
	ret void
}

define float @load_f32_constant_address() {
entry:
	%value = load volatile float, ptr inttoptr (i32 40 to ptr), align 4
	ret float %value
}

define void @store_f32_constant_address(float %value) {
entry:
	store volatile float %value, ptr inttoptr (i32 40 to ptr), align 4
	ret void
}

define double @load_f64_constant_address() {
entry:
	%value = load volatile double, ptr inttoptr (i32 48 to ptr), align 4
	ret double %value
}

define void @store_f64_constant_address(double %value) {
entry:
	store volatile double %value, ptr inttoptr (i32 48 to ptr), align 4
	ret void
}

define i32 @load_minimum_constant_address() {
entry:
	%value = load volatile i32, ptr inttoptr (i32 -2147483648 to ptr), align 4
	ret i32 %value
}

define i32 @load_negative_one_constant_address() {
entry:
	%value = load volatile i32, ptr inttoptr (i32 -1 to ptr), align 1
	ret i32 %value
}

define i32 @load_zero_constant_address() {
entry:
	%value = load volatile i32, ptr null, align 1
	ret i32 %value
}

define i32 @load_positive_one_constant_address() {
entry:
	%value = load volatile i32, ptr inttoptr (i32 1 to ptr), align 1
	ret i32 %value
}

define i32 @load_maximum_constant_address() {
entry:
	%value = load volatile i32, ptr inttoptr (i32 2147483647 to ptr), align 1
	ret i32 %value
}

define i32 @load_long_displacement_maximum(ptr %base) {
entry:
	%address = getelementptr i8, ptr %base, i32 60
	%value = load volatile i32, ptr %address, align 4
	ret i32 %value
}

; CHECK-LABEL: load_long_displacement_maximum:
; CHECK: mov.l @(60,r4),r0

define i32 @load_long_displacement_expanded(ptr %base) {
entry:
	%address = getelementptr i8, ptr %base, i32 64
	%value = load volatile i32, ptr %address, align 4
	ret i32 %value
}

; CHECK-LABEL: load_long_displacement_expanded:
; CHECK-NOT: @(64,r4)
; CHECK: add #64,r4
; CHECK: mov.l @r4,r0
