; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=atomic-expand < %s | FileCheck %s --check-prefix=EXPAND
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM

; EXPAND-LABEL: define i32 @load4_align1
; EXPAND: alloca i32, align 4
; EXPAND: call void @__atomic_load(i32 4, ptr %p, ptr {{.*}}, i32 2)
define i32 @load4_align1(ptr %p) {
  %value = load atomic i32, ptr %p acquire, align 1
  ret i32 %value
}

; EXPAND-LABEL: define ptr @load_ptr_align1
; EXPAND: call void @__atomic_load(i32 4, ptr %p, ptr {{.*}}, i32 0)
define ptr @load_ptr_align1(ptr %p) {
  %value = load atomic ptr, ptr %p monotonic, align 1
  ret ptr %value
}

; EXPAND-LABEL: define i64 @load8_align4
; EXPAND: alloca i64, align 4
; EXPAND: call void @__atomic_load(i32 8, ptr %p, ptr {{.*}}, i32 5)
define i64 @load8_align4(ptr %p) {
  %value = load atomic i64, ptr %p seq_cst, align 4
  ret i64 %value
}

; EXPAND-LABEL: define void @store8_align1
; EXPAND: alloca i64, align 4
; EXPAND: store i64 %value, ptr {{.*}}, align 4
; EXPAND: call void @__atomic_store(i32 8, ptr %p, ptr {{.*}}, i32 3)
define void @store8_align1(ptr %p, i64 %value) {
  store atomic i64 %value, ptr %p release, align 1
  ret void
}

; EXPAND-LABEL: define void @store4_align1
; EXPAND: call void @__atomic_store(i32 4, ptr %p, ptr {{.*}}, i32 0)
define void @store4_align1(ptr %p, i32 %value) {
  store atomic i32 %value, ptr %p monotonic, align 1
  ret void
}

; EXPAND-LABEL: define i32 @exchange4_align2
; EXPAND: call void @__atomic_exchange(i32 4, ptr %p, ptr {{.*}}, ptr {{.*}}, i32 4)
define i32 @exchange4_align2(ptr %p, i32 %value) {
  %old = atomicrmw xchg ptr %p, i32 %value acq_rel, align 2
  ret i32 %old
}

; EXPAND-LABEL: define i64 @exchange8_align4
; EXPAND: call void @__atomic_exchange(i32 8, ptr %p, ptr {{.*}}, ptr {{.*}}, i32 5)
define i64 @exchange8_align4(ptr %p, i64 %value) {
  %old = atomicrmw xchg ptr %p, i64 %value seq_cst, align 4
  ret i64 %old
}

; EXPAND-LABEL: define i64 @add8_align4
; EXPAND: call void @__atomic_load(i32 8, ptr %p, ptr {{.*}}, i32 0)
; EXPAND: call zeroext i1 @__atomic_compare_exchange(i32 8, ptr %p, ptr {{.*}}, ptr {{.*}}, i32 5, i32 5)
define i64 @add8_align4(ptr %p, i64 %value) {
  %old = atomicrmw add ptr %p, i64 %value seq_cst, align 4
  ret i64 %old
}

; EXPAND-LABEL: define i32 @cmpxchg4_align1
; EXPAND: call zeroext i1 @__atomic_compare_exchange(i32 4, ptr %p, ptr {{.*}}, ptr {{.*}}, i32 5, i32 2)
define i32 @cmpxchg4_align1(ptr %p, i32 %expected, i32 %desired) {
  %pair = cmpxchg ptr %p, i32 %expected, i32 %desired seq_cst acquire, align 1
  %old = extractvalue { i32, i1 } %pair, 0
  ret i32 %old
}

; ASM-COUNT-3: .long	__atomic_load
; ASM-COUNT-2: .long	__atomic_store
; ASM-COUNT-2: .long	__atomic_exchange
; ASM-COUNT-1: .long	__atomic_load
; ASM-COUNT-2: .long	__atomic_compare_exchange
; ASM-NOT: __atomic_{{.*}}_4
; ASM-NOT: __atomic_{{.*}}_8
; ASM-NOT: tas.b
