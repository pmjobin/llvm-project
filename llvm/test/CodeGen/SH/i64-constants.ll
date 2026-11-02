; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,LE

; COMMON-NOT: .long
; COMMON-NOT: .quad
; COMMON-NOT: .rodata
; COMMON-NOT: __

define i64 @constant_zero() {
; COMMON-LABEL: constant_zero:
; COMMON: mov	#0,
; COMMON-NEXT: rts
; COMMON-NEXT: mov
	ret i64 0
}

define i64 @constant_one() {
; COMMON-LABEL: constant_one:
; BE: mov	#0,r0
; BE-NEXT: rts
; BE-NEXT: mov	#1,r1
; LE: mov	#1,r0
; LE-NEXT: rts
; LE-NEXT: mov	#0,r1
	ret i64 1
}

define i64 @constant_low_ones() {
; COMMON-LABEL: constant_low_ones:
; BE: mov	#0,r0
; BE-NEXT: rts
; BE-NEXT: mov	#-1,r1
; LE: mov	#-1,r0
; LE-NEXT: rts
; LE-NEXT: mov	#0,r1
	ret i64 4294967295
}

define i64 @constant_high_one() {
; COMMON-LABEL: constant_high_one:
; BE: mov	#1,r0
; BE-NEXT: rts
; BE-NEXT: mov	#0,r1
; LE: mov	#0,r0
; LE-NEXT: rts
; LE-NEXT: mov	#1,r1
	ret i64 4294967296
}

define i64 @constant_pattern() {
; COMMON-LABEL: constant_pattern:
; BE: mov	#18,
; BE: add	#120,
; BE: mov	#-101,
; BE: rts
; BE-NEXT: add	#-16,
; LE: mov	#-101,
; LE: add	#-16,
; LE: mov	#18,
; LE: rts
; LE-NEXT: add	#120,
	ret i64 1311768467463790320
}

define i64 @constant_minimum() {
; COMMON-LABEL: constant_minimum:
; BE: mov	#-128,
; BE: shll16
; BE: shll8
; BE: rts
; BE-NEXT: mov	#0,
; LE: mov	#0,
; LE: mov	#-128,
; LE: shll16
; LE: rts
; LE-NEXT: shll8
	ret i64 -9223372036854775808
}

define i64 @constant_negative_one() {
; COMMON-LABEL: constant_negative_one:
; COMMON: mov	#-1,
; COMMON-NEXT: rts
; COMMON-NEXT: mov
	ret i64 -1
}

define i64 @constant_deadbeef() {
; COMMON-LABEL: constant_deadbeef:
; BE: mov	#-33,
; BE: add	#-17,
; BE: mov	#1,
; BE: rts
; BE-NEXT: add	#103,
; LE: mov	#1,
; LE: add	#103,
; LE: mov	#-33,
; LE: rts
; LE-NEXT: add	#-17,
	ret i64 -2401053092593050265
}

define i32 @unused_high_constant_half() {
; COMMON-LABEL: unused_high_constant_half:
; COMMON-NOT: mov	#-33,
; COMMON: mov	#1,
; COMMON: rts
; COMMON-NEXT: add	#103,
	%low = trunc i64 -2401053092593050265 to i32
	ret i32 %low
}
