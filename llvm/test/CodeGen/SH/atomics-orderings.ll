; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=atomic-expand < %s | FileCheck %s --check-prefix=EXPAND

; EXPAND-LABEL: define void @load_orders
; EXPAND: call i32 @__atomic_load_4(ptr %p, i32 0)
; EXPAND: call i32 @__atomic_load_4(ptr %p, i32 0)
; EXPAND: call i32 @__atomic_load_4(ptr %p, i32 2)
; EXPAND: call i32 @__atomic_load_4(ptr %p, i32 5)
define void @load_orders(ptr %p, ptr %out) {
  %u = load atomic i32, ptr %p unordered, align 4
  store i32 %u, ptr %out, align 4
  %m = load atomic i32, ptr %p monotonic, align 4
  store i32 %m, ptr %out, align 4
  %a = load atomic i32, ptr %p acquire, align 4
  store i32 %a, ptr %out, align 4
  %s = load atomic i32, ptr %p seq_cst, align 4
  store i32 %s, ptr %out, align 4
  ret void
}

; EXPAND-LABEL: define void @store_orders
; EXPAND: call void @__atomic_store_4(ptr %p, i32 %v, i32 0)
; EXPAND: call void @__atomic_store_4(ptr %p, i32 %v, i32 0)
; EXPAND: call void @__atomic_store_4(ptr %p, i32 %v, i32 3)
; EXPAND: call void @__atomic_store_4(ptr %p, i32 %v, i32 5)
define void @store_orders(ptr %p, i32 %v) {
  store atomic i32 %v, ptr %p unordered, align 4
  store atomic i32 %v, ptr %p monotonic, align 4
  store atomic i32 %v, ptr %p release, align 4
  store atomic i32 %v, ptr %p seq_cst, align 4
  ret void
}

; EXPAND-LABEL: define void @rmw_orders
; EXPAND: call i32 @__atomic_fetch_add_4(ptr %p, i32 %v, i32 0)
; EXPAND: call i32 @__atomic_fetch_add_4(ptr %p, i32 %v, i32 2)
; EXPAND: call i32 @__atomic_fetch_add_4(ptr %p, i32 %v, i32 3)
; EXPAND: call i32 @__atomic_fetch_add_4(ptr %p, i32 %v, i32 4)
; EXPAND: call i32 @__atomic_fetch_add_4(ptr %p, i32 %v, i32 5)
define void @rmw_orders(ptr %p, i32 %v, ptr %out) {
  %m = atomicrmw add ptr %p, i32 %v monotonic, align 4
  store i32 %m, ptr %out, align 4
  %a = atomicrmw add ptr %p, i32 %v acquire, align 4
  store i32 %a, ptr %out, align 4
  %r = atomicrmw add ptr %p, i32 %v release, align 4
  store i32 %r, ptr %out, align 4
  %ar = atomicrmw add ptr %p, i32 %v acq_rel, align 4
  store i32 %ar, ptr %out, align 4
  %s = atomicrmw add ptr %p, i32 %v seq_cst, align 4
  store i32 %s, ptr %out, align 4
  ret void
}
