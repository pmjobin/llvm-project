; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=greedy %s -o - | FileCheck %s --check-prefix=RA

; ASM-NOT: __adddi3
; ASM-NOT: __subdi3
; ASM-NOT: __ashldi3
; ASM-NOT: __lshrdi3
; ASM-NOT: __ashrdi3
; ASM-NOT: .rodata

define internal i64 @pressure_identity(i64 %value) {
	ret i64 %value
}

define i64 @two_independent_additions(i64 %a, i64 %b) {
; ASM-LABEL: two_independent_additions:
; ASM: clrt
; ASM: addc
; ASM: addc
; ASM: clrt
; ASM: addc
; ASM: addc
	%first = add i64 %a, 1311768467463790320
	%second = add i64 %b, -2401053092593050265
	%result = xor i64 %first, %second
	ret i64 %result
}

define i64 @addition_adjacent_to_division(i64 %a, i32 %numerator, i32 %denominator) {
; ASM-LABEL: addition_adjacent_to_division:
; ASM: div0u
; ASM-COUNT-32: div1
; ASM: clrt
; ASM: addc
	%sum = add i64 %a, 4294967297
	%quotient = udiv i32 %numerator, %denominator
	%wide = zext i32 %quotient to i64
	%result = xor i64 %sum, %wide
	ret i64 %result
}

define i64 @subtraction_around_call(i64 %a, i64 %b) {
; ASM-LABEL: subtraction_around_call:
; ASM: clrt
; ASM: subc
; ASM: bsr	pressure_identity
; ASM: clrt
; ASM: subc
	%before = sub i64 %a, %b
	%called = call i64 @pressure_identity(i64 %before)
	%after = sub i64 %called, %a
	ret i64 %after
}

define i64 @shift_adjacent_to_comparison(i64 %value, i64 %count, i64 %limit) {
; ASM-LABEL: shift_adjacent_to_comparison:
; ASM: shlr
; ASM-NEXT: rotcr
; ASM-NEXT: dt
; ASM: cmp/eq
	%shifted = lshr i64 %value, %count
	%condition = icmp ult i64 %shifted, %limit
	br i1 %condition, label %less, label %not_less
less:
	ret i64 %shifted
not_less:
	ret i64 %limit
}

define i64 @live_pairs_across_call(ptr %base) {
; ASM-LABEL: live_pairs_across_call:
; ASM: sts.l	pr,@-r15
; ASM: bsr	pressure_identity
; ASM: lds.l	@r15+,pr
	%p1 = getelementptr i8, ptr %base, i32 8
	%p2 = getelementptr i8, ptr %base, i32 16
	%p3 = getelementptr i8, ptr %base, i32 24
	%p4 = getelementptr i8, ptr %base, i32 32
	%v0 = load i64, ptr %base, align 4
	%v1 = load i64, ptr %p1, align 4
	%v2 = load i64, ptr %p2, align 4
	%v3 = load i64, ptr %p3, align 4
	%v4 = load i64, ptr %p4, align 4
	%called = call i64 @pressure_identity(i64 %v0)
	%x1 = xor i64 %called, %v1
	%x2 = xor i64 %x1, %v2
	%x3 = xor i64 %x2, %v3
	%result = xor i64 %x3, %v4
	ret i64 %result
}

define i64 @indirect_pair_cycle(ptr %callee, i64 %a, i64 %b) {
; ASM-LABEL: indirect_pair_cycle:
; ASM: jsr	@
	%result = call i64 %callee(i64 %b, i64 %a)
	ret i64 %result
}

; ISEL-LABEL: name: two_independent_additions
; ISEL: CLRT
; ISEL: ADDC
; ISEL: ADDC
; ISEL: CLRT
; ISEL: ADDC
; ISEL: ADDC
; ISEL-LABEL: name: addition_adjacent_to_division
; ISEL: DIV0U
; ISEL-COUNT-32: DIV1
; ISEL: CLRT
; ISEL: ADDC
; ISEL-LABEL: name: subtraction_around_call
; ISEL: CLRT
; ISEL: SUBC
; ISEL: BSR
; ISEL: CLRT
; ISEL: SUBC
; ISEL-LABEL: name: shift_adjacent_to_comparison
; ISEL: SHLR
; ISEL-NEXT: ROTCR
; ISEL-NEXT: DT
; ISEL-NEXT: BF
; ISEL: CMP_EQ
; ISEL-NOT: BR_CC64_PSEUDO

; RA-NOT: %{{[0-9]+}}:gpr
