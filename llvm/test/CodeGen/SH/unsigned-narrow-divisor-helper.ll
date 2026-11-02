; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,SINGLE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,SINGLE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,SINGLE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,SINGLE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2e -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,SINGLE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2e -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,SINGLE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3e -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,SINGLE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3e -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,SINGLE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,DUAL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,DUAL
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4a -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,DUAL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4a -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,DUAL
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4al-dsp -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,DUAL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4al-dsp -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,DUAL
; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -O3 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2 -relocation-model=pic -O3 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2a -O3 -verify-machineinstrs -filetype=obj %s -o /dev/null

; Only unsigned variable word-sized divisors on single-issue CPUs use the helper.
; Signed, full-width, constant, dual-issue and optnone lowering stay independent.

define i32 @udiv_narrow(i32 %a, i32 %b) {
; COMMON-LABEL: udiv_narrow:
; SINGLE-NOT: div1
; SINGLE: jsr
; SINGLE: .long	__udivsi3
; DUAL: div1
; DUAL-NOT: __udivsi3
; DUAL: rts
	%masked = and i32 %b, 32767
	%positive = add nuw nsw i32 %masked, 3
	%frozen = freeze i32 %positive
	%q = udiv i32 %a, %frozen
	ret i32 %q
}

define i32 @sdiv_narrow(i32 %a, i32 %b) {
; COMMON-LABEL: sdiv_narrow:
; COMMON: div1
; COMMON-NOT: __sdivsi3
; COMMON: rts
	%masked = and i32 %b, 65535
	%q = sdiv i32 %a, %masked
	ret i32 %q
}

define i32 @urem_narrow(i32 %a, i32 %b) {
; COMMON-LABEL: urem_narrow:
; SINGLE-NOT: div1
; SINGLE: jsr
; SINGLE: mul.l
; SINGLE: sub
; SINGLE: .long	__udivsi3
; DUAL: div1
; DUAL-NOT: __udivsi3
; DUAL: rts
	%masked = and i32 %b, 65535
	%r = urem i32 %a, %masked
	ret i32 %r
}

define i32 @srem_narrow(i32 %a, i32 %b) {
; COMMON-LABEL: srem_narrow:
; COMMON: div1
; COMMON-NOT: __sdivsi3
; COMMON: rts
	%masked = and i32 %b, 65535
	%r = srem i32 %a, %masked
	ret i32 %r
}

define i32 @udiv_full(i32 %a, i32 %b) {
; COMMON-LABEL: udiv_full:
; COMMON: div1
; COMMON-NOT: __udivsi3
; COMMON: rts
	%q = udiv i32 %a, %b
	ret i32 %q
}

define i32 @sdiv_full(i32 %a, i32 %b) {
; COMMON-LABEL: sdiv_full:
; COMMON: div1
; COMMON-NOT: __sdivsi3
; COMMON: rts
	%q = sdiv i32 %a, %b
	ret i32 %q
}

define i64 @udivrem_narrow(i32 %a, i32 %b) {
; COMMON-LABEL: udivrem_narrow:
; SINGLE-NOT: div1
; SINGLE-COUNT-1: jsr
; SINGLE: mul.l
; SINGLE: sub
; SINGLE: .long	__udivsi3
; DUAL: div1
; DUAL-NOT: __udivsi3
; DUAL: rts
	%masked = and i32 %b, 65535
	%q = udiv i32 %a, %masked
	%r = urem i32 %a, %masked
	%wide.q = zext i32 %q to i64
	%high = shl i64 %wide.q, 32
	%wide.r = zext i32 %r to i64
	%result = or i64 %high, %wide.r
	ret i64 %result
}

define i32 @sdiv_size(i32 %a, i32 %b) optsize {
; COMMON-LABEL: sdiv_size:
; COMMON: .long	__divsi3
	%masked = and i32 %b, 65535
	%q = sdiv i32 %a, %masked
	ret i32 %q
}

define i32 @udiv_17_bits(i32 %a, i32 %b) {
; COMMON-LABEL: udiv_17_bits:
; COMMON: div1
; COMMON-NOT: __udivsi3
; COMMON: rts
	%masked = and i32 %b, 131071
	%q = udiv i32 %a, %masked
	ret i32 %q
}

define i32 @udiv_constant(i32 %a) {
; COMMON-LABEL: udiv_constant:
; COMMON-NOT: jsr
; COMMON-NOT: __udivsi3
; COMMON: rts
	%q = udiv i32 %a, 3
	ret i32 %q
}

define i32 @udiv_optnone(i32 %a, i32 %b) noinline optnone {
; COMMON-LABEL: udiv_optnone:
; COMMON: div1
; COMMON-NOT: __udivsi3
; COMMON: rts
	%masked = and i32 %b, 65535
	%q = udiv i32 %a, %masked
	ret i32 %q
}
