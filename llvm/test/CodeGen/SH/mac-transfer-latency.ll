; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs %s -o - | FileCheck %s

; Read both product halves before starting the dependent carry chain, giving
; the first transfer time to complete while the second transfer issues.
define i64 @wide_product_add(i32 %a, i32 %b, i64 %accumulator) {
	%left = zext i32 %a to i64
	%right = zext i32 %b to i64
	%product = mul nuw i64 %left, %right
	%sum = add i64 %product, %accumulator
	ret i64 %sum
}

; CHECK-LABEL: wide_product_add:
; CHECK: dmulu.l
; CHECK: sts macl,[[LOW:r[0-9]+]]
; CHECK-NEXT: sts mach,[[HIGH:r[0-9]+]]
; CHECK-NEXT: addc {{r[0-9]+}},[[LOW]]
; CHECK: rts
; CHECK-NEXT: addc {{r[0-9]+}},[[HIGH]]
