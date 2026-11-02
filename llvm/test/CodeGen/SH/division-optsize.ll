; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh3 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh3 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o %t.pic.o
; RUN: llvm-objdump -dr --mcpu=sh2 %t.pic.o | FileCheck %s --check-prefix=PIC

define i32 @normal_sdiv(i32 %dividend, i32 %divisor) {
; CHECK-LABEL: normal_sdiv:
; CHECK: div0s
; CHECK-COUNT-32: div1
; CHECK: rts
	%quotient = sdiv i32 %dividend, %divisor
	ret i32 %quotient
}

define i32 @normal_udiv(i32 %dividend, i32 %divisor) {
; CHECK-LABEL: normal_udiv:
; CHECK: div0u
; CHECK-COUNT-32: div1
; CHECK: rts
	%quotient = udiv i32 %dividend, %divisor
	ret i32 %quotient
}

define i32 @normal_srem(i32 %dividend, i32 %divisor) {
; CHECK-LABEL: normal_srem:
; CHECK: div0s
; CHECK-COUNT-32: div1
; CHECK: mul.l
; CHECK: rts
	%remainder = srem i32 %dividend, %divisor
	ret i32 %remainder
}

define i32 @normal_urem(i32 %dividend, i32 %divisor) {
; CHECK-LABEL: normal_urem:
; CHECK: div0u
; CHECK-COUNT-32: div1
; CHECK: mul.l
; CHECK: rts
	%remainder = urem i32 %dividend, %divisor
	ret i32 %remainder
}

define i32 @normal_udiv_2(i32 %value) {
; CHECK-LABEL: normal_udiv_2:
; CHECK-NOT: __udivsi3
; CHECK: rts
; CHECK-NEXT: shlr
	%result = udiv i32 %value, 2
	ret i32 %result
}

define i32 @normal_udiv_3(i32 %value) {
; CHECK-LABEL: normal_udiv_3:
; CHECK-NOT: __udivsi3
; CHECK: dmulu.l
; CHECK: rts
	%result = udiv i32 %value, 3
	ret i32 %result
}

define i32 @normal_sdiv_10(i32 %value) {
; CHECK-LABEL: normal_sdiv_10:
; CHECK-NOT: __divsi3
; CHECK: dmuls.l
; CHECK: rts
	%result = sdiv i32 %value, 10
	ret i32 %result
}

define i32 @normal_urem_16(i32 %value) {
; CHECK-LABEL: normal_urem_16:
; CHECK-NOT: __umodsi3
; CHECK: rts
; CHECK-NEXT: and
	%result = urem i32 %value, 16
	ret i32 %result
}

define i32 @size_sdiv(i32 %dividend, i32 %divisor) optsize {
; CHECK-LABEL: size_sdiv:
; CHECK-NOT: div0s
; CHECK: jsr
; CHECK: rts
; CHECK: .long	__divsi3
	%quotient = sdiv i32 %dividend, %divisor
	ret i32 %quotient
}

define i32 @size_udiv(i32 %dividend, i32 %divisor) optsize {
; CHECK-LABEL: size_udiv:
; CHECK-NOT: div0u
; CHECK: jsr
; CHECK: rts
; CHECK: .long	__udivsi3
	%quotient = udiv i32 %dividend, %divisor
	ret i32 %quotient
}

define i32 @size_srem(i32 %dividend, i32 %divisor) optsize {
; CHECK-LABEL: size_srem:
; CHECK-NOT: div0s
; CHECK: jsr
; CHECK: rts
; CHECK: .long	__modsi3
	%remainder = srem i32 %dividend, %divisor
	ret i32 %remainder
}

define i32 @size_urem(i32 %dividend, i32 %divisor) minsize {
; CHECK-LABEL: size_urem:
; CHECK-NOT: div0u
; CHECK: jsr
; CHECK: rts
; CHECK: .long	__umodsi3
	%remainder = urem i32 %dividend, %divisor
	ret i32 %remainder
}

define { i32, i32 } @size_sdivrem(i32 %dividend, i32 %divisor) optsize {
; CHECK-LABEL: size_sdivrem:
; CHECK-COUNT-1: jsr
; CHECK: mov.l	@r15,
; CHECK: rts
; CHECK: .long	__divmodsi4
	%quotient = sdiv i32 %dividend, %divisor
	%remainder = srem i32 %dividend, %divisor
	%first = insertvalue { i32, i32 } poison, i32 %quotient, 0
	%result = insertvalue { i32, i32 } %first, i32 %remainder, 1
	ret { i32, i32 } %result
}

define { i32, i32 } @size_udivrem(i32 %dividend, i32 %divisor) optsize {
; CHECK-LABEL: size_udivrem:
; CHECK-COUNT-1: jsr
; CHECK: mov.l	@r15,
; CHECK: rts
; CHECK: .long	__udivmodsi4
	%quotient = udiv i32 %dividend, %divisor
	%remainder = urem i32 %dividend, %divisor
	%first = insertvalue { i32, i32 } poison, i32 %quotient, 0
	%result = insertvalue { i32, i32 } %first, i32 %remainder, 1
	ret { i32, i32 } %result
}

define i32 @size_udiv_2(i32 %value) optsize {
; CHECK-LABEL: size_udiv_2:
; CHECK-NOT: __udivsi3
; CHECK: rts
; CHECK-NEXT: shlr
	%result = udiv i32 %value, 2
	ret i32 %result
}

define i32 @size_udiv_3(i32 %value) optsize {
; CHECK-LABEL: size_udiv_3:
; CHECK-NOT: __udivsi3
; CHECK: dmulu.l
; CHECK: rts
	%result = udiv i32 %value, 3
	ret i32 %result
}

define i32 @size_sdiv_10(i32 %value) optsize {
; CHECK-LABEL: size_sdiv_10:
; CHECK-NOT: __divsi3
; CHECK: dmuls.l
; CHECK: rts
	%result = sdiv i32 %value, 10
	ret i32 %result
}

define i32 @size_urem_16(i32 %value) optsize {
; CHECK-LABEL: size_urem_16:
; CHECK-NOT: __umodsi3
; CHECK: rts
; CHECK-NEXT: and
	%result = urem i32 %value, 16
	ret i32 %result
}

define i32 @size_udiv_i16(i32 %dividend, i32 %divisor) optsize {
; CHECK-LABEL: size_udiv_i16:
; CHECK: jsr
; CHECK: rts
; CHECK: .long	__udivsi3
	%dividend16 = trunc i32 %dividend to i16
	%divisor16 = trunc i32 %divisor to i16
	%quotient16 = udiv i16 %dividend16, %divisor16
	%quotient = zext i16 %quotient16 to i32
	ret i32 %quotient
}

define i32 @size_srem_i8(i32 %dividend, i32 %divisor) optsize {
; CHECK-LABEL: size_srem_i8:
; CHECK: jsr
; CHECK: rts
; CHECK: .long	__modsi3
	%dividend8 = trunc i32 %dividend to i8
	%divisor8 = trunc i32 %divisor to i8
	%remainder8 = srem i8 %dividend8, %divisor8
	%remainder = sext i8 %remainder8 to i32
	ret i32 %remainder
}

define i64 @size_sdiv_i64(i64 %dividend, i64 %divisor) optsize {
; CHECK-LABEL: size_sdiv_i64:
; CHECK: jsr
; CHECK: .long	__divdi3
	%quotient = sdiv i64 %dividend, %divisor
	ret i64 %quotient
}

define i64 @size_udiv_i64(i64 %dividend, i64 %divisor) optsize {
; CHECK-LABEL: size_udiv_i64:
; CHECK: jsr
; CHECK: .long	__udivdi3
	%quotient = udiv i64 %dividend, %divisor
	ret i64 %quotient
}

define i64 @size_srem_i64(i64 %dividend, i64 %divisor) optsize {
; CHECK-LABEL: size_srem_i64:
; CHECK: jsr
; CHECK: .long	__moddi3
	%remainder = srem i64 %dividend, %divisor
	ret i64 %remainder
}

define i64 @size_urem_i64(i64 %dividend, i64 %divisor) optsize {
; CHECK-LABEL: size_urem_i64:
; CHECK: jsr
; CHECK: .long	__umoddi3
	%remainder = urem i64 %dividend, %divisor
	ret i64 %remainder
}

; PIC-DAG: R_SH_PLT32	__divsi3
; PIC-DAG: R_SH_PLT32	__udivsi3
; PIC-DAG: R_SH_PLT32	__modsi3
; PIC-DAG: R_SH_PLT32	__umodsi3
; PIC-DAG: R_SH_PLT32	__divmodsi4
; PIC-DAG: R_SH_PLT32	__udivmodsi4
