; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=atomic-expand < %s | FileCheck %s --check-prefix=EXPAND
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM

; EXPAND-LABEL: define i32 @exchange_i32
; EXPAND: call i32 @__atomic_exchange_4(ptr %p, i32 %value, i32 0)
define i32 @exchange_i32(ptr %p, i32 %value) {
  %old = atomicrmw volatile xchg ptr %p, i32 %value monotonic, align 4
  ret i32 %old
}

; EXPAND-LABEL: define ptr @exchange_ptr
; EXPAND: ptrtoint ptr %value to i32
; EXPAND: call i32 @__atomic_exchange_4(ptr %p, i32 {{.*}}, i32 4)
; EXPAND: inttoptr i32
define ptr @exchange_ptr(ptr %p, ptr %value) {
  %old = atomicrmw xchg ptr %p, ptr %value acq_rel, align 4
  ret ptr %old
}

; EXPAND-LABEL: define i64 @exchange_i64
; EXPAND: call i64 @__atomic_exchange_8(ptr %p, i64 %value, i32 5)
define i64 @exchange_i64(ptr %p, i64 %value) {
  %old = atomicrmw xchg ptr %p, i64 %value seq_cst, align 8
  ret i64 %old
}

; ASM-COUNT-2: .long	__atomic_exchange_4
; ASM-COUNT-1: .long	__atomic_exchange_8
; ASM-NOT: __sync_synchronize
; ASM-NOT: tas.b
