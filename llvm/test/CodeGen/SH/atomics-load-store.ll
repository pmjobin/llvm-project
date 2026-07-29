; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=atomic-expand < %s | FileCheck %s --check-prefix=EXPAND
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM

; EXPAND-LABEL: define i32 @load_i32_unordered
; EXPAND: call i32 @__atomic_load_4(ptr %p, i32 0)
define i32 @load_i32_unordered(ptr %p) {
  %value = load atomic volatile i32, ptr %p unordered, align 4
  ret i32 %value
}

; EXPAND-LABEL: define ptr @load_ptr_seq_cst
; EXPAND: call i32 @__atomic_load_4(ptr %p, i32 5)
; EXPAND: inttoptr i32
define ptr @load_ptr_seq_cst(ptr %p) {
  %value = load atomic ptr, ptr %p seq_cst, align 4
  ret ptr %value
}

; EXPAND-LABEL: define i64 @load_i64_acquire
; EXPAND: call i64 @__atomic_load_8(ptr %p, i32 2)
define i64 @load_i64_acquire(ptr %p) {
  %value = load atomic i64, ptr %p acquire, align 8
  ret i64 %value
}

; EXPAND-LABEL: define void @store_i32_release
; EXPAND: call void @__atomic_store_4(ptr %p, i32 %value, i32 3)
define void @store_i32_release(ptr %p, i32 %value) {
  store atomic volatile i32 %value, ptr %p release, align 4
  ret void
}

; EXPAND-LABEL: define void @store_ptr_monotonic
; EXPAND: ptrtoint ptr %value to i32
; EXPAND: call void @__atomic_store_4(ptr %p, i32 {{.*}}, i32 0)
define void @store_ptr_monotonic(ptr %p, ptr %value) {
  store atomic ptr %value, ptr %p monotonic, align 4
  ret void
}

; EXPAND-LABEL: define void @store_i64_seq_cst
; EXPAND: call void @__atomic_store_8(ptr %p, i64 %value, i32 5)
define void @store_i64_seq_cst(ptr %p, i64 %value) {
  store atomic i64 %value, ptr %p seq_cst, align 8
  ret void
}

; ASM-COUNT-2: .long	__atomic_load_4
; ASM-COUNT-1: .long	__atomic_load_8
; ASM-COUNT-2: .long	__atomic_store_4
; ASM-COUNT-1: .long	__atomic_store_8
; ASM-NOT: __sync_synchronize
; ASM-NOT: tas.b
