; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

@byte = global i8 -1, align 1
@word = global i16 -2, align 2
@long = global i32 42, align 4
@wide = global i64 1234605616436508552, align 4
@pointer = global ptr null, align 4
@external_long = external global i32

define i32 @load_i32() {
; CHECK-LABEL: load_i32:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,[[ADDR:r[0-9]+]]
; CHECK-NEXT: mov.l	@[[ADDR]],r0
; CHECK: .long	long
	%value = load i32, ptr @long, align 4
	ret i32 %value
}

define i32 @load_external_i32() {
; CHECK-LABEL: load_external_i32:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,[[ADDR:r[0-9]+]]
; CHECK-NEXT: mov.l	@[[ADDR]],r0
; CHECK: .long	external_long
	%value = load i32, ptr @external_long, align 4
	ret i32 %value
}

define void @store_i32(i32 %value) {
; CHECK-LABEL: store_i32:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,[[ADDR:r[0-9]+]]
; CHECK-NEXT: mov.l	r4,@[[ADDR]]
; CHECK: .long	long
	store i32 %value, ptr @long, align 4
	ret void
}

define i32 @load_i8_signed() {
; CHECK-LABEL: load_i8_signed:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,[[ADDR:r[0-9]+]]
; CHECK: mov.b	@[[ADDR]],
; CHECK: .long	byte
	%value = load i8, ptr @byte, align 1
	%result = sext i8 %value to i32
	ret i32 %result
}

define i32 @load_i8_unsigned() {
; CHECK-LABEL: load_i8_unsigned:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,[[ADDR:r[0-9]+]]
; CHECK: mov.b	@[[ADDR]],
; CHECK: extu.b
; CHECK: .long	byte
	%value = load i8, ptr @byte, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @load_i16_signed() {
; CHECK-LABEL: load_i16_signed:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,[[ADDR:r[0-9]+]]
; CHECK: mov.w	@[[ADDR]],
; CHECK: .long	word
	%value = load i16, ptr @word, align 2
	%result = sext i16 %value to i32
	ret i32 %result
}

define i32 @load_i16_unsigned() {
; CHECK-LABEL: load_i16_unsigned:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,[[ADDR:r[0-9]+]]
; CHECK: mov.w	@[[ADDR]],
; CHECK: .long	word
	%value = load i16, ptr @word, align 2
	%result = zext i16 %value to i32
	ret i32 %result
}

define void @store_narrow(i32 %value) {
; CHECK-LABEL: store_narrow:
; CHECK: mov.b
; CHECK: mov.w
; CHECK: .long	byte
; CHECK: .long	word
	%narrow8 = trunc i32 %value to i8
	%narrow16 = trunc i32 %value to i16
	store volatile i8 %narrow8, ptr @byte, align 1
	store volatile i16 %narrow16, ptr @word, align 2
	ret void
}

define i64 @load_i64() {
; CHECK-LABEL: load_i64:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,[[ADDR0:r[0-9]+]]
; CHECK: mov.l	@[[ADDR0]],
; CHECK: mov.l	.LCPI{{[0-9]+}}_1_0,[[ADDR1:r[0-9]+]]
; CHECK: mov.l	@[[ADDR1]],
; CHECK: .long	wide
; CHECK: .long	wide+4
	%value = load i64, ptr @wide, align 4
	ret i64 %value
}

define void @store_i64(i64 %value) {
; CHECK-LABEL: store_i64:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,[[ADDR0:r[0-9]+]]
; CHECK: mov.l	{{r[0-9]+}},@[[ADDR0]]
; CHECK: mov.l	.LCPI{{[0-9]+}}_1_0,[[ADDR1:r[0-9]+]]
; CHECK: mov.l	{{r[0-9]+}},@[[ADDR1]]
; CHECK: .long	wide+4
; CHECK: .long	wide{{$}}
	store i64 %value, ptr @wide, align 4
	ret void
}

define ptr @load_pointer() {
; CHECK-LABEL: load_pointer:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,[[ADDR:r[0-9]+]]
; CHECK-NEXT: mov.l	@[[ADDR]],r0
; CHECK: .long	pointer
	%value = load ptr, ptr @pointer, align 4
	ret ptr %value
}

define void @store_pointer(ptr %value) {
; CHECK-LABEL: store_pointer:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,[[ADDR:r[0-9]+]]
; CHECK: mov.l	r4,@[[ADDR]]
; CHECK: .long	pointer
	store ptr %value, ptr @pointer, align 4
	ret void
}
