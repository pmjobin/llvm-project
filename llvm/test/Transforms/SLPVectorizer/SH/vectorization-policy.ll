; RUN: opt -passes=slp-vectorizer -mtriple=shle-unknown-none-elf -mcpu=sh4 -S %s | FileCheck %s --check-prefix=SLP
; RUN: opt -passes=slp-vectorizer -mtriple=shle-unknown-none-elf -mcpu=sh4 %s -o - | llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -o /dev/null

; SH inherits the generic SLP policy because scalarized vector transforms can
; improve code. The resulting vector division must remain compilable.
define i32 @vector_division(i32 %value) optsize {
; SLP-LABEL: define i32 @vector_division(
; SLP: sdiv <2 x i32>
	%zero = sdiv i32 0, 2
	%result = sdiv i32 %value, 2
	ret i32 %result
}
