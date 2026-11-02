; RUN: llc -enable-new-pm=0 -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-before=atomic-expand < %s | FileCheck %s --check-prefix=BEFORE
; RUN: llc -enable-new-pm=1 -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-before=atomic-expand < %s | FileCheck %s --check-prefix=BEFORE
; RUN: llc -enable-new-pm=0 -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=atomic-expand < %s | FileCheck %s --check-prefix=EXPANDED
; RUN: llc -enable-new-pm=1 -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=atomic-expand < %s | FileCheck %s --check-prefix=EXPANDED
; RUN: llc -enable-new-pm=0 -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -enable-new-pm=1 -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL

; BEFORE: load atomic i32, ptr %p seq_cst
; BEFORE: atomicrmw min ptr %p, i32 %v acq_rel
; BEFORE: cmpxchg weak ptr %p, i32 %expected, i32 %desired seq_cst acquire

; EXPANDED-NOT: load atomic
; EXPANDED-NOT: atomicrmw
; EXPANDED-NOT: cmpxchg
; EXPANDED: call i32 @__atomic_load_4
; EXPANDED: call zeroext i1 @__atomic_compare_exchange_4

; ISEL-NOT: ATOMIC_
; ISEL: JSR
; ISEL: csr_sh

define i32 @pipeline(ptr %p, i32 %v, i32 %expected, i32 %desired) {
  %loaded = load atomic i32, ptr %p seq_cst, align 4
  %old = atomicrmw min ptr %p, i32 %v acq_rel, align 4
  %pair = cmpxchg weak ptr %p, i32 %expected, i32 %desired seq_cst acquire, align 4
  %cas_old = extractvalue { i32, i1 } %pair, 0
  %sum0 = add i32 %loaded, %old
  %sum1 = add i32 %sum0, %cas_old
  ret i32 %sum1
}
