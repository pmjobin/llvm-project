; RUN: opt %s -o %t.full.bc
; RUN: llvm-lto -O2 -j2 -exported-symbol=multiply_word -exported-symbol=multiply_full -exported-symbol=multiply_high -exported-symbol=divide_size -exported-symbol=remainder_size -o %t.full.o %t.full.bc
; RUN: llvm-readobj --file-headers %t.full.o.0 | FileCheck %s --check-prefix=FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t.full.o.0 %t.full.o.1 | FileCheck %s --check-prefix=CODE
; RUN: opt -module-summary %s -o %t.thin.bc
; RUN: llvm-lto -O2 -j2 --thinlto-action=run -exported-symbol=multiply_word -exported-symbol=multiply_full -exported-symbol=multiply_high -exported-symbol=divide_size -exported-symbol=remainder_size %t.thin.bc
; RUN: llvm-readobj --file-headers %t.thin.bc.thinlto.o | FileCheck %s --check-prefix=FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t.thin.bc.thinlto.o | FileCheck %s --check-prefix=CODE

; FLAGS: Flags [ (0x9)

; CODE-DAG: mulu.w
; CODE-DAG: dmuls.l
; CODE-DAG: dmulu.l
; CODE-DAG: sts macl,
; CODE-DAG: sts mach,
; CODE-DAG: jsr
; CODE-DAG: R_SH_DIR32 __divsi3
; CODE-DAG: R_SH_DIR32 __umodsi3

target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-linux-gnu"

define i32 @multiply_word(i32 %a, i32 %b) #0 {
	%left = and i32 %a, 65535
	%right = and i32 %b, 65535
	%product = mul i32 %left, %right
	ret i32 %product
}

define i64 @multiply_full(i32 %a, i32 %b) #0 {
	%left = sext i32 %a to i64
	%right = sext i32 %b to i64
	%product = mul i64 %left, %right
	ret i64 %product
}

define i32 @multiply_high(i32 %a, i32 %b) #0 {
	%left = zext i32 %a to i64
	%right = zext i32 %b to i64
	%product = mul i64 %left, %right
	%shifted = lshr i64 %product, 32
	%high = trunc i64 %shifted to i32
	ret i32 %high
}

define i32 @divide_size(i32 %dividend, i32 %divisor) #1 {
	%quotient = sdiv i32 %dividend, %divisor
	ret i32 %quotient
}

define i32 @remainder_size(i32 %dividend, i32 %divisor) #1 {
	%remainder = urem i32 %dividend, %divisor
	ret i32 %remainder
}

attributes #0 = { noinline "target-abi"="gnu-hard-float" "target-cpu"="sh4" }
attributes #1 = { noinline optsize "target-abi"="gnu-hard-float" "target-cpu"="sh4" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}
