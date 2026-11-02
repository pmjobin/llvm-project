; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=FAST,COMMON
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=FAST,COMMON
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CONTROL,COMMON
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CONTROL,COMMON
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CONTROL,COMMON

; Cost the GPR sequence, including copies and shift-count temporaries, against
; the multiply and its special-register result transfer. All arithmetic wraps.

; FAST-LABEL: zero:
; FAST-NOT: mul.l
; FAST: mov #0
define i32 @zero(i32 %x) {
	%result = mul i32 %x, 0
	ret i32 %result
}

; FAST-LABEL: one:
; FAST-NOT: mul.l
; FAST: mov r4
define i32 @one(i32 %x) {
	%result = mul i32 %x, 1
	ret i32 %result
}

; FAST-LABEL: negative_one:
; FAST-NOT: mul.l
; FAST: neg
define i32 @negative_one(i32 %x) {
	%result = mul i32 %x, -1
	ret i32 %result
}

; FAST-LABEL: power:
; FAST-NOT: mul.l
; FAST: shll8
define i32 @power(i32 %x) {
	%result = mul i32 %x, 256
	ret i32 %result
}

; FAST-LABEL: power_plus_one:
; FAST-NOT: mul.l
; FAST: shll8
define i32 @power_plus_one(i32 %x) {
	%result = mul i32 %x, 257
	ret i32 %result
}

; FAST-LABEL: power_minus_one:
; FAST-NOT: mul.l
; FAST: shll8
define i32 @power_minus_one(i32 %x) {
	%result = mul i32 %x, 255
	ret i32 %result
}

; FAST-LABEL: sparse:
; FAST-NOT: mul.l
; FAST: shll2
define i32 @sparse(i32 %x) {
	%result = mul i32 %x, 17
	ret i32 %result
}

; FAST-LABEL: speed_33:
; FAST-NOT: mul.l
; FAST: shld
; CONTROL-LABEL: speed_33:
; CONTROL: mul.l
define i32 @speed_33(i32 %x) {
	%result = mul i32 %x, 33
	ret i32 %result
}

; FAST-LABEL: negative:
; FAST-NOT: mul.l
; FAST: sub
define i32 @negative(i32 %x) {
	%result = mul i32 %x, -7
	ret i32 %result
}

; FAST-LABEL: int_min:
; FAST-NOT: mul.l
; FAST: shld
define i32 @int_min(i32 %x) {
	%result = mul i32 %x, -2147483648
	ret i32 %result
}

; FAST-LABEL: int_max:
; FAST-NOT: mul.l
; FAST: shld
define i32 @int_max(i32 %x) {
	%result = mul i32 %x, 2147483647
	ret i32 %result
}

; COMMON-LABEL: dense:
; COMMON: mul.l
define i32 @dense(i32 %x) {
	%result = mul i32 %x, 1431655765
	ret i32 %result
}

; COMMON-LABEL: nontrivial:
; COMMON: mul.l
define i32 @nontrivial(i32 %x) {
	%result = mul i32 %x, 16777619
	ret i32 %result
}

; COMMON-LABEL: unprofitable_negative:
; COMMON: mul.l
define i32 @unprofitable_negative(i32 %x) {
	%result = mul i32 %x, -33
	ret i32 %result
}

; COMMON-LABEL: two_shift_inputs:
; COMMON: mul.l
define i32 @two_shift_inputs(i32 %x) {
	%result = mul i32 %x, 10
	ret i32 %result
}

; COMMON-LABEL: size:
; COMMON: mul.l
define i32 @size(i32 %x) optsize {
	%result = mul i32 %x, 33
	ret i32 %result
}

; COMMON-LABEL: minimum_size:
; COMMON: mul.l
define i32 @minimum_size(i32 %x) minsize {
	%result = mul i32 %x, 33
	ret i32 %result
}

; COMMON-LABEL: no_optimization:
; COMMON: mul.l
define i32 @no_optimization(i32 %x) noinline optnone {
	%result = mul i32 %x, 33
	ret i32 %result
}

; A compare result can stay in T across SHLD and ADD, which preserve T.
; FAST-LABEL: compare_before:
; FAST: cmp/hs
; FAST: shld
; FAST-NOT: shll{{[ \t]}}
; FAST: bt
; FAST-NOT: mul.l
; FAST: rts
define i32 @compare_before(i32 %x, i32 %a, i32 %b) {
	%condition = icmp ult i32 %a, %b
	%product = mul i32 %x, 33
	br i1 %condition, label %yes, label %no
yes:
	%sum = add i32 %product, %a
	ret i32 %sum
no:
	%difference = xor i32 %product, %b
	ret i32 %difference
}

; A one-bit shift defines T. Its compare/branch dependency must stay intact.
; FAST-LABEL: compare_after:
; FAST-NOT: mul.l
; FAST: cmp/hs
; FAST: bt
define i32 @compare_after(i32 %x, i32 %a, i32 %b) {
	%product = mul i32 %x, 7
	%condition = icmp ult i32 %product, %a
	br i1 %condition, label %yes, label %no
yes:
	ret i32 %product
no:
	ret i32 %b
}

; Explicit asm owns its MAC and T effects. The ordinary multiply may use GPRs.
; FAST-LABEL: asm_state:
; FAST: cmp/eq
; FAST: sts macl
; FAST: shld
; FAST-NOT: mul.l
; FAST: rts
define i32 @asm_state(i32 %x, i32 %a) {
	%value = call i32 asm sideeffect "cmp/eq $1,$1; sts macl,$0", "=r,r,~{t},~{macl},~{mach}"(i32 %a)
	%product = mul i32 %value, 33
	%result = add i32 %product, %x
	ret i32 %result
}
