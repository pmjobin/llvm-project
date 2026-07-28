; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=COMMON
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=COMMON
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=COMMON
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=COMMON

; COMMON-NOT: __mulsi3
; COMMON-NOT: __udivsi3
; COMMON-NOT: __divsi3
; COMMON-NOT: __umodsi3
; COMMON-NOT: __modsi3
; COMMON-NOT: dmulu.l
; COMMON-NOT: dmuls.l

define i32 @udiv_1(i32 %value) {
; COMMON-LABEL: udiv_1:
; COMMON-NOT: div0u
; COMMON: rts
	%result = udiv i32 %value, 1
	ret i32 %result
}

define i32 @udiv_2(i32 %value) {
; COMMON-LABEL: udiv_2:
; COMMON: shlr
; COMMON-NOT: div0u
; COMMON: rts
	%result = udiv i32 %value, 2
	ret i32 %result
}

define i32 @udiv_3(i32 %value) {
; COMMON-LABEL: udiv_3:
; COMMON: div0u
; COMMON-COUNT-32: div1
; COMMON: rts
	%result = udiv i32 %value, 3
	ret i32 %result
}

define i32 @udiv_5(i32 %value) {
; COMMON-LABEL: udiv_5:
; COMMON: div0u
; COMMON: rts
	%result = udiv i32 %value, 5
	ret i32 %result
}

define i32 @udiv_7(i32 %value) {
; COMMON-LABEL: udiv_7:
; COMMON: div0u
; COMMON: rts
	%result = udiv i32 %value, 7
	ret i32 %result
}

define i32 @udiv_10(i32 %value) {
; COMMON-LABEL: udiv_10:
; COMMON: div0u
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
; COMMON: div0u
; COMMON: rts
	%result = udiv i32 %value, 31
	ret i32 %result
}

define i32 @udiv_top_bit(i32 %value) {
; COMMON-LABEL: udiv_top_bit:
; COMMON: shlr16
; COMMON-NOT: div0u
; COMMON: rts
	%result = udiv i32 %value, 2147483648
	ret i32 %result
}

define i32 @udiv_max(i32 %value) {
; COMMON-LABEL: udiv_max:
; COMMON: div0u
; COMMON-COUNT-32: div1
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
; COMMON: neg
; COMMON-NOT: div0s
; COMMON: rts
	%result = sdiv i32 %value, -1
	ret i32 %result
}

define i32 @sdiv_2(i32 %value) {
; COMMON-LABEL: sdiv_2:
; COMMON: {{shar|div0s}}
; COMMON: rts
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
; COMMON: div0s
; COMMON: rts
	%result = sdiv i32 %value, 3
	ret i32 %result
}

define i32 @sdiv_negative_3(i32 %value) {
; COMMON-LABEL: sdiv_negative_3:
; COMMON: div0s
; COMMON: rts
	%result = sdiv i32 %value, -3
	ret i32 %result
}

define i32 @sdiv_7(i32 %value) {
; COMMON-LABEL: sdiv_7:
; COMMON: div0s
; COMMON: rts
	%result = sdiv i32 %value, 7
	ret i32 %result
}

define i32 @sdiv_negative_7(i32 %value) {
; COMMON-LABEL: sdiv_negative_7:
; COMMON: div0s
; COMMON: rts
	%result = sdiv i32 %value, -7
	ret i32 %result
}

define i32 @sdiv_min(i32 %value) {
; COMMON-LABEL: sdiv_min:
; COMMON: div0s
; COMMON-COUNT-32: div1
; COMMON: rts
	%result = sdiv i32 %value, -2147483648
	ret i32 %result
}

define i32 @urem_max(i32 %value) {
; COMMON-LABEL: urem_max:
; COMMON: div0u
; COMMON-COUNT-32: div1
; COMMON: mul.l
; COMMON-NEXT: sts	macl,
; COMMON: rts
	%result = urem i32 %value, 4294967295
	ret i32 %result
}

define i32 @srem_min(i32 %value) {
; COMMON-LABEL: srem_min:
; COMMON: div0s
; COMMON-COUNT-32: div1
; COMMON: mul.l
; COMMON-NEXT: sts	macl,
; COMMON: rts
	%result = srem i32 %value, -2147483648
	ret i32 %result
}

define i32 @udivrem_max(i32 %value) {
; COMMON-LABEL: udivrem_max:
; COMMON: div0u
; COMMON-COUNT-32: div1
; COMMON: mul.l
; COMMON-NEXT: sts	macl,
; COMMON-NOT: div0u
; COMMON-NOT: div1
; COMMON: rts
	%quotient = udiv i32 %value, 4294967295
	%remainder = urem i32 %value, 4294967295
	%result = add i32 %quotient, %remainder
	ret i32 %result
}
