; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=atomic-expand < %s | FileCheck %s --check-prefix=EXPAND
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM

; EXPAND-LABEL: define i32 @add4
; EXPAND: call i32 @__atomic_fetch_add_4(ptr %p, i32 %v, i32 0)
define i32 @add4(ptr %p, i32 %v) {
  %old = atomicrmw add ptr %p, i32 %v monotonic, align 4
  ret i32 %old
}

; EXPAND-LABEL: define i32 @sub4
; EXPAND: call i32 @__atomic_fetch_sub_4(ptr %p, i32 %v, i32 2)
define i32 @sub4(ptr %p, i32 %v) {
  %old = atomicrmw sub ptr %p, i32 %v acquire, align 4
  ret i32 %old
}

; EXPAND-LABEL: define i32 @and4
; EXPAND: call i32 @__atomic_fetch_and_4(ptr %p, i32 %v, i32 3)
define i32 @and4(ptr %p, i32 %v) {
  %old = atomicrmw and ptr %p, i32 %v release, align 4
  ret i32 %old
}

; EXPAND-LABEL: define i32 @nand4
; EXPAND: call i32 @__atomic_fetch_nand_4(ptr %p, i32 %v, i32 4)
define i32 @nand4(ptr %p, i32 %v) {
  %old = atomicrmw nand ptr %p, i32 %v acq_rel, align 4
  ret i32 %old
}

; EXPAND-LABEL: define i32 @or4
; EXPAND: call i32 @__atomic_fetch_or_4(ptr %p, i32 %v, i32 5)
define i32 @or4(ptr %p, i32 %v) {
  %old = atomicrmw or ptr %p, i32 %v seq_cst, align 4
  ret i32 %old
}

; EXPAND-LABEL: define i32 @xor4
; EXPAND: call i32 @__atomic_fetch_xor_4(ptr %p, i32 %v, i32 0)
define i32 @xor4(ptr %p, i32 %v) {
  %old = atomicrmw volatile xor ptr %p, i32 %v monotonic, align 4
  ret i32 %old
}

; EXPAND-LABEL: define i64 @add8
; EXPAND: call i64 @__atomic_fetch_add_8(ptr %p, i64 %v, i32 0)
define i64 @add8(ptr %p, i64 %v) {
  %old = atomicrmw add ptr %p, i64 %v monotonic, align 8
  ret i64 %old
}

; EXPAND-LABEL: define i64 @sub8
; EXPAND: call i64 @__atomic_fetch_sub_8(ptr %p, i64 %v, i32 2)
define i64 @sub8(ptr %p, i64 %v) {
  %old = atomicrmw sub ptr %p, i64 %v acquire, align 8
  ret i64 %old
}

; EXPAND-LABEL: define i64 @and8
; EXPAND: call i64 @__atomic_fetch_and_8(ptr %p, i64 %v, i32 3)
define i64 @and8(ptr %p, i64 %v) {
  %old = atomicrmw and ptr %p, i64 %v release, align 8
  ret i64 %old
}

; EXPAND-LABEL: define i64 @nand8
; EXPAND: call i64 @__atomic_fetch_nand_8(ptr %p, i64 %v, i32 4)
define i64 @nand8(ptr %p, i64 %v) {
  %old = atomicrmw nand ptr %p, i64 %v acq_rel, align 8
  ret i64 %old
}

; EXPAND-LABEL: define i64 @or8
; EXPAND: call i64 @__atomic_fetch_or_8(ptr %p, i64 %v, i32 5)
define i64 @or8(ptr %p, i64 %v) {
  %old = atomicrmw or ptr %p, i64 %v seq_cst, align 8
  ret i64 %old
}

; EXPAND-LABEL: define i64 @xor8
; EXPAND: call i64 @__atomic_fetch_xor_8(ptr %p, i64 %v, i32 0)
define i64 @xor8(ptr %p, i64 %v) {
  %old = atomicrmw volatile xor ptr %p, i64 %v monotonic, align 8
  ret i64 %old
}

; EXPAND-LABEL: define i32 @max4
; EXPAND: call i32 @__atomic_load_4
; EXPAND: call zeroext i1 @__atomic_compare_exchange_4
define i32 @max4(ptr %p, i32 %v) {
  %old = atomicrmw max ptr %p, i32 %v seq_cst, align 4
  ret i32 %old
}

; EXPAND-LABEL: define i32 @min4
; EXPAND: call i32 @__atomic_load_4
; EXPAND: call zeroext i1 @__atomic_compare_exchange_4
define i32 @min4(ptr %p, i32 %v) {
  %old = atomicrmw min ptr %p, i32 %v seq_cst, align 4
  ret i32 %old
}

; EXPAND-LABEL: define i32 @umax4
; EXPAND: call i32 @__atomic_load_4
; EXPAND: call zeroext i1 @__atomic_compare_exchange_4
define i32 @umax4(ptr %p, i32 %v) {
  %old = atomicrmw umax ptr %p, i32 %v seq_cst, align 4
  ret i32 %old
}

; EXPAND-LABEL: define i32 @umin4
; EXPAND: call i32 @__atomic_load_4
; EXPAND: call zeroext i1 @__atomic_compare_exchange_4
define i32 @umin4(ptr %p, i32 %v) {
  %old = atomicrmw umin ptr %p, i32 %v seq_cst, align 4
  ret i32 %old
}

; EXPAND-LABEL: define i32 @uinc4
; EXPAND: call i32 @__atomic_load_4
; EXPAND: call zeroext i1 @__atomic_compare_exchange_4
define i32 @uinc4(ptr %p, i32 %v) {
  %old = atomicrmw uinc_wrap ptr %p, i32 %v seq_cst, align 4
  ret i32 %old
}

; EXPAND-LABEL: define i32 @udec4
; EXPAND: call i32 @__atomic_load_4
; EXPAND: call zeroext i1 @__atomic_compare_exchange_4
define i32 @udec4(ptr %p, i32 %v) {
  %old = atomicrmw udec_wrap ptr %p, i32 %v seq_cst, align 4
  ret i32 %old
}

; EXPAND-LABEL: define i32 @usub_cond4
; EXPAND: call i32 @__atomic_load_4
; EXPAND: call zeroext i1 @__atomic_compare_exchange_4
define i32 @usub_cond4(ptr %p, i32 %v) {
  %old = atomicrmw usub_cond ptr %p, i32 %v seq_cst, align 4
  ret i32 %old
}

; EXPAND-LABEL: define i32 @usub_sat4
; EXPAND: call i32 @__atomic_load_4
; EXPAND: call zeroext i1 @__atomic_compare_exchange_4
define i32 @usub_sat4(ptr %p, i32 %v) {
  %old = atomicrmw usub_sat ptr %p, i32 %v seq_cst, align 4
  ret i32 %old
}

; EXPAND-LABEL: define i64 @max8
; EXPAND: call i64 @__atomic_load_8
; EXPAND: call zeroext i1 @__atomic_compare_exchange_8
define i64 @max8(ptr %p, i64 %v) {
  %old = atomicrmw max ptr %p, i64 %v seq_cst, align 8
  ret i64 %old
}

; EXPAND-LABEL: define i64 @min8
; EXPAND: call i64 @__atomic_load_8
; EXPAND: call zeroext i1 @__atomic_compare_exchange_8
define i64 @min8(ptr %p, i64 %v) {
  %old = atomicrmw min ptr %p, i64 %v seq_cst, align 8
  ret i64 %old
}

; EXPAND-LABEL: define i64 @umax8
; EXPAND: call i64 @__atomic_load_8
; EXPAND: call zeroext i1 @__atomic_compare_exchange_8
define i64 @umax8(ptr %p, i64 %v) {
  %old = atomicrmw umax ptr %p, i64 %v seq_cst, align 8
  ret i64 %old
}

; EXPAND-LABEL: define i64 @umin8
; EXPAND: call i64 @__atomic_load_8
; EXPAND: call zeroext i1 @__atomic_compare_exchange_8
define i64 @umin8(ptr %p, i64 %v) {
  %old = atomicrmw umin ptr %p, i64 %v seq_cst, align 8
  ret i64 %old
}

; EXPAND-LABEL: define i64 @uinc8
; EXPAND: call i64 @__atomic_load_8
; EXPAND: call zeroext i1 @__atomic_compare_exchange_8
define i64 @uinc8(ptr %p, i64 %v) {
  %old = atomicrmw uinc_wrap ptr %p, i64 %v seq_cst, align 8
  ret i64 %old
}

; EXPAND-LABEL: define i64 @udec8
; EXPAND: call i64 @__atomic_load_8
; EXPAND: call zeroext i1 @__atomic_compare_exchange_8
define i64 @udec8(ptr %p, i64 %v) {
  %old = atomicrmw udec_wrap ptr %p, i64 %v seq_cst, align 8
  ret i64 %old
}

; EXPAND-LABEL: define i64 @usub_cond8
; EXPAND: call i64 @__atomic_load_8
; EXPAND: call zeroext i1 @__atomic_compare_exchange_8
define i64 @usub_cond8(ptr %p, i64 %v) {
  %old = atomicrmw usub_cond ptr %p, i64 %v seq_cst, align 8
  ret i64 %old
}

; EXPAND-LABEL: define i64 @usub_sat8
; EXPAND: call i64 @__atomic_load_8
; EXPAND: call zeroext i1 @__atomic_compare_exchange_8
define i64 @usub_sat8(ptr %p, i64 %v) {
  %old = atomicrmw usub_sat ptr %p, i64 %v seq_cst, align 8
  ret i64 %old
}

; ASM: .long	__atomic_fetch_add_4
; ASM: .long	__atomic_fetch_sub_4
; ASM: .long	__atomic_fetch_and_4
; ASM: .long	__atomic_fetch_nand_4
; ASM: .long	__atomic_fetch_or_4
; ASM: .long	__atomic_fetch_xor_4
; ASM: .long	__atomic_fetch_add_8
; ASM: .long	__atomic_fetch_sub_8
; ASM: .long	__atomic_fetch_and_8
; ASM: .long	__atomic_fetch_nand_8
; ASM: .long	__atomic_fetch_or_8
; ASM: .long	__atomic_fetch_xor_8
; ASM: .long	__atomic_load_4
; ASM: .long	__atomic_compare_exchange_4
; ASM: .long	__atomic_load_4
; ASM: .long	__atomic_compare_exchange_4
; ASM: .long	__atomic_load_4
; ASM: .long	__atomic_compare_exchange_4
; ASM: .long	__atomic_load_4
; ASM: .long	__atomic_compare_exchange_4
; ASM: .long	__atomic_load_4
; ASM: .long	__atomic_compare_exchange_4
; ASM: .long	__atomic_load_4
; ASM: .long	__atomic_compare_exchange_4
; ASM: .long	__atomic_load_4
; ASM: .long	__atomic_compare_exchange_4
; ASM: .long	__atomic_load_4
; ASM: .long	__atomic_compare_exchange_4
; ASM: .long	__atomic_load_8
; ASM: .long	__atomic_compare_exchange_8
; ASM: .long	__atomic_load_8
; ASM: .long	__atomic_compare_exchange_8
; ASM: .long	__atomic_load_8
; ASM: .long	__atomic_compare_exchange_8
; ASM: .long	__atomic_load_8
; ASM: .long	__atomic_compare_exchange_8
; ASM: .long	__atomic_load_8
; ASM: .long	__atomic_compare_exchange_8
; ASM: .long	__atomic_load_8
; ASM: .long	__atomic_compare_exchange_8
; ASM: .long	__atomic_load_8
; ASM: .long	__atomic_compare_exchange_8
; ASM: .long	__atomic_load_8
; ASM: .long	__atomic_compare_exchange_8
; ASM-NOT: tas.b
