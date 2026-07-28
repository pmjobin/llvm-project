; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i64-mul.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=WIDE-MUL
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i64-udiv.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=WIDE-DIV
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i64-sdiv.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=WIDE-DIV
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i64-urem.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=WIDE-DIV
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i64-srem.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=WIDE-DIV
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i8-udiv.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=NARROW
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i16-sdiv.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=NARROW
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i8-urem.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=NARROW
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i16-srem.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=NARROW
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector-mul.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TYPE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector-div.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TYPE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/floating.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TYPE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/multiply-overflow.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INTRINSIC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/saturating.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INTRINSIC

; WIDE-MUL: LLVM ERROR: SH i64 multiplication is not supported
; WIDE-DIV: LLVM ERROR: SH i64 division and remainder are not supported
; NARROW: LLVM ERROR: SH narrow integer division and remainder are not supported
; TYPE: LLVM ERROR: SH only supports i8, i16, i32, and selected i64 integer operations
; INTRINSIC: LLVM ERROR: SH intrinsics are not supported

;--- i64-mul.ll
define i32 @i64_mul(i32 %a, i32 %b) {
	%a64 = zext i32 %a to i64
	%b64 = zext i32 %b to i64
	%wide = mul i64 %a64, %b64
	%result = trunc i64 %wide to i32
	ret i32 %result
}

;--- i64-udiv.ll
define i32 @i64_udiv(i32 %a, i32 %b) {
	%a64 = zext i32 %a to i64
	%b64 = zext i32 %b to i64
	%wide = udiv i64 %a64, %b64
	%result = trunc i64 %wide to i32
	ret i32 %result
}

;--- i64-sdiv.ll
define i32 @i64_sdiv(i32 %a, i32 %b) {
	%a64 = sext i32 %a to i64
	%b64 = sext i32 %b to i64
	%wide = sdiv i64 %a64, %b64
	%result = trunc i64 %wide to i32
	ret i32 %result
}

;--- i64-urem.ll
define i32 @i64_urem(i32 %a, i32 %b) {
	%a64 = zext i32 %a to i64
	%b64 = zext i32 %b to i64
	%wide = urem i64 %a64, %b64
	%result = trunc i64 %wide to i32
	ret i32 %result
}

;--- i64-srem.ll
define i32 @i64_srem(i32 %a, i32 %b) {
	%a64 = sext i32 %a to i64
	%b64 = sext i32 %b to i64
	%wide = srem i64 %a64, %b64
	%result = trunc i64 %wide to i32
	ret i32 %result
}

;--- i8-udiv.ll
define i32 @i8_udiv(i32 %a, i32 %b) {
	%a8 = trunc i32 %a to i8
	%b8 = trunc i32 %b to i8
	%narrow = udiv i8 %a8, %b8
	%result = zext i8 %narrow to i32
	ret i32 %result
}

;--- i16-sdiv.ll
define i32 @i16_sdiv(i32 %a, i32 %b) {
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%narrow = sdiv i16 %a16, %b16
	%result = sext i16 %narrow to i32
	ret i32 %result
}

;--- i8-urem.ll
define i32 @i8_urem(i32 %a, i32 %b) {
	%a8 = trunc i32 %a to i8
	%b8 = trunc i32 %b to i8
	%narrow = urem i8 %a8, %b8
	%result = zext i8 %narrow to i32
	ret i32 %result
}

;--- i16-srem.ll
define i32 @i16_srem(i32 %a, i32 %b) {
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%narrow = srem i16 %a16, %b16
	%result = sext i16 %narrow to i32
	ret i32 %result
}

;--- vector-mul.ll
define i32 @vector_mul(i32 %a, i32 %b) {
	%va = insertelement <2 x i32> poison, i32 %a, i32 0
	%vb = insertelement <2 x i32> poison, i32 %b, i32 0
	%product = mul <2 x i32> %va, %vb
	%result = extractelement <2 x i32> %product, i32 0
	ret i32 %result
}

;--- vector-div.ll
define i32 @vector_div(i32 %a, i32 %b) {
	%va = insertelement <2 x i32> poison, i32 %a, i32 0
	%vb = insertelement <2 x i32> poison, i32 %b, i32 0
	%quotient = udiv <2 x i32> %va, %vb
	%result = extractelement <2 x i32> %quotient, i32 0
	ret i32 %result
}

;--- floating.ll
define i32 @floating_mul_div(i32 %bits) {
	%value = bitcast i32 %bits to float
	%product = fmul float %value, %value
	%quotient = fdiv float %product, %value
	%result = bitcast float %quotient to i32
	ret i32 %result
}

;--- multiply-overflow.ll
declare { i32, i1 } @llvm.umul.with.overflow.i32(i32, i32)

define i32 @multiply_overflow(i32 %a, i32 %b) {
	%pair = call { i32, i1 } @llvm.umul.with.overflow.i32(i32 %a, i32 %b)
	%result = extractvalue { i32, i1 } %pair, 0
	ret i32 %result
}

;--- saturating.ll
declare i32 @llvm.smul.fix.sat.i32(i32, i32, i32 immarg)

define i32 @saturating_multiply(i32 %a, i32 %b) {
	%result = call i32 @llvm.smul.fix.sat.i32(i32 %a, i32 %b, i32 0)
	ret i32 %result
}
