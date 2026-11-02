; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs < %s | FileCheck %s
; Native memory/type legalization splits exact byte-sized scalar values.
; Runtime boundary/whole-value coverage uses these exact functions.

; CHECK-LABEL: load_u24:
; CHECK: mov.b
define i32 @load_u24(ptr %p) {
  %v = load i24, ptr %p, align 1
  %r = zext i24 %v to i32
  ret i32 %r
}

; CHECK-LABEL: load_s24:
; CHECK: mov.b
define i32 @load_s24(ptr %p) {
  %v = load i24, ptr %p, align 1
  %r = sext i24 %v to i32
  ret i32 %r
}

; CHECK-LABEL: store_u24:
; CHECK: mov.b
define void @store_u24(ptr %p, i32 %x) {
  %v = trunc i32 %x to i24
  store i24 %v, ptr %p, align 1
  ret void
}

; CHECK-LABEL: load_u24_a4:
; CHECK: mov.b
define i32 @load_u24_a4(ptr %p) {
  %v = load i24, ptr %p, align 4
  %r = zext i24 %v to i32
  ret i32 %r
}

; CHECK-LABEL: load_s24_a4:
; CHECK: mov.b
define i32 @load_s24_a4(ptr %p) {
  %v = load i24, ptr %p, align 4
  %r = sext i24 %v to i32
  ret i32 %r
}

; CHECK-LABEL: store_u24_a4:
; CHECK: mov.b
define void @store_u24_a4(ptr %p, i32 %x) {
  %v = trunc i32 %x to i24
  store i24 %v, ptr %p, align 4
  ret void
}

; CHECK-LABEL: load_u40:
; CHECK: mov.b
define i64 @load_u40(ptr %p) {
  %v = load i40, ptr %p, align 1
  %r = zext i40 %v to i64
  ret i64 %r
}

; CHECK-LABEL: load_s40:
; CHECK: mov.b
define i64 @load_s40(ptr %p) {
  %v = load i40, ptr %p, align 1
  %r = sext i40 %v to i64
  ret i64 %r
}

; CHECK-LABEL: store_u40:
; CHECK: mov.b
define void @store_u40(ptr %p, i64 %x) {
  %v = trunc i64 %x to i40
  store i40 %v, ptr %p, align 1
  ret void
}

; CHECK-LABEL: load_u40_a4:
; CHECK: mov.b
define i64 @load_u40_a4(ptr %p) {
  %v = load i40, ptr %p, align 4
  %r = zext i40 %v to i64
  ret i64 %r
}

; CHECK-LABEL: load_s40_a4:
; CHECK: mov.b
define i64 @load_s40_a4(ptr %p) {
  %v = load i40, ptr %p, align 4
  %r = sext i40 %v to i64
  ret i64 %r
}

; CHECK-LABEL: store_u40_a4:
; CHECK: mov.b
define void @store_u40_a4(ptr %p, i64 %x) {
  %v = trunc i64 %x to i40
  store i40 %v, ptr %p, align 4
  ret void
}

; CHECK-LABEL: load_u48:
; CHECK: mov.b
define i64 @load_u48(ptr %p) {
  %v = load i48, ptr %p, align 1
  %r = zext i48 %v to i64
  ret i64 %r
}

; CHECK-LABEL: load_s48:
; CHECK: mov.b
define i64 @load_s48(ptr %p) {
  %v = load i48, ptr %p, align 1
  %r = sext i48 %v to i64
  ret i64 %r
}

; CHECK-LABEL: store_u48:
; CHECK: mov.b
define void @store_u48(ptr %p, i64 %x) {
  %v = trunc i64 %x to i48
  store i48 %v, ptr %p, align 1
  ret void
}

; CHECK-LABEL: load_u48_a4:
; CHECK-DAG: mov.l
; CHECK-DAG: mov.w
define i64 @load_u48_a4(ptr %p) {
  %v = load i48, ptr %p, align 4
  %r = zext i48 %v to i64
  ret i64 %r
}

; CHECK-LABEL: load_s48_a4:
; CHECK-DAG: mov.l
; CHECK-DAG: mov.w
define i64 @load_s48_a4(ptr %p) {
  %v = load i48, ptr %p, align 4
  %r = sext i48 %v to i64
  ret i64 %r
}

; CHECK-LABEL: store_u48_a4:
; CHECK-DAG: mov.l
; CHECK-DAG: mov.w
define void @store_u48_a4(ptr %p, i64 %x) {
  %v = trunc i64 %x to i48
  store i48 %v, ptr %p, align 4
  ret void
}

; CHECK-LABEL: load_u56:
; CHECK: mov.b
define i64 @load_u56(ptr %p) {
  %v = load i56, ptr %p, align 1
  %r = zext i56 %v to i64
  ret i64 %r
}

; CHECK-LABEL: load_s56:
; CHECK: mov.b
define i64 @load_s56(ptr %p) {
  %v = load i56, ptr %p, align 1
  %r = sext i56 %v to i64
  ret i64 %r
}

; CHECK-LABEL: store_u56:
; CHECK: mov.b
define void @store_u56(ptr %p, i64 %x) {
  %v = trunc i64 %x to i56
  store i56 %v, ptr %p, align 1
  ret void
}

; CHECK-LABEL: load_u56_a4:
; CHECK: mov.b
define i64 @load_u56_a4(ptr %p) {
  %v = load i56, ptr %p, align 4
  %r = zext i56 %v to i64
  ret i64 %r
}

; CHECK-LABEL: load_s56_a4:
; CHECK: mov.b
define i64 @load_s56_a4(ptr %p) {
  %v = load i56, ptr %p, align 4
  %r = sext i56 %v to i64
  ret i64 %r
}

; CHECK-LABEL: store_u56_a4:
; CHECK: mov.b
define void @store_u56_a4(ptr %p, i64 %x) {
  %v = trunc i64 %x to i56
  store i56 %v, ptr %p, align 4
  ret void
}
