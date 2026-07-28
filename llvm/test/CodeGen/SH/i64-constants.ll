; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,LE

; COMMON-NOT: .long
; COMMON-NOT: .quad
; COMMON-NOT: .rodata
; COMMON-NOT: __

define i64 @constant_zero() {
; COMMON-LABEL: constant_zero:
; COMMON: mov	#0,
; COMMON-NEXT: mov
; COMMON: rts
	ret i64 0
}

define i64 @constant_one() {
; COMMON-LABEL: constant_one:
; BE: mov	#0,r0
; BE-NEXT: mov	#1,r1
; LE: mov	#1,r0
; LE-NEXT: mov	#0,r1
	ret i64 1
}

define i64 @constant_low_ones() {
; COMMON-LABEL: constant_low_ones:
; BE: mov	#0,r0
; BE-NEXT: mov	#-1,r1
; LE: mov	#-1,r0
; LE-NEXT: mov	#0,r1
	ret i64 4294967295
}

define i64 @constant_high_one() {
; COMMON-LABEL: constant_high_one:
; BE: mov	#1,r0
; BE-NEXT: mov	#0,r1
; LE: mov	#0,r0
; LE-NEXT: mov	#1,r1
	ret i64 4294967296
}

define i64 @constant_pattern() {
; COMMON-LABEL: constant_pattern:
; BE: mov	#18,
; BE: mov	#120,
; BE: mov	#-102,
; BE: mov	#-16,
; LE: mov	#-102,
; LE: mov	#-16,
; LE: mov	#18,
; LE: mov	#120,
; COMMON: rts
	ret i64 1311768467463790320
}

define i64 @constant_minimum() {
; COMMON-LABEL: constant_minimum:
; BE: mov	#-128,
; BE: shll8
; BE: mov	#0,
; LE: mov	#0,
; LE: mov	#-128,
; LE: shll8
; COMMON: rts
	ret i64 -9223372036854775808
}

define i64 @constant_negative_one() {
; COMMON-LABEL: constant_negative_one:
; COMMON: mov	#-1,
; COMMON-NEXT: mov
; COMMON: rts
	ret i64 -1
}

define i64 @constant_deadbeef() {
; COMMON-LABEL: constant_deadbeef:
; BE: mov	#-34,
; BE: mov	#-17,
; BE: mov	#1,
; BE: mov	#103,
; LE: mov	#1,
; LE: mov	#103,
; LE: mov	#-34,
; LE: mov	#-17,
; COMMON: rts
	ret i64 -2401053092593050265
}

define i32 @unused_high_constant_half() {
; COMMON-LABEL: unused_high_constant_half:
; COMMON-NOT: mov	#-34,
; COMMON: mov	#1,
; COMMON: mov	#103,
; COMMON: rts
	%low = trunc i64 -2401053092593050265 to i32
	ret i32 %low
}
