; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define internal i32 @divide_identity(i32 %value) noinline {
	ret i32 %value
}

define i32 @two_unsigned_divisions(i32 %a, i32 %b, i32 %c) {
; CHECK-LABEL: two_unsigned_divisions:
; CHECK: div0u
; CHECK-COUNT-32: div1
; CHECK: rotcl
; CHECK-NEXT: div0u
; CHECK-COUNT-32: div1
; CHECK: rotcl
; CHECK-NOT: div0u
; CHECK-NOT: div1
; CHECK: rts
	%first = udiv i32 %a, %b
	%second = udiv i32 %a, %c
	%result = add i32 %first, %second
	ret i32 %result
}

define i32 @unsigned_then_signed(i32 %a, i32 %b, i32 %c) {
; CHECK-LABEL: unsigned_then_signed:
; CHECK: div0s
; CHECK-COUNT-32: div1
; CHECK: rotcl
; CHECK-NEXT: addc
; CHECK: div0u
; CHECK-COUNT-32: div1
; CHECK: rotcl
; CHECK-NOT: div0u
; CHECK-NOT: div0s
; CHECK: rts
	%first = udiv i32 %a, %b
	%second = sdiv i32 %a, %c
	%result = xor i32 %first, %second
	ret i32 %result
}

define i32 @division_around_call(i32 %a, i32 %b, i32 %c) {
; CHECK-LABEL: division_around_call:
; CHECK: div0u
; CHECK-COUNT-32: div1
; CHECK: rotcl
; CHECK: bsr	divide_identity
; CHECK-NEXT: nop
; CHECK: div0s
; CHECK-COUNT-32: div1
; CHECK: rotcl
; CHECK-NEXT: addc
; CHECK: rts
	%before = udiv i32 %a, %b
	%called = call i32 @divide_identity(i32 %before)
	%after = sdiv i32 %called, %c
	ret i32 %after
}

define i32 @division_with_shift_and_compare(i32 %a, i32 %b, i32 %count) {
; CHECK-LABEL: division_with_shift_and_compare:
; CHECK: div0u
; CHECK-COUNT-32: div1
; CHECK: rotcl
; CHECK-NOT: dt
; CHECK-NOT: cmp/
; CHECK: tst
; CHECK: shll
; CHECK-NEXT: dt
; CHECK-NEXT: bf
; CHECK: cmp/eq
	%quotient = udiv i32 %a, %b
	%shifted = shl i32 %quotient, %count
	%equal = icmp eq i32 %shifted, %a
	br i1 %equal, label %same, label %different

same:
	ret i32 %shifted

different:
	ret i32 %a
}

define i32 @division_pressure(ptr %base, i32 %dividend, i32 %divisor) {
; CHECK-LABEL: division_pressure:
; CHECK: add	#-
; CHECK: mov.l
; CHECK: div0u
; CHECK-COUNT-32: div1
; CHECK: mul.l
; CHECK-NEXT: sts	macl,
; CHECK: mov.l	@r15,
; CHECK: rts
	%p0 = getelementptr i8, ptr %base, i32 0
	%p1 = getelementptr i8, ptr %base, i32 4
	%p2 = getelementptr i8, ptr %base, i32 8
	%p3 = getelementptr i8, ptr %base, i32 12
	%p4 = getelementptr i8, ptr %base, i32 16
	%p5 = getelementptr i8, ptr %base, i32 20
	%p6 = getelementptr i8, ptr %base, i32 24
	%p7 = getelementptr i8, ptr %base, i32 28
	%p8 = getelementptr i8, ptr %base, i32 32
	%p9 = getelementptr i8, ptr %base, i32 36
	%p10 = getelementptr i8, ptr %base, i32 40
	%p11 = getelementptr i8, ptr %base, i32 44
	%v0 = load volatile i32, ptr %p0, align 4
	%v1 = load volatile i32, ptr %p1, align 4
	%v2 = load volatile i32, ptr %p2, align 4
	%v3 = load volatile i32, ptr %p3, align 4
	%v4 = load volatile i32, ptr %p4, align 4
	%v5 = load volatile i32, ptr %p5, align 4
	%v6 = load volatile i32, ptr %p6, align 4
	%v7 = load volatile i32, ptr %p7, align 4
	%v8 = load volatile i32, ptr %p8, align 4
	%v9 = load volatile i32, ptr %p9, align 4
	%v10 = load volatile i32, ptr %p10, align 4
	%v11 = load volatile i32, ptr %p11, align 4
	%s0 = add i32 %v0, %v1
	%s1 = add i32 %s0, %v2
	%s2 = add i32 %s1, %v3
	%s3 = add i32 %s2, %v4
	%s4 = add i32 %s3, %v5
	%s5 = add i32 %s4, %v6
	%s6 = add i32 %s5, %v7
	%s7 = add i32 %s6, %v8
	%s8 = add i32 %s7, %v9
	%s9 = add i32 %s8, %v10
	%s10 = add i32 %s9, %v11
	%mixed = xor i32 %dividend, %s10
	%quotient = udiv i32 %mixed, %divisor
	%remainder = urem i32 %mixed, %divisor
	%x0 = xor i32 %v0, %quotient
	%x1 = xor i32 %v1, %quotient
	%x2 = xor i32 %v2, %quotient
	%x3 = xor i32 %v3, %quotient
	%x4 = xor i32 %v4, %quotient
	%x5 = xor i32 %v5, %quotient
	%x6 = xor i32 %v6, %quotient
	%x7 = xor i32 %v7, %quotient
	%x8 = xor i32 %v8, %quotient
	%x9 = xor i32 %v9, %quotient
	%x10 = xor i32 %v10, %quotient
	%x11 = xor i32 %v11, %quotient
	%r0 = add i32 %x0, %x1
	%r1 = add i32 %r0, %x2
	%r2 = add i32 %r1, %x3
	%r3 = add i32 %r2, %x4
	%r4 = add i32 %r3, %x5
	%r5 = add i32 %r4, %x6
	%r6 = add i32 %r5, %x7
	%r7 = add i32 %r6, %x8
	%r8 = add i32 %r7, %x9
	%r9 = add i32 %r8, %x10
	%r10 = add i32 %r9, %x11
	%result = add i32 %r10, %remainder
	ret i32 %result
}
