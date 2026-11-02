; REQUIRES: sh-registered-target, x86-registered-target
; RUN: opt -passes=inline -S -mtriple=shle-unknown-linux-gnu -mcpu=sh4 %s | FileCheck %s --check-prefixes=CHECK,SH,SIZE
; RUN: opt -passes=inline -S -mtriple=sh-unknown-none-elf -mcpu=sh2 %s | FileCheck %s --check-prefixes=CHECK,SH,SIZE
; RUN: opt -passes=inline -S -mtriple=x86_64-unknown-linux-gnu %s | FileCheck %s --check-prefixes=CHECK,DEFAULT,SIZE
; RUN: opt -passes=inline -S -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -inline-threshold=100 %s | FileCheck %s --check-prefixes=CHECK,DEFAULT,OVERRIDE
;
; Sixteen multiplies cost 45 after removing call overhead. The single-block
; bonus raises ordinary optsize's base threshold 50 to 75, and SH's 25 to 37.
; Target policy belongs to the caller's optsize decision, not the callee's
; attributes. MinSize, explicit overrides, hints and last-call elimination keep
; their existing policies; X86 checks the default shared implementation.

define i32 @callee(i32 %x)  {
  %v0 = mul i32 %x, %x
  %v1 = mul i32 %v0, %x
  %v2 = mul i32 %v1, %x
  %v3 = mul i32 %v2, %x
  %v4 = mul i32 %v3, %x
  %v5 = mul i32 %v4, %x
  %v6 = mul i32 %v5, %x
  %v7 = mul i32 %v6, %x
  %v8 = mul i32 %v7, %x
  %v9 = mul i32 %v8, %x
  %v10 = mul i32 %v9, %x
  %v11 = mul i32 %v10, %x
  %v12 = mul i32 %v11, %x
  %v13 = mul i32 %v12, %x
  %v14 = mul i32 %v13, %x
  %v15 = mul i32 %v14, %x
  ret i32 %v15
}

define i32 @size_callee(i32 %x) optsize {
  %v0 = mul i32 %x, %x
  %v1 = mul i32 %v0, %x
  %v2 = mul i32 %v1, %x
  %v3 = mul i32 %v2, %x
  %v4 = mul i32 %v3, %x
  %v5 = mul i32 %v4, %x
  %v6 = mul i32 %v5, %x
  %v7 = mul i32 %v6, %x
  %v8 = mul i32 %v7, %x
  %v9 = mul i32 %v8, %x
  %v10 = mul i32 %v9, %x
  %v11 = mul i32 %v10, %x
  %v12 = mul i32 %v11, %x
  %v13 = mul i32 %v12, %x
  %v14 = mul i32 %v13, %x
  %v15 = mul i32 %v14, %x
  ret i32 %v15
}

define i32 @hinted_callee(i32 %x) inlinehint {
  %v0 = mul i32 %x, %x
  %v1 = mul i32 %v0, %x
  %v2 = mul i32 %v1, %x
  %v3 = mul i32 %v2, %x
  %v4 = mul i32 %v3, %x
  %v5 = mul i32 %v4, %x
  %v6 = mul i32 %v5, %x
  %v7 = mul i32 %v6, %x
  %v8 = mul i32 %v7, %x
  %v9 = mul i32 %v8, %x
  %v10 = mul i32 %v9, %x
  %v11 = mul i32 %v10, %x
  %v12 = mul i32 %v11, %x
  %v13 = mul i32 %v12, %x
  %v14 = mul i32 %v13, %x
  %v15 = mul i32 %v14, %x
  ret i32 %v15
}

define i32 @always_callee(i32 %x) alwaysinline {
  %v0 = mul i32 %x, %x
  %v1 = mul i32 %v0, %x
  %v2 = mul i32 %v1, %x
  %v3 = mul i32 %v2, %x
  %v4 = mul i32 %v3, %x
  %v5 = mul i32 %v4, %x
  %v6 = mul i32 %v5, %x
  %v7 = mul i32 %v6, %x
  %v8 = mul i32 %v7, %x
  %v9 = mul i32 %v8, %x
  %v10 = mul i32 %v9, %x
  %v11 = mul i32 %v10, %x
  %v12 = mul i32 %v11, %x
  %v13 = mul i32 %v12, %x
  %v14 = mul i32 %v13, %x
  %v15 = mul i32 %v14, %x
  ret i32 %v15
}

define internal i32 @local_callee(i32 %x) optsize {
  %v0 = mul i32 %x, %x
  %v1 = mul i32 %v0, %x
  %v2 = mul i32 %v1, %x
  %v3 = mul i32 %v2, %x
  %v4 = mul i32 %v3, %x
  %v5 = mul i32 %v4, %x
  %v6 = mul i32 %v5, %x
  %v7 = mul i32 %v6, %x
  %v8 = mul i32 %v7, %x
  %v9 = mul i32 %v8, %x
  %v10 = mul i32 %v9, %x
  %v11 = mul i32 %v10, %x
  %v12 = mul i32 %v11, %x
  %v13 = mul i32 %v12, %x
  %v14 = mul i32 %v13, %x
  %v15 = mul i32 %v14, %x
  ret i32 %v15
}

; CHECK-LABEL: define i32 @caller_os(
; SH: call i32 @callee(
; DEFAULT-NOT: call
; CHECK: ret i32
define i32 @caller_os(i32 %x) optsize {
  %r = call i32 @callee(i32 %x)
  ret i32 %r
}

; CHECK-LABEL: define i32 @caller_oz(
; SIZE: call i32 @callee(
; OVERRIDE-NOT: call
; CHECK: ret i32
define i32 @caller_oz(i32 %x) minsize optsize {
  %r = call i32 @callee(i32 %x)
  ret i32 %r
}

; CHECK-LABEL: define i32 @caller_normal(
; CHECK-NOT: call
; CHECK: ret i32
define i32 @caller_normal(i32 %x)  {
  %r = call i32 @callee(i32 %x)
  ret i32 %r
}

; CHECK-LABEL: define i32 @caller_of_size_callee(
; CHECK-NOT: call
; CHECK: ret i32
define i32 @caller_of_size_callee(i32 %x)  {
  %r = call i32 @size_callee(i32 %x)
  ret i32 %r
}

; CHECK-LABEL: define i32 @caller_hinted_os(
; CHECK-NOT: call
; CHECK: ret i32
define i32 @caller_hinted_os(i32 %x) optsize {
  %r = call i32 @hinted_callee(i32 %x)
  ret i32 %r
}

; CHECK-LABEL: define i32 @caller_always_os(
; CHECK-NOT: call
; CHECK: ret i32
define i32 @caller_always_os(i32 %x) optsize {
  %r = call i32 @always_callee(i32 %x)
  ret i32 %r
}

; CHECK-LABEL: define i32 @caller_last_os(
; CHECK-NOT: call
; CHECK: ret i32
define i32 @caller_last_os(i32 %x) optsize {
  %r = call i32 @local_callee(i32 %x)
  ret i32 %r
}
