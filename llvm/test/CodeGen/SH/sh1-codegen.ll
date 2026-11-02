; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh1 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=SH1 --implicit-check-not=mul.l --implicit-check-not=dmuls.l --implicit-check-not=dmulu.l --implicit-check-not=dt --implicit-check-not=mac.l --implicit-check-not=bf/s --implicit-check-not=bt/s --implicit-check-not=braf --implicit-check-not=bsrf --implicit-check-not=shad --implicit-check-not=shld --implicit-check-not=fadd
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh1 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=SH1 --implicit-check-not=mul.l --implicit-check-not=dmuls.l --implicit-check-not=dmulu.l --implicit-check-not=dt --implicit-check-not=mac.l --implicit-check-not=bf/s --implicit-check-not=bt/s --implicit-check-not=braf --implicit-check-not=bsrf --implicit-check-not=shad --implicit-check-not=shld --implicit-check-not=fadd
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh1 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh1 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh1 -O2 -filetype=obj -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh1 -O2 -filetype=obj -verify-machineinstrs %s -o /dev/null

declare i32 @llvm.sh.tas.b(ptr)
declare float @external_float(float)
declare i32 @external_integer(i32)

; SH1-LABEL: multiply:
; SH1: .long	__mulsi3
define i32 @multiply(i32 %left, i32 %right) {
	%result = mul i32 %left, %right
	ret i32 %result
}

; SH1-LABEL: multiply_high:
; SH1: .long	__muldi3
define i32 @multiply_high(i32 %left, i32 %right) {
	%wide.left = zext i32 %left to i64
	%wide.right = zext i32 %right to i64
	%product = mul i64 %wide.left, %wide.right
	%shifted = lshr i64 %product, 32
	%result = trunc i64 %shifted to i32
	ret i32 %result
}

; SH1-LABEL: divide:
; SH1: .long	__sdivsi3
define i32 @divide(i32 %left, i32 %right) {
	%result = sdiv i32 %left, %right
	ret i32 %result
}

; SH1-LABEL: remainder:
; SH1: .long	__udivsi3
; SH1: .long	__mulsi3
define i32 @remainder(i32 %left, i32 %right) {
	%result = urem i32 %left, %right
	ret i32 %result
}

; SH1-LABEL: variable_shift:
; SH1: .long	__ashlsi3
define i32 @variable_shift(i32 %value, i32 %count) {
	%result = shl i32 %value, %count
	ret i32 %result
}

; SH1-LABEL: compact_shift:
; SH1: .long	__ashlsi3
define i32 @compact_shift(i32 %value, i32 %count) optsize {
	%result = shl i32 %value, %count
	ret i32 %result
}

; SH1-LABEL: rotate:
; SH1-DAG: .long	__ashlsi3
; SH1-DAG: .long	__lshrsi3
define i32 @rotate(i32 %value, i32 %count) optsize {
	%complement = sub i32 0, %count
	%left = shl i32 %value, %count
	%right = lshr i32 %value, %complement
	%result = or disjoint i32 %left, %right
	ret i32 %result
}

; SH1-LABEL: soft_float:
; SH1-DAG: .long	__addsf3
; SH1-DAG: .long	__adddf3
define double @soft_float(float %single.left, float %single.right, double %double.left, double %double.right) {
	%single = fadd float %single.left, %single.right
	%wide.single = fpext float %single to double
	%double = fadd double %double.left, %double.right
	%result = fadd double %wide.single, %double
	ret double %result
}

; SH1-LABEL: direct_call:
; SH1: jsr	@
; SH1: .long	external_integer
define i32 @direct_call(i32 %value) {
	%result = call i32 @external_integer(i32 %value)
	ret i32 %result
}

; SH1-LABEL: indirect_call:
; SH1: jsr	@
define i32 @indirect_call(ptr %callee, i32 %value) {
	%result = call i32 %callee(i32 %value)
	ret i32 %result
}

; SH1-LABEL: atomic_tas:
; SH1: tas.b	@
define i32 @atomic_tas(ptr %address) {
	%result = call i32 @llvm.sh.tas.b(ptr %address)
	ret i32 %result
}
