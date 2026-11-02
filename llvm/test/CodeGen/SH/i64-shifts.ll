; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,SH2,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,SH2,LE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,SH3,SH3BE,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=COMMON,SH3,SH3LE,LE

; COMMON-NOT: __ashldi3
; COMMON-NOT: __lshrdi3
; COMMON-NOT: __ashrdi3

define i64 @shl_zero(i64 %value) {
; COMMON-LABEL: shl_zero:
; COMMON-NOT: shll
; COMMON: rts
	%result = shl i64 %value, 0
	ret i64 %result
}

define i64 @shl_one(i64 %value) {
; COMMON-LABEL: shl_one:
; COMMON: shll
; SH2: rotl
; SH2: movt
; SH3: shld
; COMMON: or
; COMMON: rts
; COMMON-NEXT: shll
	%result = shl i64 %value, 1
	ret i64 %result
}

define i64 @shl_32(i64 %value) {
; COMMON-LABEL: shl_32:
; BE: mov	r5,r0
; BE-NEXT: rts
; BE-NEXT: mov	#0,r1
; LE: mov	r4,r1
; LE-NEXT: rts
; LE-NEXT: mov	#0,r0
; COMMON-NOT: shll
	%result = shl i64 %value, 32
	ret i64 %result
}

define i64 @lshr_one(i64 %value) {
; COMMON-LABEL: lshr_one:
; SH2: shll16
; SH3: shld
; COMMON: or
; COMMON-NOT: rotcr
; COMMON: rts
; COMMON-NEXT: shlr
	%result = lshr i64 %value, 1
	ret i64 %result
}

define i64 @ashr_one(i64 %value) {
; COMMON-LABEL: ashr_one:
; SH2: shll16
; SH3: shld
; COMMON: or
; COMMON-NOT: rotcr
; COMMON: rts
; COMMON-NEXT: shar
	%result = ashr i64 %value, 1
	ret i64 %result
}

define i64 @lshr_32(i64 %value) {
; COMMON-LABEL: lshr_32:
; BE: mov	r4,r1
; BE-NEXT: rts
; BE-NEXT: mov	#0,r0
; LE: mov	r5,r0
; LE-NEXT: rts
; LE-NEXT: mov	#0,r1
; COMMON-NOT: shlr
	%result = lshr i64 %value, 32
	ret i64 %result
}

define i64 @ashr_32(i64 %value) {
; COMMON-LABEL: ashr_32:
; COMMON: mov
; SH2: shar
; SH2: rts
; SH3: rts
; SH3-NEXT: shad
	%result = ashr i64 %value, 32
	ret i64 %result
}

define i64 @shl_63(i64 %value) {
; COMMON-LABEL: shl_63:
; SH2: shll16
; SH2: shll8
; SH2: shll2
; SH3: shld
; COMMON: rts
	%result = shl i64 %value, 63
	ret i64 %result
}

define i64 @shl_31(i64 %value) {
; COMMON-LABEL: shl_31:
; COMMON: shlr
; SH2: shll16
; COMMON-NOT: rotcl
; COMMON: rts
; SH3-NEXT: shld
	%result = shl i64 %value, 31
	ret i64 %result
}

define i64 @shl_33(i64 %value) {
; COMMON-LABEL: shl_33:
; COMMON: shll
; COMMON-NOT: rotcl
; COMMON: rts
; COMMON-NEXT: mov	#0,
	%result = shl i64 %value, 33
	ret i64 %result
}

define i64 @lshr_31(i64 %value) {
; COMMON-LABEL: lshr_31:
; COMMON-NOT: rotcr
; COMMON: shll
; COMMON: or
; SH2: rotl
; COMMON: rts
; SH2-NEXT: movt
; SH3-NEXT: shld
	%result = lshr i64 %value, 31
	ret i64 %result
}

define i64 @lshr_33(i64 %value) {
; COMMON-LABEL: lshr_33:
; COMMON: shlr
; COMMON-NOT: rotcr
; COMMON: rts
; COMMON-NEXT: mov	#0,
	%result = lshr i64 %value, 33
	ret i64 %result
}

define i64 @lshr_63(i64 %value) {
; COMMON-LABEL: lshr_63:
; SH2: rotl
; SH2: movt
; SH3: shld
; COMMON-NOT: rotcr
; COMMON: rts
; COMMON-NEXT: mov	#0,
	%result = lshr i64 %value, 63
	ret i64 %result
}

define i64 @ashr_31(i64 %value) {
; COMMON-LABEL: ashr_31:
; COMMON: shll
; SH2: shar
; SH2: rts
; SH3: rts
; SH3-NEXT: shad
; COMMON-NOT: rotcr
	%result = ashr i64 %value, 31
	ret i64 %result
}

define i64 @ashr_33(i64 %value) {
; COMMON-LABEL: ashr_33:
; SH2: shar
; SH2: rts
; SH3BE: shad
; SH3BE: rts
; SH3BE-NEXT: shar
; SH3LE: shar
; SH3LE: rts
; SH3LE-NEXT: shad
; COMMON-NOT: rotcr
	%result = ashr i64 %value, 33
	ret i64 %result
}

define i64 @ashr_63(i64 %value) {
; COMMON-LABEL: ashr_63:
; SH2: shar
; SH3: shad
; COMMON-NOT: rotcr
; COMMON: rts
	%result = ashr i64 %value, 63
	ret i64 %result
}

define i64 @variable_shl(i64 %value, i64 %count) {
; COMMON-LABEL: variable_shl:
; COMMON: mov	#63,
; BE: and	r7,
; LE: and	r6,
; COMMON: tst
; COMMON: bt
; COMMON: shll
; COMMON-NEXT: rotcl
; COMMON-NEXT: dt
; COMMON: bf
; COMMON-NOT: bra
; COMMON: rts
	%result = shl i64 %value, %count
	ret i64 %result
}

define i64 @variable_lshr(i64 %value, i64 %count) {
; COMMON-LABEL: variable_lshr:
; COMMON: mov	#63,
; COMMON: tst
; COMMON: bt
; COMMON: shlr
; COMMON-NEXT: rotcr
; COMMON-NEXT: dt
; COMMON: bf
; COMMON: rts
	%result = lshr i64 %value, %count
	ret i64 %result
}

define i64 @variable_ashr(i64 %value, i64 %count) {
; COMMON-LABEL: variable_ashr:
; COMMON: mov	#63,
; COMMON: tst
; COMMON: bt
; COMMON: shar
; COMMON-NEXT: rotcr
; COMMON-NEXT: dt
; COMMON: bf
; COMMON: rts
	%result = ashr i64 %value, %count
	ret i64 %result
}

define i64 @two_variable_shifts(i64 %value, i64 %a, i64 %b) {
; COMMON-LABEL: two_variable_shifts:
; COMMON: mov	#63,
; COMMON: rotcl
; COMMON: bf
; COMMON: mov	#63,
; COMMON: rotcl
; COMMON: bf
	%first = shl i64 %value, %a
	%second = shl i64 %first, %b
	ret i64 %second
}
