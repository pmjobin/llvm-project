; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -code-model=small -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -code-model=small -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -code-model=large -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=LARGE
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -code-model=large -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=LARGE
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -filetype=null < %s
; RUN: opt -passes='default<O2>' -force-attribute=minsize < %s | llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -filetype=null

define i32 @dense_zero(i32 %x) {
entry:
	switch i32 %x, label %default [
		i32 0, label %c0
		i32 1, label %c1
		i32 2, label %c2
		i32 3, label %c3
		i32 4, label %c4
		i32 5, label %c5
	]
c0:
	ret i32 11
c1:
	ret i32 23
c2:
	ret i32 37
c3:
	ret i32 53
c4:
	ret i32 71
c5:
	ret i32 89
default:
	ret i32 -1
}

define i32 @dense_positive_minimum(i32 %x) {
entry:
	switch i32 %x, label %default [
		i32 10, label %c10
		i32 11, label %c11
		i32 12, label %c12
		i32 13, label %c13
		i32 14, label %c14
		i32 15, label %c15
	]
c10:
	ret i32 101
c11:
	ret i32 113
c12:
	ret i32 127
c13:
	ret i32 143
c14:
	ret i32 161
c15:
	ret i32 181
default:
	ret i32 -1
}

define i32 @dense_negative_holes(i32 %x) {
entry:
	switch i32 %x, label %default [
		i32 -4, label %low
		i32 -3, label %repeated
		i32 -1, label %repeated
		i32 0, label %zero
		i32 2, label %high
		i32 3, label %top
	]
low:
	ret i32 13
repeated:
	ret i32 29
zero:
	ret i32 47
high:
	ret i32 67
top:
	ret i32 97
default:
	ret i32 -1
}

define i32 @dense_i8(i32 %x) {
entry:
	%narrow = trunc i32 %x to i8
	switch i8 %narrow, label %default [
		i8 0, label %c0
		i8 1, label %c1
		i8 2, label %c2
		i8 3, label %c3
		i8 4, label %c4
		i8 5, label %c5
	]
c0:
	ret i32 10
c1:
	ret i32 22
c2:
	ret i32 35
c3:
	ret i32 49
c4:
	ret i32 64
c5:
	ret i32 80
default:
	ret i32 -1
}

define i32 @dense_i16(i32 %x) {
entry:
	%narrow = trunc i32 %x to i16
	switch i16 %narrow, label %default [
		i16 1000, label %c0
		i16 1001, label %c1
		i16 1002, label %c2
		i16 1003, label %c3
		i16 1004, label %c4
		i16 1005, label %c5
	]
c0:
	ret i32 7
c1:
	ret i32 19
c2:
	ret i32 31
c3:
	ret i32 43
c4:
	ret i32 61
c5:
	ret i32 79
default:
	ret i32 -1
}

define i32 @dense_signed_boundary(i32 %x) {
entry:
	switch i32 %x, label %default [
		i32 2147483642, label %c0
		i32 2147483643, label %c1
		i32 2147483644, label %c2
		i32 2147483645, label %c3
		i32 2147483646, label %c4
		i32 2147483647, label %c5
	]
c0:
	ret i32 5
c1:
	ret i32 17
c2:
	ret i32 41
c3:
	ret i32 59
c4:
	ret i32 73
c5:
	ret i32 101
default:
	ret i32 -1
}

; Zero-based normalization checks x > 5 before dispatch.
; ASM-LABEL: dense_zero:
; ASM: mov	#5,[[LIMIT:r[0-9]+]]
; ASM: cmp/hi	[[LIMIT]],[[INDEX:r[0-9]+]]
; ASM: bt	[[DEFAULT:.LBB[0-9_]+]]
; ASM: mov.l	[[CPI:.LCPI[0-9_]+]],[[BASE:r[0-9]+]]
; ASM: shll2
; ASM: mov.l	@
; ASM: jmp	@
; ASM-NEXT: nop
; ASM: [[CPI]]:
; ASM-NEXT: .long	[[JTI:.LJTI[0-9_]+]]
; ASM: .size	dense_zero,
; ASM: .section	.rodata
; ASM: [[JTI]]:
; ASM-COUNT-6: .long	.LBB

; Positive minima are subtracted before the same unsigned range check.
; ASM-LABEL: dense_positive_minimum:
; ASM: add	#-10,[[POSINDEX:r[0-9]+]]
; ASM: mov	#5,[[POSLIMIT:r[0-9]+]]
; ASM: cmp/hi	[[POSLIMIT]],[[POSINDEX]]
; ASM: bt
; ASM: jmp	@
; ASM-NEXT: nop

; The range -4 through 3 has eight entries. Indices 2 and 6 are holes that
; use the default block; -3 and -1 deliberately repeat one destination.
; ASM-LABEL: dense_negative_holes:
; ASM: add	#4,[[NEGINDEX:r[0-9]+]]
; ASM: mov	#7,[[NEGLIMIT:r[0-9]+]]
; ASM: cmp/hi	[[NEGLIMIT]],[[NEGINDEX]]
; ASM: bt
; ASM: jmp	@
; ASM-NEXT: nop
; ASM: [[NEGJTI:.LJTI[0-9_]+]]:
; ASM-COUNT-8: .long	.LBB

; Legally promoted narrow switches still use an i32 GPR table index.
; ASM-LABEL: dense_i8:
; ASM: extu.b
; ASM: cmp/hi
; ASM: shll2
; ASM: jmp	@
; ASM-NEXT: nop
; ASM-LABEL: dense_i16:
; ASM: extu.w
; ASM: cmp/hi
; ASM: shll2
; ASM: jmp	@
; ASM-NEXT: nop

; Normalization at the signed upper boundary remains modulo-32-bit pointer
; arithmetic followed by an unsigned bounds check.
; ASM-LABEL: dense_signed_boundary:
; ASM: cmp/hi
; ASM: shll2
; ASM: jmp	@
; ASM-NEXT: nop

; LARGE-LABEL: dense_zero:
; LARGE: mov.l	.LCPI
; LARGE: shll2
; LARGE: jmp	@
; LARGE-NEXT: nop
; LARGE-LABEL: dense_signed_boundary:
; LARGE: jmp	@
; LARGE-NEXT: nop
