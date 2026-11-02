; Speed code uses LLVM reciprocal constant division; O0 retains the
; original exact division sequence. Both paths retain runtime boundary tests.
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefixes=COMMON,BASE
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefixes=COMMON,SPEED
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefixes=COMMON,BASE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefixes=COMMON,SPEED

; COMMON-NOT: __mulsi3
; COMMON-NOT: __udivsi3
; COMMON-NOT: __divsi3
; COMMON-NOT: __umodsi3
; COMMON-NOT: __modsi3
; BASE-NOT: dmulu.l
; BASE-NOT: dmuls.l

define i32 @udiv_1(i32 %value) {
; COMMON-LABEL: udiv_1:
; COMMON-NOT: div0u
; COMMON: rts
	%result = udiv i32 %value, 1
	ret i32 %result
}

define i32 @udiv_2(i32 %value) {
; COMMON-LABEL: udiv_2:
; COMMON-NOT: div0u
; COMMON: rts
; COMMON-NEXT: shlr
	%result = udiv i32 %value, 2
	ret i32 %result
}

define i32 @udiv_3(i32 %value) {
; COMMON-LABEL: udiv_3:
; SPEED: dmulu.l
; BASE: div0u
; BASE-COUNT-32: div1
; COMMON: rts
	%result = udiv i32 %value, 3
	ret i32 %result
}

define i32 @udiv_5(i32 %value) {
; COMMON-LABEL: udiv_5:
; SPEED: dmulu.l
; BASE: div0u
; COMMON: rts
	%result = udiv i32 %value, 5
	ret i32 %result
}

define i32 @udiv_7(i32 %value) {
; COMMON-LABEL: udiv_7:
; SPEED: dmulu.l
; BASE: div0u
; COMMON: rts
	%result = udiv i32 %value, 7
	ret i32 %result
}

define i32 @udiv_10(i32 %value) {
; COMMON-LABEL: udiv_10:
; SPEED: dmulu.l
; BASE: div0u
; COMMON: rts
	%result = udiv i32 %value, 10
	ret i32 %result
}

define i32 @udiv_16(i32 %value) {
; COMMON-LABEL: udiv_16:
; COMMON: shlr2
; COMMON-NOT: div0u
; COMMON: rts
	%result = udiv i32 %value, 16
	ret i32 %result
}

define i32 @udiv_31(i32 %value) {
; COMMON-LABEL: udiv_31:
; SPEED: dmulu.l
; BASE: div0u
; COMMON: rts
	%result = udiv i32 %value, 31
	ret i32 %result
}

define i32 @udiv_top_bit(i32 %value) {
; COMMON-LABEL: udiv_top_bit:
; COMMON: rotl
; COMMON-NOT: div0u
; COMMON: rts
; COMMON-NEXT: movt
	%result = udiv i32 %value, 2147483648
	ret i32 %result
}

define i32 @udiv_max(i32 %value) {
; COMMON-LABEL: udiv_max:
; BASE: div0u
; BASE-COUNT-32: div1
; COMMON: rts
	%result = udiv i32 %value, 4294967295
	ret i32 %result
}

define i32 @sdiv_1(i32 %value) {
; COMMON-LABEL: sdiv_1:
; COMMON-NOT: div0s
; COMMON: rts
	%result = sdiv i32 %value, 1
	ret i32 %result
}

define i32 @sdiv_negative_1(i32 %value) {
; COMMON-LABEL: sdiv_negative_1:
; COMMON-NOT: div0s
; COMMON: rts
; COMMON-NEXT: neg
	%result = sdiv i32 %value, -1
	ret i32 %result
}

define i32 @sdiv_2(i32 %value) {
; COMMON-LABEL: sdiv_2:
; BASE: {{shar|div0s}}
; SPEED: cmp/pz
; SPEED-NEXT: subc
; COMMON: rts
; SPEED-NEXT: shar
	%result = sdiv i32 %value, 2
	ret i32 %result
}

define i32 @sdiv_negative_2(i32 %value) {
; COMMON-LABEL: sdiv_negative_2:
; COMMON: {{shar|div0s}}
; COMMON: rts
	%result = sdiv i32 %value, -2
	ret i32 %result
}

define i32 @sdiv_3(i32 %value) {
; COMMON-LABEL: sdiv_3:
; BASE: div0s
; COMMON: rts
	%result = sdiv i32 %value, 3
	ret i32 %result
}

define i32 @sdiv_negative_3(i32 %value) {
; COMMON-LABEL: sdiv_negative_3:
; BASE: div0s
; COMMON: rts
	%result = sdiv i32 %value, -3
	ret i32 %result
}

define i32 @sdiv_7(i32 %value) {
; COMMON-LABEL: sdiv_7:
; BASE: div0s
; COMMON: rts
	%result = sdiv i32 %value, 7
	ret i32 %result
}

define i32 @sdiv_negative_7(i32 %value) {
; COMMON-LABEL: sdiv_negative_7:
; BASE: div0s
; COMMON: rts
	%result = sdiv i32 %value, -7
	ret i32 %result
}

define i32 @sdiv_min(i32 %value) {
; COMMON-LABEL: sdiv_min:
; BASE: div0s
; BASE-COUNT-32: div1
; COMMON: rts
	%result = sdiv i32 %value, -2147483648
	ret i32 %result
}

define i32 @urem_max(i32 %value) {
; COMMON-LABEL: urem_max:
; BASE: div0u
; BASE-COUNT-32: div1
; BASE: mul.l
; BASE-NEXT: sts	macl,
; SPEED: dmulu.l
; SPEED-NEXT: sts	mach,
; COMMON: rts
; SPEED-NEXT: sub
	%result = urem i32 %value, 4294967295
	ret i32 %result
}

define i32 @srem_min(i32 %value) {
; COMMON-LABEL: srem_min:
; BASE: div0s
; BASE-COUNT-32: div1
; BASE: mul.l
; BASE-NEXT: sts	macl,
; SPEED: shlr{{[ \t]}}
; SPEED-NEXT: add
; SPEED-NEXT: and
; COMMON: rts
; SPEED-NEXT: add
	%result = srem i32 %value, -2147483648
	ret i32 %result
}

define i32 @udivrem_max(i32 %value) {
; COMMON-LABEL: udivrem_max:
; BASE: div0u
; BASE-COUNT-32: div1
; BASE: mul.l
; BASE-NEXT: sts	macl,
; SPEED: dmulu.l
; SPEED: sts	mach,[[HI:r[0-9]+]]
; SPEED: sub	{{r[0-9]+}},[[RESULT:r[0-9]+]]
; SPEED-NEXT: mov	#-1,[[NEGONE:r[0-9]+]]
; SPEED-NEXT: cmp/pz	[[HI]]
; COMMON-NOT: div0u
; COMMON-NOT: div1
; COMMON: rts
; SPEED-NEXT: subc	[[NEGONE]],[[RESULT]]
	%quotient = udiv i32 %value, 4294967295
	%remainder = urem i32 %value, 4294967295
	%result = add i32 %quotient, %remainder
	ret i32 %result
}
