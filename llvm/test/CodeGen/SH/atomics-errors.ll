; RUN: split-file %s %t
; RUN: not --crash llc -disable-verify -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i1.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TYPE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i8.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TYPE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i16.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TYPE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/i128.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TYPE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/float-load.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TYPE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/float-store.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TYPE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/float-rmw.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TYPE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vector.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TYPE
; RUN: not --crash llc -disable-verify -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/aggregate.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TYPE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/address-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TYPE
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/scope.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SCOPE
; RUN: not --crash llc -disable-verify -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/tas-address-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TAS-AS
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/user-i1.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=USER-I1
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/consume-helper.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=HELPER

; TYPE: LLVM ERROR: SH generic atomic operation is not supported for this type
; SCOPE: LLVM ERROR: SH synchronization scope is not supported
; TAS-AS: LLVM ERROR: SH TAS.B intrinsic requires an address-space-zero pointer
; USER-I1: LLVM ERROR: SH comparison results may only be used by conditional branches
; HELPER: LLVM ERROR: SH calls only support void, i32, i64, and pointer return values

;--- i1.ll
define i1 @atomic_i1(ptr %p) {
  %value = load atomic i1, ptr %p monotonic, align 1
  ret i1 %value
}

;--- i8.ll
define i8 @atomic_i8(ptr %p, i8 %v) {
  %old = atomicrmw xchg ptr %p, i8 %v monotonic, align 1
  ret i8 %old
}

;--- i16.ll
define void @atomic_i16(ptr %p, i16 %v) {
  store atomic i16 %v, ptr %p release, align 2
  ret void
}

;--- i128.ll
define i128 @atomic_i128(ptr %p) {
  %value = load atomic i128, ptr %p acquire, align 16
  ret i128 %value
}

;--- float-load.ll
define float @atomic_float_load(ptr %p) {
  %value = load atomic float, ptr %p acquire, align 4
  ret float %value
}

;--- float-store.ll
define void @atomic_float_store(ptr %p, float %v) {
  store atomic float %v, ptr %p release, align 4
  ret void
}

;--- float-rmw.ll
define float @atomic_float_rmw(ptr %p, float %v) {
  %old = atomicrmw fadd ptr %p, float %v monotonic, align 4
  ret float %old
}

;--- vector.ll
define <2 x i32> @atomic_vector(ptr %p) {
  %value = load atomic <2 x i32>, ptr %p acquire, align 8
  ret <2 x i32> %value
}

;--- address-space.ll
define i32 @atomic_address_space(ptr addrspace(1) %p) {
  %value = load atomic i32, ptr addrspace(1) %p acquire, align 4
  ret i32 %value
}

;--- aggregate.ll
%S = type { i32, i32 }

define %S @atomic_aggregate(ptr %p) {
  %value = load atomic %S, ptr %p acquire, align 4
  ret %S %value
}

;--- scope.ll
define i32 @atomic_scope(ptr %p) {
  %value = load atomic i32, ptr %p syncscope("sh-custom") acquire, align 4
  ret i32 %value
}

;--- tas-address-space.ll
declare i32 @llvm.sh.tas.b(ptr addrspace(1))

define i32 @tas_address_space() {
  %value = call i32 @llvm.sh.tas.b(ptr addrspace(1) inttoptr (i32 1 to ptr addrspace(1)))
  ret i32 %value
}

;--- user-i1.ll
define i1 @user_i1(i32 %a, i32 %b) {
  %value = icmp eq i32 %a, %b
  ret i1 %value
}

;--- consume-helper.ll
declare zeroext i1 @__atomic_compare_exchange_4(ptr, ptr, i32, i32, i32)

define i32 @consume_helper(ptr %p, ptr %expected, i32 %desired) {
  %success = call zeroext i1 @__atomic_compare_exchange_4(ptr %p, ptr %expected, i32 %desired, i32 1, i32 0)
  %result = zext i1 %success to i32
  ret i32 %result
}
