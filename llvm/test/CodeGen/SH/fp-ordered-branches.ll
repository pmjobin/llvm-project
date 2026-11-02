; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2e -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=F32
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2e -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=F32
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh3e -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=F32
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3e -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=F32
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2a -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=F32,F64
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2a -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=F32,F64
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=F32,F64
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=F32,F64
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4a -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=F32,F64
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4a -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=F32,F64
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mtune=st40-100 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=F32,F64
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mtune=st40-300 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=F32,F64
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=UNORDERED

; F32-LABEL: ordered_ge_f32:
; F32: fcmp/gt [[GE32_A:fr[0-9]+]],[[GE32_B:fr[0-9]+]]
; F32-NEXT: bt
; F32: fcmp/eq [[GE32_A]],[[GE32_B]]
; F32-NEXT: bt

define i32 @ordered_ge_f32(float %lhs, float %rhs, i32 %true_value, i32 %false_value) {
entry:
	%condition = fcmp oge float %lhs, %rhs
	%result = select i1 %condition, i32 %true_value, i32 %false_value
	ret i32 %result
}

; F32-LABEL: ordered_le_f32:
; F32: fcmp/gt [[LE32_A:fr[0-9]+]],[[LE32_B:fr[0-9]+]]
; F32-NEXT: bt
; F32: fcmp/eq [[LE32_A]],[[LE32_B]]
; F32-NEXT: bt

define i32 @ordered_le_f32(float %lhs, float %rhs, i32 %true_value, i32 %false_value) {
entry:
	%condition = fcmp ole float %lhs, %rhs
	br i1 %condition, label %true, label %false

true:
	ret i32 %true_value

false:
	ret i32 %false_value
}

; F64-LABEL: ordered_ge_f64:
; F64: fcmp/gt [[GE64_A:dr[0-9]+]],[[GE64_B:dr[0-9]+]]
; F64-NEXT: bt
; F64: fcmp/eq [[GE64_A]],[[GE64_B]]
; F64-NEXT: bt

define i32 @ordered_ge_f64(double %lhs, double %rhs, i32 %true_value, i32 %false_value) {
entry:
	%condition = fcmp oge double %lhs, %rhs
	br i1 %condition, label %true, label %false

true:
	ret i32 %true_value

false:
	ret i32 %false_value
}

; F64-LABEL: ordered_le_f64:
; F64: fcmp/gt [[LE64_A:dr[0-9]+]],[[LE64_B:dr[0-9]+]]
; F64-NEXT: bt
; F64: fcmp/eq [[LE64_A]],[[LE64_B]]
; F64-NEXT: bt

define i32 @ordered_le_f64(double %lhs, double %rhs, i32 %true_value, i32 %false_value) {
entry:
	%condition = fcmp ole double %lhs, %rhs
	br i1 %condition, label %true, label %false

true:
	ret i32 %true_value

false:
	ret i32 %false_value
}

; F32-LABEL: ordered_ge_f32_inverted:
; F32: fcmp/gt [[IGE32_A:fr[0-9]+]],[[IGE32_B:fr[0-9]+]]
; F32-NEXT: bt
; F32: fcmp/eq [[IGE32_A]],[[IGE32_B]]
; F32-NEXT: bt

define i32 @ordered_ge_f32_inverted(float %lhs, float %rhs, i32 %true_value, i32 %false_value) {
entry:
	%condition = fcmp oge float %lhs, %rhs
	br i1 %condition, label %false, label %true

true:
	ret i32 %true_value

false:
	ret i32 %false_value
}

; F64-LABEL: ordered_le_f64_inverted:
; F64: fcmp/gt [[ILE64_A:dr[0-9]+]],[[ILE64_B:dr[0-9]+]]
; F64-NEXT: bt
; F64: fcmp/eq [[ILE64_A]],[[ILE64_B]]
; F64-NEXT: bt

define i32 @ordered_le_f64_inverted(double %lhs, double %rhs, i32 %true_value, i32 %false_value) {
entry:
	%condition = fcmp ole double %lhs, %rhs
	br i1 %condition, label %false, label %true

true:
	ret i32 %true_value

false:
	ret i32 %false_value
}

; The unordered predicates must retain their explicit T-bit materialization;
; NaNs take the true edge for these comparisons.

; UNORDERED-LABEL: unordered_ge_f32:
; UNORDERED: fcmp/gt
; UNORDERED-NEXT: movt

define i32 @unordered_ge_f32(float %lhs, float %rhs, i32 %true_value, i32 %false_value) {
entry:
	%condition = fcmp uge float %lhs, %rhs
	%result = select i1 %condition, i32 %true_value, i32 %false_value
	ret i32 %result
}

; UNORDERED-LABEL: unordered_le_f64:
; UNORDERED: fcmp/gt
; UNORDERED-NEXT: movt

define i32 @unordered_le_f64(double %lhs, double %rhs, i32 %true_value, i32 %false_value) {
entry:
	%condition = fcmp ule double %lhs, %rhs
	%result = select i1 %condition, i32 %true_value, i32 %false_value
	ret i32 %result
}
