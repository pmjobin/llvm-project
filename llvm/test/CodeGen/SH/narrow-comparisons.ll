; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s

define i32 @cmp_eq(ptr %bp, ptr %bq, ptr %wp, ptr %wq) {
; CHECK-LABEL: cmp_eq:
; CHECK: mov.b
; CHECK: mov.b
; CHECK: cmp/eq
; CHECK: b{{[tf]}}
; CHECK: mov.w
; CHECK: mov.w
; CHECK: cmp/eq
; CHECK: b{{[tf]}}
	%ba = load i8, ptr %bp, align 1
	%bb = load i8, ptr %bq, align 1
	%bc = icmp eq i8 %ba, %bb
	br i1 %bc, label %word, label %false
word:
	%wa = load i16, ptr %wp, align 2
	%wb = load i16, ptr %wq, align 2
	%wc = icmp eq i16 %wa, %wb
	br i1 %wc, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @cmp_ne(ptr %bp, ptr %bq, ptr %wp, ptr %wq) {
; CHECK-LABEL: cmp_ne:
; CHECK: mov.b
; CHECK: mov.b
; CHECK: cmp/eq
; CHECK: b{{[tf]}}
; CHECK: mov.w
; CHECK: mov.w
; CHECK: cmp/eq
; CHECK: b{{[tf]}}
	%ba = load i8, ptr %bp, align 1
	%bb = load i8, ptr %bq, align 1
	%bc = icmp ne i8 %ba, %bb
	br i1 %bc, label %word, label %false
word:
	%wa = load i16, ptr %wp, align 2
	%wb = load i16, ptr %wq, align 2
	%wc = icmp ne i16 %wa, %wb
	br i1 %wc, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @cmp_sgt(ptr %bp, ptr %bq, ptr %wp, ptr %wq) {
; CHECK-LABEL: cmp_sgt:
; CHECK: mov.b
; CHECK: mov.b
; CHECK: cmp/gt
; CHECK: b{{[tf]}}
; CHECK: mov.w
; CHECK: mov.w
; CHECK: cmp/gt
; CHECK: b{{[tf]}}
	%ba = load i8, ptr %bp, align 1
	%bb = load i8, ptr %bq, align 1
	%bc = icmp sgt i8 %ba, %bb
	br i1 %bc, label %word, label %false
word:
	%wa = load i16, ptr %wp, align 2
	%wb = load i16, ptr %wq, align 2
	%wc = icmp sgt i16 %wa, %wb
	br i1 %wc, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @cmp_sge(ptr %bp, ptr %bq, ptr %wp, ptr %wq) {
; CHECK-LABEL: cmp_sge:
; CHECK: mov.b
; CHECK: mov.b
; CHECK: cmp/ge
; CHECK: b{{[tf]}}
; CHECK: mov.w
; CHECK: mov.w
; CHECK: cmp/ge
; CHECK: b{{[tf]}}
	%ba = load i8, ptr %bp, align 1
	%bb = load i8, ptr %bq, align 1
	%bc = icmp sge i8 %ba, %bb
	br i1 %bc, label %word, label %false
word:
	%wa = load i16, ptr %wp, align 2
	%wb = load i16, ptr %wq, align 2
	%wc = icmp sge i16 %wa, %wb
	br i1 %wc, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @cmp_slt(ptr %bp, ptr %bq, ptr %wp, ptr %wq) {
; CHECK-LABEL: cmp_slt:
; CHECK: mov.b
; CHECK: mov.b
; CHECK: cmp/ge
; CHECK: b{{[tf]}}
; CHECK: mov.w
; CHECK: mov.w
; CHECK: cmp/ge
; CHECK: b{{[tf]}}
	%ba = load i8, ptr %bp, align 1
	%bb = load i8, ptr %bq, align 1
	%bc = icmp slt i8 %ba, %bb
	br i1 %bc, label %word, label %false
word:
	%wa = load i16, ptr %wp, align 2
	%wb = load i16, ptr %wq, align 2
	%wc = icmp slt i16 %wa, %wb
	br i1 %wc, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @cmp_sle(ptr %bp, ptr %bq, ptr %wp, ptr %wq) {
; CHECK-LABEL: cmp_sle:
; CHECK: mov.b
; CHECK: mov.b
; CHECK: cmp/gt
; CHECK: b{{[tf]}}
; CHECK: mov.w
; CHECK: mov.w
; CHECK: cmp/gt
; CHECK: b{{[tf]}}
	%ba = load i8, ptr %bp, align 1
	%bb = load i8, ptr %bq, align 1
	%bc = icmp sle i8 %ba, %bb
	br i1 %bc, label %word, label %false
word:
	%wa = load i16, ptr %wp, align 2
	%wb = load i16, ptr %wq, align 2
	%wc = icmp sle i16 %wa, %wb
	br i1 %wc, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @cmp_ugt(ptr %bp, ptr %bq, ptr %wp, ptr %wq) {
; CHECK-LABEL: cmp_ugt:
; CHECK-COUNT-2: extu.b
; CHECK: cmp/hi
; CHECK: b{{[tf]}}
; CHECK-COUNT-2: extu.w
; CHECK: cmp/hi
; CHECK: b{{[tf]}}
	%ba = load i8, ptr %bp, align 1
	%bb = load i8, ptr %bq, align 1
	%bc = icmp ugt i8 %ba, %bb
	br i1 %bc, label %word, label %false
word:
	%wa = load i16, ptr %wp, align 2
	%wb = load i16, ptr %wq, align 2
	%wc = icmp ugt i16 %wa, %wb
	br i1 %wc, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @cmp_uge(ptr %bp, ptr %bq, ptr %wp, ptr %wq) {
; CHECK-LABEL: cmp_uge:
; CHECK-COUNT-2: extu.b
; CHECK: cmp/hs
; CHECK: b{{[tf]}}
; CHECK-COUNT-2: extu.w
; CHECK: cmp/hs
; CHECK: b{{[tf]}}
	%ba = load i8, ptr %bp, align 1
	%bb = load i8, ptr %bq, align 1
	%bc = icmp uge i8 %ba, %bb
	br i1 %bc, label %word, label %false
word:
	%wa = load i16, ptr %wp, align 2
	%wb = load i16, ptr %wq, align 2
	%wc = icmp uge i16 %wa, %wb
	br i1 %wc, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @cmp_ult(ptr %bp, ptr %bq, ptr %wp, ptr %wq) {
; CHECK-LABEL: cmp_ult:
; CHECK-COUNT-2: extu.b
; CHECK: cmp/hs
; CHECK: b{{[tf]}}
; CHECK-COUNT-2: extu.w
; CHECK: cmp/hs
; CHECK: b{{[tf]}}
	%ba = load i8, ptr %bp, align 1
	%bb = load i8, ptr %bq, align 1
	%bc = icmp ult i8 %ba, %bb
	br i1 %bc, label %word, label %false
word:
	%wa = load i16, ptr %wp, align 2
	%wb = load i16, ptr %wq, align 2
	%wc = icmp ult i16 %wa, %wb
	br i1 %wc, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}

define i32 @cmp_ule(ptr %bp, ptr %bq, ptr %wp, ptr %wq) {
; CHECK-LABEL: cmp_ule:
; CHECK-COUNT-2: extu.b
; CHECK: cmp/hi
; CHECK: b{{[tf]}}
; CHECK-COUNT-2: extu.w
; CHECK: cmp/hi
; CHECK: b{{[tf]}}
	%ba = load i8, ptr %bp, align 1
	%bb = load i8, ptr %bq, align 1
	%bc = icmp ule i8 %ba, %bb
	br i1 %bc, label %word, label %false
word:
	%wa = load i16, ptr %wp, align 2
	%wb = load i16, ptr %wq, align 2
	%wc = icmp ule i16 %wa, %wb
	br i1 %wc, label %true, label %false
true:
	ret i32 1
false:
	ret i32 0
}
