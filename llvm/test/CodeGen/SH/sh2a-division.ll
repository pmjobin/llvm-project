; RUN: llc -mtriple=sh -mcpu=sh2a -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SH2A
; RUN: llc -mtriple=shle -mcpu=sh2a -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SH2A
; RUN: llc -mtriple=shle -mcpu=sh2 -mtune=sh2a -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SH2

define i32 @signed_quotient(i32 %dividend, i32 %divisor) {
	%result = sdiv i32 %dividend, %divisor
	ret i32 %result
}

define i32 @unsigned_quotient(i32 %dividend, i32 %divisor) {
	%result = udiv i32 %dividend, %divisor
	ret i32 %result
}

define i32 @signed_remainder(i32 %dividend, i32 %divisor) {
	%result = srem i32 %dividend, %divisor
	ret i32 %result
}

define i32 @unsigned_remainder(i32 %dividend, i32 %divisor) {
	%result = urem i32 %dividend, %divisor
	ret i32 %result
}

; SH2A-LABEL: signed_quotient:
; SH2A: divs r0,
; SH2A-LABEL: unsigned_quotient:
; SH2A: divu r0,
; SH2A-LABEL: signed_remainder:
; SH2A: divs r0,
; SH2A: mul.l
; SH2A: sts macl,
; SH2A: sub
; SH2A-LABEL: unsigned_remainder:
; SH2A: divu r0,
; SH2A: mul.l
; SH2A: sts macl,
; SH2A: sub
; SH2A-NOT: __divsi3
; SH2A-NOT: __udivsi3

; SH2-NOT: divs r0,
; SH2-NOT: divu r0,
