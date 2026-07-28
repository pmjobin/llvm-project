; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,LE

; COMMON-NOT: __adddi3
; COMMON-NOT: __subdi3
; COMMON-NOT: .long
; COMMON-NOT: .quad
; COMMON-NOT: .rodata

define i64 @add64(i64 %a, i64 %b) {
; COMMON-LABEL: add64:
; BE: clrt
; BE-NEXT: mov	r5,r1
; BE-NEXT: addc	r7,r1
; BE-NEXT: mov	r4,r0
; BE-NEXT: addc	r6,r0
; LE: clrt
; LE-NEXT: mov	r4,r0
; LE-NEXT: addc	r6,r0
; LE-NEXT: mov	r5,r1
; LE-NEXT: addc	r7,r1
	%result = add i64 %a, %b
	ret i64 %result
}

define i64 @add64_flags(i64 %a, i64 %b) {
; COMMON-LABEL: add64_flags:
; COMMON: clrt
; COMMON: addc
; COMMON: addc
	%result = add nuw nsw i64 %a, %b
	ret i64 %result
}

define i64 @sub64(i64 %a, i64 %b) {
; COMMON-LABEL: sub64:
; BE: clrt
; BE: subc	r7,r1
; BE: subc	r6,r0
; LE: clrt
; LE: subc	r6,r0
; LE: subc	r7,r1
	%result = sub i64 %a, %b
	ret i64 %result
}

define i64 @neg64(i64 %value) {
; COMMON-LABEL: neg64:
; COMMON: mov	#0,
; COMMON: clrt
; COMMON: subc
; COMMON-NEXT: subc
	%result = sub i64 0, %value
	ret i64 %result
}

define i64 @and64(i64 %a, i64 %b) {
; COMMON-LABEL: and64:
; COMMON: and
; COMMON: and
	%result = and i64 %a, %b
	ret i64 %result
}

define i64 @or64(i64 %a, i64 %b) {
; COMMON-LABEL: or64:
; COMMON: or
; COMMON: or
	%result = or i64 %a, %b
	ret i64 %result
}

define i64 @xor64(i64 %a, i64 %b) {
; COMMON-LABEL: xor64:
; COMMON: xor
; COMMON: xor
	%result = xor i64 %a, %b
	ret i64 %result
}

define i64 @complement64(i64 %value) {
; COMMON-LABEL: complement64:
; COMMON: not
; COMMON-NEXT: not
	%result = xor i64 %value, -1
	ret i64 %result
}

define i64 @constant64() {
; COMMON-LABEL: constant64:
; BE: mov	#18,r0
; BE: mov	#120,
; BE: mov	#-102,r1
; BE: mov	#-16,
; LE: mov	#-102,r0
; LE: mov	#-16,
; LE: mov	#18,r1
; LE: mov	#120,
; COMMON-NOT: .long
; COMMON: rts
	ret i64 1311768467463790320
}

define i64 @constant_halves() {
; COMMON-LABEL: constant_halves:
; BE: mov	#1,r0
; BE-NEXT: mov	#-1,r1
; LE: mov	#-1,r0
; LE-NEXT: mov	#1,r1
; COMMON: rts
	ret i64 8589934591
}

define i64 @zero_extend64(i32 %value) {
; COMMON-LABEL: zero_extend64:
; BE: mov	r4,r1
; BE-NEXT: mov	#0,r0
; LE: mov	r4,r0
; LE-NEXT: mov	#0,r1
	%result = zext i32 %value to i64
	ret i64 %result
}

define i64 @sign_extend64(i32 %value) {
; COMMON-LABEL: sign_extend64:
; COMMON: shar
; COMMON: rts
	%result = sext i32 %value to i64
	ret i64 %result
}

define i64 @zero_extend_i8_to_i64(i32 %value) {
; COMMON-LABEL: zero_extend_i8_to_i64:
; COMMON: extu.b
; COMMON: mov	#0,
	%narrow = trunc i32 %value to i8
	%result = zext i8 %narrow to i64
	ret i64 %result
}

define i64 @sign_extend_i16_to_i64(i32 %value) {
; COMMON-LABEL: sign_extend_i16_to_i64:
; COMMON: exts.w
; COMMON: shar
	%narrow = trunc i32 %value to i16
	%result = sext i16 %narrow to i64
	ret i64 %result
}

define i32 @truncate64(i64 %value) {
; COMMON-LABEL: truncate64:
; BE: mov	r5,r0
; LE: mov	r4,r0
; COMMON-NEXT: rts
	%result = trunc i64 %value to i32
	ret i32 %result
}
