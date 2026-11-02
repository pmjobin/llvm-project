; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llvm-as %s -o %t.bc
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null

declare i32 @llvm.ctlz.i32(i32, i1 immarg)
declare i32 @llvm.cttz.i32(i32, i1 immarg)
declare i32 @llvm.ctpop.i32(i32)
declare i32 @llvm.bswap.i32(i32)
declare i32 @llvm.bitreverse.i32(i32)
declare i32 @llvm.smin.i32(i32, i32)
declare i32 @llvm.smax.i32(i32, i32)
declare i32 @llvm.umin.i32(i32, i32)
declare i32 @llvm.umax.i32(i32, i32)
declare i32 @llvm.abs.i32(i32, i1 immarg)
declare i64 @llvm.abs.i64(i64, i1 immarg)
declare i32 @llvm.sadd.sat.i32(i32, i32)
declare i32 @llvm.ssub.sat.i32(i32, i32)
declare i32 @llvm.uadd.sat.i32(i32, i32)
declare i32 @llvm.usub.sat.i32(i32, i32)
declare i64 @llvm.bswap.i64(i64)
declare i64 @llvm.ctlz.i64(i64, i1 immarg)
declare i64 @llvm.ctpop.i64(i64)

define i32 @leading_zero_count(i32 %value) {
; CHECK-LABEL: leading_zero_count:
; CHECK: mul.l
; CHECK: rts
	%result = call i32 @llvm.ctlz.i32(i32 %value, i1 false)
	ret i32 %result
}

define i32 @trailing_zero_count(i32 %value) {
; CHECK-LABEL: trailing_zero_count:
; CHECK: mul.l
; CHECK: rts
	%result = call i32 @llvm.cttz.i32(i32 %value, i1 false)
	ret i32 %result
}

define i32 @population_count(i32 %value) {
; CHECK-LABEL: population_count:
; CHECK: mul.l
; CHECK: rts
	%result = call i32 @llvm.ctpop.i32(i32 %value)
	ret i32 %result
}

define i32 @byte_swap(i32 %value) {
; CHECK-LABEL: byte_swap:
; CHECK: rts
	%result = call i32 @llvm.bswap.i32(i32 %value)
	ret i32 %result
}

define i32 @bit_reverse(i32 %value) {
; CHECK-LABEL: bit_reverse:
; CHECK: rts
	%result = call i32 @llvm.bitreverse.i32(i32 %value)
	ret i32 %result
}

define i32 @signed_minimum(i32 %a, i32 %b) {
; CHECK-LABEL: signed_minimum:
; CHECK: rts
	%result = call i32 @llvm.smin.i32(i32 %a, i32 %b)
	ret i32 %result
}

define i32 @signed_maximum(i32 %a, i32 %b) {
; CHECK-LABEL: signed_maximum:
; CHECK: rts
	%result = call i32 @llvm.smax.i32(i32 %a, i32 %b)
	ret i32 %result
}

define i32 @unsigned_minimum(i32 %a, i32 %b) {
; CHECK-LABEL: unsigned_minimum:
; CHECK: rts
	%result = call i32 @llvm.umin.i32(i32 %a, i32 %b)
	ret i32 %result
}

define i32 @unsigned_maximum(i32 %a, i32 %b) {
; CHECK-LABEL: unsigned_maximum:
; CHECK: rts
	%result = call i32 @llvm.umax.i32(i32 %a, i32 %b)
	ret i32 %result
}

define i32 @absolute_i32(i32 %value) {
; CHECK-LABEL: absolute_i32:
; CHECK: rts
	%result = call i32 @llvm.abs.i32(i32 %value, i1 true)
	ret i32 %result
}

define i64 @absolute_i64(i64 %value) {
; CHECK-LABEL: absolute_i64:
; CHECK: rts
	%result = call i64 @llvm.abs.i64(i64 %value, i1 true)
	ret i64 %result
}

define i32 @signed_saturating_add(i32 %lhs, i32 %rhs) {
; CHECK-LABEL: signed_saturating_add:
; CHECK: rts
	%result = call i32 @llvm.sadd.sat.i32(i32 %lhs, i32 %rhs)
	ret i32 %result
}

define i32 @signed_saturating_subtract(i32 %lhs, i32 %rhs) {
; CHECK-LABEL: signed_saturating_subtract:
; CHECK: rts
	%result = call i32 @llvm.ssub.sat.i32(i32 %lhs, i32 %rhs)
	ret i32 %result
}

define i32 @unsigned_saturating_add(i32 %lhs, i32 %rhs) {
; CHECK-LABEL: unsigned_saturating_add:
; CHECK: rts
	%result = call i32 @llvm.uadd.sat.i32(i32 %lhs, i32 %rhs)
	ret i32 %result
}

define i32 @unsigned_saturating_subtract(i32 %lhs, i32 %rhs) {
; CHECK-LABEL: unsigned_saturating_subtract:
; CHECK: rts
	%result = call i32 @llvm.usub.sat.i32(i32 %lhs, i32 %rhs)
	ret i32 %result
}

define i64 @byte_swap_i64(i64 %value) {
; CHECK-LABEL: byte_swap_i64:
; CHECK: rts
	%result = call i64 @llvm.bswap.i64(i64 %value)
	ret i64 %result
}

define i64 @leading_zero_count_i64(i64 %value) {
; CHECK-LABEL: leading_zero_count_i64:
; CHECK: rts
	%result = call i64 @llvm.ctlz.i64(i64 %value, i1 false)
	ret i64 %result
}

define i64 @population_count_i64(i64 %value) {
; CHECK-LABEL: population_count_i64:
; CHECK: rts
	%result = call i64 @llvm.ctpop.i64(i64 %value)
	ret i64 %result
}
