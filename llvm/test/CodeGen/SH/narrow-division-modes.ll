; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs %s -o - | FileCheck %s

define i32 @size_quotient(i32 %n, i16 %input) optsize {
; CHECK-LABEL: size_quotient:
; CHECK-NOT: xtrct
; CHECK-NOT: div1
; CHECK: __udivsi3
	%extended = zext i16 %input to i32
	%d = or i32 %extended, 1
	%q = udiv i32 %n, %d
	ret i32 %q
}

define i32 @minimum_quotient(i32 %n, i16 %input) minsize {
; CHECK-LABEL: minimum_quotient:
; CHECK-NOT: xtrct
; CHECK-NOT: div1
; CHECK: __udivsi3
	%extended = zext i16 %input to i32
	%d = or i32 %extended, 1
	%q = udiv i32 %n, %d
	ret i32 %q
}

define i32 @unoptimized_quotient(i32 %n, i16 %input) noinline optnone {
; CHECK-LABEL: unoptimized_quotient:
; CHECK-NOT: xtrct
; CHECK: div0u
; CHECK: rotcl
; CHECK: div1
; CHECK-NOT: xtrct
	%extended = zext i16 %input to i32
	%d = or i32 %extended, 1
	%q = udiv i32 %n, %d
	ret i32 %q
}

define i32 @size_signed_quotient(i32 %n, i16 %input) optsize {
; CHECK-LABEL: size_signed_quotient:
; CHECK-NOT: xtrct
; CHECK-NOT: div1
; CHECK: __divsi3
	%extended = zext i16 %input to i32
	%d = or i32 %extended, 1
	%q = sdiv i32 %n, %d
	ret i32 %q
}

define i32 @minimum_signed_remainder(i32 %n, i16 %input) minsize {
; CHECK-LABEL: minimum_signed_remainder:
; CHECK-NOT: xtrct
; CHECK-NOT: div1
; CHECK: __modsi3
	%extended = zext i16 %input to i32
	%d = or i32 %extended, 1
	%r = srem i32 %n, %d
	ret i32 %r
}

define i32 @unoptimized_signed_quotient(i32 %n, i16 %input) noinline optnone {
; CHECK-LABEL: unoptimized_signed_quotient:
; CHECK-NOT: xtrct
; CHECK: div0s
; CHECK: rotcl
; CHECK: div1
; CHECK-NOT: xtrct
	%extended = zext i16 %input to i32
	%d = or i32 %extended, 1
	%q = sdiv i32 %n, %d
	ret i32 %q
}
