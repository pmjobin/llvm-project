; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -stop-before=finalize-isel < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -verify-machine-dom-info -verify-loop-info < %s > /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -verify-machine-dom-info -verify-loop-info < %s > /dev/null

define i32 @source_is_count(i32 %raw, ptr %out) {
; CHECK-LABEL: name: source_is_count
; CHECK: SHL_SMALL_PSEUDO {{.*}}, 0, 2
	%c = and i32 %raw, 2
	%v = shl i32 %c, %c
	store volatile i32 %c, ptr %out
	ret i32 %v
}

; The count cube includes 2 and 16 as well as the actual values 0 and 18.
define i32 @correlated_count(i32 %x, i32 %raw) {
; CHECK-LABEL: name: correlated_count
; CHECK: SRL_SMALL_PSEUDO {{.*}}, 0, 18
	%low = and i32 %raw, 2
	%high = shl i32 %low, 3
	%c = or i32 %low, %high
	%v = lshr i32 %x, %c
	ret i32 %v
}

; A count's range alone does not prove that branching on it is supported.
define i32 @poison_count_unselected(i32 %x, i32 %raw, i1 %take) {
; CHECK-LABEL: name: poison_count_unselected
; CHECK-NOT: _SMALL_PSEUDO
; CHECK: JSR
	%p = add nsw i32 %raw, 1
	%c = and i32 %p, 24
	%v = shl i32 %x, %c
	%r = select i1 %take, i32 %v, i32 73
	ret i32 %r
}

declare void @clobber()

define i32 @stable_frozen_count(i32 %x, i32 %raw, ptr %out) {
; CHECK-LABEL: name: stable_frozen_count
; CHECK: SRL_SMALL_PSEUDO {{.*}}, 8, 2
	%p = add nsw i32 %raw, 1
	%f = freeze i32 %p
	%m = and i32 %f, 2
	%c = or i32 %m, 8
	%v = lshr i32 %x, %c
	store volatile i32 %c, ptr %out
	call void @clobber()
	%r = xor i32 %v, %c
	ret i32 %r
}

define i32 @stable_frozen_result(i32 %x, i32 %raw, ptr %out) {
; CHECK-LABEL: name: stable_frozen_result
; CHECK: SHL_SMALL_PSEUDO {{.*}}, 0, 24
	%c = and i32 %raw, 24
	%v = shl nuw i32 %x, %c
	%f = freeze i32 %v
	store volatile i32 %f, ptr %out
	call void @clobber()
	ret i32 %f
}

define i64 @carry_and_shift(i32 %raw, i64 %a, i64 %b) {
; CHECK-LABEL: name: carry_and_shift
; CHECK: SHL_SMALL_PSEUDO {{.*}}, 8, 2
	%x = trunc i64 %a to i32
	%m = and i32 %raw, 2
	%c = or i32 %m, 8
	%v = shl i32 %x, %c
	%sum = add i64 %a, %b
	%z = zext i32 %v to i64
	%r = xor i64 %sum, %z
	ret i64 %r
}

define i64 @borrow_and_shift(i32 %raw, i64 %a, i64 %b) {
; CHECK-LABEL: name: borrow_and_shift
; CHECK: SRL_SMALL_PSEUDO {{.*}}, 8, 2
	%x = trunc i64 %a to i32
	%m = and i32 %raw, 2
	%c = or i32 %m, 8
	%v = lshr i32 %x, %c
	%difference = sub i64 %a, %b
	%z = zext i32 %v to i64
	%r = xor i64 %difference, %z
	ret i64 %r
}

define i32 @comparison_and_effects(i32 %x, i32 %raw, i32 %a, i32 %b, ptr %out) {
; CHECK-LABEL: name: comparison_and_effects
; CHECK: SHL_SMALL_PSEUDO {{.*}}, 0, 24
	%flag = icmp ult i32 %a, %b
	%word = zext i1 %flag to i32
	store volatile i32 %word, ptr %out
	%c = and i32 %raw, 24
	%v = shl i32 %x, %c
	%r = select i1 %flag, i32 %v, i32 73
	store volatile i32 %r, ptr %out
	ret i32 %r
}

define i32 @frozen_undef_source(i32 %raw, ptr %out) {
	%c = and i32 %raw, 2
	%v = lshr i32 undef, %c
	%f = freeze i32 %v
	store volatile i32 %f, ptr %out
	call void @clobber()
	ret i32 %f
}

define i32 @unselected_undef_count(i32 %x) {
	%c = and i32 undef, 24
	%v = shl i32 %x, %c
	%r = select i1 false, i32 %v, i32 73
	ret i32 %r
}
