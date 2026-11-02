; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O1 -verify-machineinstrs -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=SELECT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=SELECT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=SELECT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=NATIVE
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=NATIVE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=OLD
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O2 -verify-machineinstrs -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=OLD
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=OLD
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -stop-before=finalize-isel < %s | FileCheck %s --check-prefix=OLD

; OLD: name: left
; OLD-NOT: _SMALL_PSEUDO

define i32 @left(i32 %x, i32 %raw) {
; SELECT-LABEL: name: left
; SELECT: SHL_SMALL_PSEUDO {{.*}}, 0, 24
; NATIVE-LABEL: left:
; NATIVE-NOT: __ashlsi3
; NATIVE: tst
; NATIVE: shll16
; NATIVE: tst
; NATIVE: shll8
	%c = and i32 %raw, 24
	%v = shl i32 %x, %c
	ret i32 %v
}

define i32 @right(i32 %x, i32 %raw) {
; SELECT-LABEL: name: right
; SELECT: SRL_SMALL_PSEUDO {{.*}}, 0, 24
; NATIVE-LABEL: right:
; NATIVE-NOT: __lshrsi3
; NATIVE: tst
; NATIVE: shlr16
; NATIVE: tst
; NATIVE: shlr8
	%c = and i32 %raw, 24
	%v = lshr i32 %x, %c
	ret i32 %v
}

define i32 @fixed_and_unknown(i32 %x, i32 %raw) {
; SELECT-LABEL: name: fixed_and_unknown
; SELECT: SHL_SMALL_PSEUDO {{.*}}, 8, 2
	%m = and i32 %raw, 2
	%c = or i32 %m, 8
	%v = shl i32 %x, %c
	ret i32 %v
}

define i32 @live_source_count(i32 %x, i32 %raw, ptr %out) {
; SELECT-LABEL: name: live_source_count
; SELECT: SRL_SMALL_PSEUDO {{.*}}, 8, 2
	%m = and i32 %raw, 2
	%c = or i32 %m, 8
	%v = lshr i32 %x, %c
	%sum = add i32 %x, %c
	store i32 %sum, ptr %out
	ret i32 %v
}

define i32 @frozen_count(i32 %x, i32 %raw) {
; SELECT-LABEL: name: frozen_count
; SELECT: SHL_SMALL_PSEUDO {{.*}}, 0, 24
	%f = freeze i32 %raw
	%c = and i32 %f, 24
	%v = shl i32 %x, %c
	ret i32 %v
}

define i32 @compare_live(i32 %x, i32 %raw, i32 %a, i32 %b) {
; SELECT-LABEL: name: compare_live
; SELECT: SHL_SMALL_PSEUDO {{.*}}, 0, 24
	%flag = icmp ult i32 %a, %b
	%c = and i32 %raw, 24
	%v = shl i32 %x, %c
	%r = select i1 %flag, i32 %v, i32 %a
	ret i32 %r
}

define i32 @larger_domain(i32 %x, i32 %raw) {
; SELECT-LABEL: name: larger_domain
; SELECT-NOT: _SMALL_PSEUDO
; SELECT: JSR
	%c = and i32 %raw, 7
	%v = shl i32 %x, %c
	ret i32 %v
}

define i32 @unbounded(i32 %x, i32 %raw) {
; SELECT-LABEL: name: unbounded
; SELECT-NOT: _SMALL_PSEUDO
; SELECT: JSR
	%v = lshr i32 %x, %raw
	ret i32 %v
}

define i32 @outside_width(i32 %x, i32 %raw) {
; SELECT-LABEL: name: outside_width
; SELECT-NOT: _SMALL_PSEUDO
; SELECT: JSR
	%c = and i32 %raw, 48
	%v = shl i32 %x, %c
	ret i32 %v
}

define i32 @cost_fallback(i32 %x, i32 %raw) {
; SELECT-LABEL: name: cost_fallback
; SELECT-NOT: _SMALL_PSEUDO
; SELECT: JSR
	%m = and i32 %raw, 16
	%c = or i32 %m, 15
	%v = shl i32 %x, %c
	ret i32 %v
}

define i32 @arithmetic(i32 %x, i32 %raw) {
; SELECT-LABEL: name: arithmetic
; SELECT-NOT: _SMALL_PSEUDO
; SELECT: JSR
	%c = and i32 %raw, 24
	%v = ashr i32 %x, %c
	ret i32 %v
}

define i32 @size(i32 %x, i32 %raw) optsize {
; SELECT-LABEL: name: size
; SELECT-NOT: _SMALL_PSEUDO
; SELECT: JSR
	%c = and i32 %raw, 24
	%v = shl i32 %x, %c
	ret i32 %v
}

define i32 @minimum(i32 %x, i32 %raw) minsize {
; SELECT-LABEL: name: minimum
; SELECT-NOT: _SMALL_PSEUDO
; SELECT: SHLrr
	%c = and i32 %raw, 24
	%v = shl i32 %x, %c
	ret i32 %v
}

define i32 @none(i32 %x, i32 %raw) noinline optnone {
; SELECT-LABEL: name: none
; SELECT-NOT: _SMALL_PSEUDO
; SELECT: SHLrr
	%c = and i32 %raw, 24
	%v = shl i32 %x, %c
	ret i32 %v
}

define i32 @unused_poison(i32 %x, i32 %raw, i1 %take) {
	%c = and i32 %raw, 24
	%v = shl nuw i32 %x, %c
	%r = select i1 %take, i32 %v, i32 73
	ret i32 %r
}

declare void @clobber()
define i32 @frozen_result(i32 %x, i32 %raw, ptr %out) {
	%c = and i32 %raw, 24
	%v = shl nuw i32 %x, %c
	%f = freeze i32 %v
	store volatile i32 %f, ptr %out
	call void @clobber()
	ret i32 %f
}

define i32 @undef_source(i32 %raw) {
	%c = and i32 %raw, 2
	%v = lshr i32 undef, %c
	%f = freeze i32 %v
	ret i32 %f
}
