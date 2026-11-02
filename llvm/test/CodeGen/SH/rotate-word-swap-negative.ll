; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s

declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i8 @llvm.fshl.i8(i8, i8, i8)
declare i16 @llvm.fshr.i16(i16, i16, i16)
declare i64 @llvm.fshl.i64(i64, i64, i64)

define i32 @two_inputs(i32 %x, i32 %y) {
; CHECK-LABEL: two_inputs:
; CHECK-NOT: swap.w
; CHECK: .Lfunc_end
	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %y, i32 7)
	ret i32 %r
}

define i32 @variable(i32 %x, i32 %count) {
; CHECK-LABEL: variable:
; CHECK-NOT: swap.w
; CHECK: .Lfunc_end
	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 %count)
	ret i32 %r
}

define i8 @narrow8(i8 %x) {
; CHECK-LABEL: narrow8:
; CHECK-NOT: swap.w
; CHECK: .Lfunc_end
	%r = call i8 @llvm.fshl.i8(i8 %x, i8 %x, i8 7)
	ret i8 %r
}

define i16 @narrow16(i16 %x) {
; CHECK-LABEL: narrow16:
; CHECK-NOT: swap.w
; CHECK: .Lfunc_end
	%r = call i16 @llvm.fshr.i16(i16 %x, i16 %x, i16 7)
	ret i16 %r
}

define i64 @wide64(i64 %x) {
; CHECK-LABEL: wide64:
; CHECK-NOT: swap.w
; CHECK: .Lfunc_end
	%r = call i64 @llvm.fshl.i64(i64 %x, i64 %x, i64 7)
	ret i64 %r
}
