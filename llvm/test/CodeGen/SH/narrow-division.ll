; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4a -O3 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4al-dsp -O3 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh4 -relocation-model=pic -O3 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh4 -relocation-model=pic -O3 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=O0 --implicit-check-not=xtrct
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=O0 --implicit-check-not=xtrct

; Eligibility is a nonzero unsigned-word value, independent of how it was
; constructed. Packing and the single final rotate must survive allocation.
define i32 @quotient_from_extension(i32 %n, i16 %input) {
; O0-LABEL: quotient_from_extension:
; O0: div0u
; O0: rotcl
; O0: div1
; CHECK-LABEL: quotient_from_extension:
; CHECK: div0u
; CHECK-NOT: rotcl
; CHECK-COUNT-16: div1
; CHECK-NOT: rotcl
; CHECK: xtrct
; CHECK: swap.w
; CHECK-COUNT-16: div1
; CHECK-NOT: div1
; CHECK: rotcl
; CHECK-NOT: div1
; CHECK-NOT: rotcl
	%extended = zext i16 %input to i32
	%d = or i32 %extended, 1
	%q = udiv i32 %n, %d
	ret i32 %q
}

; This addition is nonzero even without no-wrap flags. Moving freeze must not
; lose the semantic proof obtained from the operand bounds.
define i32 @quotient_bounded_add(i32 %n, i32 %input) {
; CHECK-LABEL: quotient_bounded_add:
; CHECK: div0u
; CHECK-COUNT-16: div1
; CHECK: xtrct
; CHECK-COUNT-16: div1
; CHECK-NOT: div1
; CHECK: rotcl
; CHECK-NOT: rotcl
	%bounded = lshr i32 %input, 17
	%sum = add i32 %bounded, 9
	%d = freeze i32 %sum
	%q = udiv i32 %n, %d
	ret i32 %q
}

define i32 @remainder_from_byte(i32 %n, i8 %input) {
; CHECK-LABEL: remainder_from_byte:
; CHECK: div0u
; CHECK-COUNT-16: div1
; CHECK: xtrct
; CHECK-COUNT-16: div1
; CHECK: rotcl
; CHECK-NOT: div1
; CHECK: mul.l
; CHECK: sub
	%extended = zext i8 %input to i32
	%d = add i32 %extended, 1
	%r = urem i32 %n, %d
	ret i32 %r
}

; Combined demand must still have one quotient engine, including divisors
; whose shifted representation has its high bit set.
define i64 @quotient_and_remainder(i32 %n, i32 %input) {
; CHECK-LABEL: quotient_and_remainder:
; CHECK: div0u
; CHECK-COUNT-16: div1
; CHECK: xtrct
; CHECK-COUNT-16: div1
; CHECK: rotcl
; CHECK-NOT: div0u
; CHECK-NOT: div1
; CHECK-NOT: rotcl
; CHECK: mul.l
; CHECK: sub
; CHECK-NOT: div1
	%bounded = lshr i32 %input, 17
	%d = add i32 %bounded, 32768
	%q = udiv i32 %n, %d
	%r = urem i32 %n, %d
	%wideq = zext i32 %q to i64
	%wider = zext i32 %r to i64
	%high = shl i64 %wideq, 32
	%result = or i64 %high, %wider
	ret i64 %result
}

define i32 @zero_possible(i32 %n, i16 %input) {
; CHECK-LABEL: zero_possible:
; CHECK-NOT: xtrct
; CHECK: div0u
; CHECK: rotcl
; CHECK: div1
; CHECK-NOT: xtrct
	%d = zext i16 %input to i32
	%q = udiv i32 %n, %d
	ret i32 %q
}

define i32 @unknown_range(i32 %n, i32 %d) {
; CHECK-LABEL: unknown_range:
; CHECK-NOT: xtrct
; CHECK: div0u
; CHECK: rotcl
; CHECK: div1
; CHECK-NOT: xtrct
	%q = udiv i32 %n, %d
	ret i32 %q
}

define i32 @full_width_nonzero(i32 %n, i32 %input) {
; CHECK-LABEL: full_width_nonzero:
; CHECK-NOT: xtrct
; CHECK: div0u
; CHECK: rotcl
; CHECK: div1
; CHECK-NOT: xtrct
	%d = or i32 %input, 65536
	%q = udiv i32 %n, %d
	ret i32 %q
}

define i32 @negative_denominator(i32 %n, i32 %input) {
; CHECK-LABEL: negative_denominator:
; CHECK-NOT: xtrct
; CHECK: div0s
; CHECK: rotcl
; CHECK: div1
; CHECK-NOT: xtrct
	%d = or i32 %input, -32768
	%q = sdiv i32 %n, %d
	ret i32 %q
}

; A narrow result of wrapping addition can be zero.
define i32 @wrapping_narrow_add(i32 %n, i16 %input) {
; CHECK-LABEL: wrapping_narrow_add:
; CHECK-NOT: xtrct
; CHECK: div0u
; CHECK: rotcl
; CHECK: div1
; CHECK-NOT: xtrct
	%sum = add i16 %input, 1
	%d = zext i16 %sum to i32
	%q = udiv i32 %n, %d
	ret i32 %q
}

; Signed positive divisors use the same unsigned magnitude engine, including
; the unsigned magnitude of INT32_MIN. There is no general DIV0S chain.
define i32 @signed_positive_quotient(i32 %n, i16 %input) {
; CHECK-LABEL: signed_positive_quotient:
; CHECK-NOT: div0s
; CHECK: div0u
; CHECK-COUNT-16: div1
; CHECK: xtrct
; CHECK-COUNT-16: div1
; CHECK: rotcl
; CHECK-NOT: div0s
; CHECK-NOT: div1
; CHECK-NOT: rotcl
	%extended = zext i16 %input to i32
	%d = or i32 %extended, 1
	%q = sdiv i32 %n, %d
	ret i32 %q
}

define i32 @signed_positive_remainder(i32 %n, i8 %input) {
; CHECK-LABEL: signed_positive_remainder:
; CHECK-NOT: div0s
; CHECK: div0u
; CHECK-COUNT-16: div1
; CHECK: xtrct
; CHECK-COUNT-16: div1
; CHECK: rotcl
; CHECK-NOT: div1
; CHECK: mul.l
; CHECK: sub
	%extended = zext i8 %input to i32
	%d = add i32 %extended, 1
	%r = srem i32 %n, %d
	ret i32 %r
}

define i64 @signed_positive_pair(i32 %n, i32 %input) {
; CHECK-LABEL: signed_positive_pair:
; CHECK-NOT: div0s
; CHECK: div0u
; CHECK-COUNT-16: div1
; CHECK: xtrct
; CHECK-COUNT-16: div1
; CHECK: rotcl
; CHECK-NOT: div0u
; CHECK-NOT: div1
; CHECK-NOT: rotcl
; CHECK: mul.l
; CHECK: sub
; CHECK-NOT: div1
	%bounded = lshr i32 %input, 17
	%d = add i32 %bounded, 32768
	%q = sdiv i32 %n, %d
	%r = srem i32 %n, %d
	%wideq = zext i32 %q to i64
	%wider = zext i32 %r to i64
	%high = shl i64 %wideq, 32
	%result = or i64 %high, %wider
	ret i64 %result
}

define i32 @signed_zero_possible(i32 %n, i16 %input) {
; CHECK-LABEL: signed_zero_possible:
; CHECK-NOT: xtrct
; CHECK: div0s
; CHECK: rotcl
; CHECK: div1
; CHECK-NOT: xtrct
	%d = zext i16 %input to i32
	%q = sdiv i32 %n, %d
	ret i32 %q
}

define i32 @signed_unknown_range(i32 %n, i32 %d) {
; CHECK-LABEL: signed_unknown_range:
; CHECK-NOT: xtrct
; CHECK: div0s
; CHECK: rotcl
; CHECK: div1
; CHECK-NOT: xtrct
	%q = sdiv i32 %n, %d
	ret i32 %q
}
