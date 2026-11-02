; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=STATIC
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs < %s | FileCheck %s --check-prefix=STATIC
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -relocation-model=pic -verify-machineinstrs < %s | FileCheck %s --check-prefix=PIC
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -relocation-model=pic -verify-machineinstrs < %s | FileCheck %s --check-prefix=PIC
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=obj -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -filetype=obj -verify-machineinstrs < %s -o /dev/null

; STATIC-DAG: .long __divdi3
; STATIC-DAG: .long __udivdi3
; STATIC-DAG: .long __moddi3
; STATIC-DAG: .long __umoddi3
; PIC-DAG: .long __divdi3@PLT
; PIC-DAG: .long __udivdi3@PLT
; PIC-DAG: .long __moddi3@PLT
; PIC-DAG: .long __umoddi3@PLT

define i64 @mul_i64(i64 %a, i64 %b) {
	; Generic legalization uses ordinary SH-2 integer multiply instructions.
	%result = mul i64 %a, %b
	ret i64 %result
}

define i64 @sdiv_i64(i64 %a, i64 %b) {
	%result = sdiv i64 %a, %b
	ret i64 %result
}

define i64 @udiv_i64(i64 %a, i64 %b) {
	%result = udiv i64 %a, %b
	ret i64 %result
}

define i64 @srem_i64(i64 %a, i64 %b) {
	%result = srem i64 %a, %b
	ret i64 %result
}

define i64 @urem_i64(i64 %a, i64 %b) {
	%result = urem i64 %a, %b
	ret i64 %result
}

declare { i64, i1 } @llvm.umul.with.overflow.i64(i64, i64)
declare { i64, i1 } @llvm.smul.with.overflow.i64(i64, i64)

define i64 @umul_overflow_i64(i64 %a, i64 %b, ptr %overflow) {
	%pair = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 %a, i64 %b)
	%result = extractvalue { i64, i1 } %pair, 0
	%flag = extractvalue { i64, i1 } %pair, 1
	store i1 %flag, ptr %overflow, align 1
	ret i64 %result
}

define i64 @smul_overflow_i64(i64 %a, i64 %b, ptr %overflow) {
	%pair = call { i64, i1 } @llvm.smul.with.overflow.i64(i64 %a, i64 %b)
	%result = extractvalue { i64, i1 } %pair, 0
	%flag = extractvalue { i64, i1 } %pair, 1
	store i1 %flag, ptr %overflow, align 1
	ret i64 %result
}
