; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s

define internal i32 @pressure_identity(i32 %value) noinline {
	ret i32 %value
}

define i32 @integer_pressure(ptr %base, i32 %value, i32 %count) {
; CHECK-LABEL: integer_pressure:
; CHECK: sts.l	pr,@-r15
; CHECK: add	#-{{[1-5][0-9]}},r15
; CHECK: mov	#-34,
; CHECK: shll8
; CHECK: mov	#18,
; CHECK: shll8
; CHECK: xor
; CHECK: mov	#31,
; CHECK: and
; CHECK: tst
; CHECK: shll
; CHECK-NEXT: dt
; CHECK-NEXT: bf
; CHECK: bsr	pressure_identity
; CHECK-NEXT: nop
; CHECK: or
; CHECK: mov.l	@r15,
; CHECK-NEXT: xor
; CHECK: lds.l	@r15+,pr
	%p0 = getelementptr i8, ptr %base, i32 0
	%p1 = getelementptr i8, ptr %base, i32 4
	%p2 = getelementptr i8, ptr %base, i32 8
	%p3 = getelementptr i8, ptr %base, i32 12
	%p4 = getelementptr i8, ptr %base, i32 16
	%p5 = getelementptr i8, ptr %base, i32 20
	%p6 = getelementptr i8, ptr %base, i32 24
	%p7 = getelementptr i8, ptr %base, i32 28
	%v0 = load volatile i32, ptr %p0, align 4
	%v1 = load volatile i32, ptr %p1, align 4
	%v2 = load volatile i32, ptr %p2, align 4
	%v3 = load volatile i32, ptr %p3, align 4
	%v4 = load volatile i32, ptr %p4, align 4
	%v5 = load volatile i32, ptr %p5, align 4
	%v6 = load volatile i32, ptr %p6, align 4
	%v7 = load volatile i32, ptr %p7, align 4
	%logical0 = xor i32 %v0, 305419896
	%logical1 = or i32 %v1, -559038737
	%logical2 = and i32 %v2, 2147483647
	%shifted = shl i32 %value, %count
	%called = call i32 @pressure_identity(i32 %shifted)
	%s0 = add i32 %called, %logical0
	%s1 = add i32 %s0, %logical1
	%s2 = add i32 %s1, %logical2
	%s3 = sub i32 %s2, %v3
	%s4 = xor i32 %s3, %v4
	%s5 = or i32 %s4, %v5
	%s6 = add i32 %s5, %v6
	%result = xor i32 %s6, %v7
	ret i32 %result
}
