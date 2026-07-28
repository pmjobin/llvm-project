; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i64.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=I64
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/funnel.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INTRINSIC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/ctpop.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INTRINSIC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/ctlz.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INTRINSIC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/cttz.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INTRINSIC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/overflow.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INTRINSIC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/borrow.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INTRINSIC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/minmax.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INTRINSIC
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VECTOR
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector-shift.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VECTOR
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/global-address.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/function-address.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/block-address.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS

; I64: LLVM ERROR: SH only supports i8, i16, and i32 integer operations
; INTRINSIC: LLVM ERROR: SH intrinsics are not supported
; VECTOR: LLVM ERROR: SH only supports i8, i16, and i32 integer operations
; ADDRESS: LLVM ERROR: SH global, function, and block address constants are not supported

;--- i64.ll
define i32 @wide_alu(i32 %a, i32 %b) {
	%wide_a = zext i32 %a to i64
	%wide_b = zext i32 %b to i64
	%wide = xor i64 %wide_a, %wide_b
	%result = trunc i64 %wide to i32
	ret i32 %result
}

;--- funnel.ll
declare i32 @llvm.fshl.i32(i32, i32, i32)

define i32 @rotate_left(i32 %value, i32 %count) {
	%result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 %count)
	ret i32 %result
}

;--- ctpop.ll
declare i32 @llvm.ctpop.i32(i32)

define i32 @population_count(i32 %value) {
	%result = call i32 @llvm.ctpop.i32(i32 %value)
	ret i32 %result
}

;--- ctlz.ll
declare i32 @llvm.ctlz.i32(i32, i1 immarg)

define i32 @leading_zero_count(i32 %value) {
	%result = call i32 @llvm.ctlz.i32(i32 %value, i1 false)
	ret i32 %result
}

;--- cttz.ll
declare i32 @llvm.cttz.i32(i32, i1 immarg)

define i32 @trailing_zero_count(i32 %value) {
	%result = call i32 @llvm.cttz.i32(i32 %value, i1 false)
	ret i32 %result
}

;--- overflow.ll
declare { i32, i1 } @llvm.uadd.with.overflow.i32(i32, i32)

define i32 @carry_value(i32 %a, i32 %b) {
	%pair = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
	%result = extractvalue { i32, i1 } %pair, 0
	ret i32 %result
}

;--- borrow.ll
declare { i32, i1 } @llvm.usub.with.overflow.i32(i32, i32)

define i32 @borrow_value(i32 %a, i32 %b) {
	%pair = call { i32, i1 } @llvm.usub.with.overflow.i32(i32 %a, i32 %b)
	%result = extractvalue { i32, i1 } %pair, 0
	ret i32 %result
}

;--- minmax.ll
declare i32 @llvm.smax.i32(i32, i32)

define i32 @signed_maximum(i32 %a, i32 %b) {
	%result = call i32 @llvm.smax.i32(i32 %a, i32 %b)
	ret i32 %result
}

;--- vector.ll
define i32 @vector_logical(i32 %a, i32 %b) {
	%vector_a = insertelement <2 x i32> poison, i32 %a, i32 0
	%vector_b = insertelement <2 x i32> poison, i32 %b, i32 0
	%logical = and <2 x i32> %vector_a, %vector_b
	%result = extractelement <2 x i32> %logical, i32 0
	ret i32 %result
}

;--- vector-shift.ll
define i32 @vector_shift(i32 %value, i32 %count) {
	%vector_value = insertelement <2 x i32> poison, i32 %value, i32 0
	%vector_count = insertelement <2 x i32> poison, i32 %count, i32 0
	%shifted = shl <2 x i32> %vector_value, %vector_count
	%result = extractelement <2 x i32> %shifted, i32 0
	ret i32 %result
}

;--- global-address.ll
@integer_global = global i32 0, align 4

define i32 @global_address() {
	ret i32 ptrtoint (ptr @integer_global to i32)
}

;--- function-address.ll
define i32 @function_address() {
	ret i32 ptrtoint (ptr @function_address to i32)
}

;--- block-address.ll
define void @block_address(ptr %destination_slot) {
entry:
	store volatile ptr blockaddress(@block_address, %destination), ptr %destination_slot
	br label %destination

destination:
	ret void
}
