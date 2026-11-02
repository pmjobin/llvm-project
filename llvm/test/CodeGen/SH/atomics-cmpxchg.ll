; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=atomic-expand < %s | FileCheck %s --check-prefix=EXPAND
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM

; EXPAND-LABEL: define i32 @strong4
; EXPAND: store i32 %expected, ptr [[EXPECTED:%.*]], align 4
; EXPAND: [[OK:%.*]] = call zeroext i1 @__atomic_compare_exchange_4(ptr %p, ptr [[EXPECTED]], i32 %desired, i32 0, i32 0)
; EXPAND: [[OLD:%.*]] = load i32, ptr [[EXPECTED]], align 4
; EXPAND: zext i1 %ok to i32
define i32 @strong4(ptr %p, i32 %expected, i32 %desired, ptr %old_out) {
  %pair = cmpxchg ptr %p, i32 %expected, i32 %desired monotonic monotonic, align 4
  %old = extractvalue { i32, i1 } %pair, 0
  %ok = extractvalue { i32, i1 } %pair, 1
  store i32 %old, ptr %old_out, align 4
  %flag = zext i1 %ok to i32
  ret i32 %flag
}

; EXPAND-LABEL: define i32 @weak4_branch
; EXPAND: call zeroext i1 @__atomic_compare_exchange_4(ptr %p, ptr {{.*}}, i32 %desired, i32 2, i32 2)
define i32 @weak4_branch(ptr %p, i32 %expected, i32 %desired) {
  %pair = cmpxchg weak ptr %p, i32 %expected, i32 %desired acquire acquire, align 4
  %ok = extractvalue { i32, i1 } %pair, 1
  br i1 %ok, label %yes, label %no

yes:
  ret i32 1

no:
  ret i32 0
}

; EXPAND-LABEL: define i64 @strong8
; EXPAND: call zeroext i1 @__atomic_compare_exchange_8(ptr %p, ptr {{.*}}, i64 %desired, i32 3, i32 0)
define i64 @strong8(ptr %p, i64 %expected, i64 %desired, ptr %ok_out) {
  %pair = cmpxchg ptr %p, i64 %expected, i64 %desired release monotonic, align 8
  %old = extractvalue { i64, i1 } %pair, 0
  %ok = extractvalue { i64, i1 } %pair, 1
  %flag = zext i1 %ok to i32
  store i32 %flag, ptr %ok_out, align 4
  ret i64 %old
}

; EXPAND-LABEL: define i64 @weak8
; EXPAND: call zeroext i1 @__atomic_compare_exchange_8(ptr %p, ptr {{.*}}, i64 %desired, i32 4, i32 2)
define i64 @weak8(ptr %p, i64 %expected, i64 %desired) {
  %pair = cmpxchg weak ptr %p, i64 %expected, i64 %desired acq_rel acquire, align 8
  %old = extractvalue { i64, i1 } %pair, 0
  ret i64 %old
}

; EXPAND-LABEL: define ptr @strong_ptr
; EXPAND: ptrtoint ptr %desired to i32
; EXPAND: call zeroext i1 @__atomic_compare_exchange_4(ptr %p, ptr {{.*}}, i32 {{.*}}, i32 5, i32 5)
; EXPAND: load ptr, ptr {{.*}}, align 4
define ptr @strong_ptr(ptr %p, ptr %expected, ptr %desired) {
  %pair = cmpxchg ptr %p, ptr %expected, ptr %desired seq_cst seq_cst, align 4
  %old = extractvalue { ptr, i1 } %pair, 0
  ret ptr %old
}

; EXPAND-LABEL: define i32 @weak_ptr
; EXPAND: call zeroext i1 @__atomic_compare_exchange_4(ptr %p, ptr {{.*}}, i32 {{.*}}, i32 5, i32 2)
define i32 @weak_ptr(ptr %p, ptr %expected, ptr %desired) {
  %pair = cmpxchg weak ptr %p, ptr %expected, ptr %desired seq_cst acquire, align 4
  %ok = extractvalue { ptr, i1 } %pair, 1
  %flag = zext i1 %ok to i32
  ret i32 %flag
}

; ASM-COUNT-2: .long	__atomic_compare_exchange_4
; ASM-COUNT-2: .long	__atomic_compare_exchange_8
; ASM-COUNT-2: .long	__atomic_compare_exchange_4
; ASM-NOT: tas.b
