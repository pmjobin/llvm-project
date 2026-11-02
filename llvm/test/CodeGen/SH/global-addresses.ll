; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

@global_i32 = global i32 42, align 4
@array = global [16 x i32] zeroinitializer, align 4
@clustered_array = global [32 x i32] zeroinitializer, align 4
@sparse_array = global [32770 x i8] zeroinitializer, align 1
@external_i32 = external global i32

define ptr @global_address() {
; CHECK-LABEL: global_address:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,r0
; CHECK: rts
; CHECK-NEXT: nop
; CHECK: .p2align	2
; CHECK-NEXT: .LCPI{{[0-9]+}}_0_0:
; CHECK-NEXT: .long	global_i32
	ret ptr @global_i32
}

define ptr @constant_gep() {
; CHECK-LABEL: constant_gep:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,r0
; CHECK-NOT: add
; CHECK: rts
; CHECK: .LCPI{{[0-9]+}}_0_0:
; CHECK-NEXT: .long	array+12
	ret ptr getelementptr ([16 x i32], ptr @array, i32 0, i32 3)
}

define ptr @external_global_address() {
; CHECK-LABEL: external_global_address:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,r0
; CHECK: .long	external_i32
	ret ptr @external_i32
}

define i32 @ptrtoint_i32() {
; CHECK-LABEL: ptrtoint_i32:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,r0
; CHECK: .long	global_i32
	ret i32 ptrtoint (ptr @global_i32 to i32)
}

define i64 @ptrtoint_i64() {
; CHECK-LABEL: ptrtoint_i64:
; CHECK-DAG: mov	#0,r{{[01]}}
; CHECK-DAG: mov.l	.LCPI{{[0-9]+}}_0_0,r{{[01]}}
; CHECK: .long	global_i32
	ret i64 ptrtoint (ptr @global_i32 to i64)
}

define ptr @inttoptr_i32(i32 %value) {
; CHECK-LABEL: inttoptr_i32:
; CHECK: rts
; CHECK-NEXT: mov	r4,r0
	%address = inttoptr i32 %value to ptr
	ret ptr %address
}

define ptr @truncated_inttoptr_i64(i64 %value) {
; CHECK-LABEL: truncated_inttoptr_i64:
; CHECK: rts
; CHECK-NEXT: mov	r{{[45]}},r0
	%truncated = trunc i64 %value to i32
	%address = inttoptr i32 %truncated to ptr
	ret ptr %address
}

define i32 @repeated_global_loads() {
; CHECK-LABEL: repeated_global_loads:
; CHECK-COUNT-1: mov.l	.LCPI{{[0-9]+}}_0_0,
; CHECK-COUNT-1: .LCPI{{[0-9]+}}_0_0:
; CHECK-NEXT: .long	global_i32
	%first = load volatile i32, ptr @global_i32, align 4
	%second = load volatile i32, ptr @global_i32, align 4
	%sum = add i32 %first, %second
	ret i32 %sum
}

define i32 @clustered_global_offsets() {
; CHECK-LABEL: clustered_global_offsets:
; CHECK-COUNT-1: .long	clustered_array
; CHECK-NOT: .long	clustered_array+
	%first = load volatile i32, ptr @clustered_array, align 4
	%second = load volatile i32, ptr getelementptr ([32 x i32], ptr @clustered_array, i32 0, i32 1), align 4
	%third = load volatile i32, ptr getelementptr ([32 x i32], ptr @clustered_array, i32 0, i32 2), align 4
	%pair = add i32 %first, %second
	%sum = add i32 %pair, %third
	ret i32 %sum
}

define i32 @sparse_global_offsets() {
; CHECK-LABEL: sparse_global_offsets:
; CHECK-DAG: .long	sparse_array
; CHECK-DAG: .long	sparse_array+1
; CHECK-DAG: .long	sparse_array+32769
	%first = load volatile i8, ptr @sparse_array, align 1
	%second = load volatile i8, ptr getelementptr ([32770 x i8], ptr @sparse_array, i32 0, i32 1), align 1
	%third = load volatile i8, ptr getelementptr ([32770 x i8], ptr @sparse_array, i32 0, i32 32769), align 1
	%first.ext = zext i8 %first to i32
	%second.ext = zext i8 %second to i32
	%third.ext = zext i8 %third to i32
	%pair = add i32 %first.ext, %second.ext
	%sum = add i32 %pair, %third.ext
	ret i32 %sum
}
