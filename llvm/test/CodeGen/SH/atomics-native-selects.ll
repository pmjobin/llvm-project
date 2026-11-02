; RUN: llc -enable-new-pm=0 -mtriple=sh-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs -stop-after=sh-prepare-ir < %s | FileCheck %s --check-prefix=IR
; RUN: llc -enable-new-pm=1 -mtriple=shle-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs -stop-after=sh-prepare-ir-module < %s | FileCheck %s --check-prefix=IR
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM

; Ordinary SelectionDAG legalization owns desired-value selects, including a
; compound condition, a floating bitcast, and the generic runtime's alloca.
; These exact functions also run against GNU libatomic in --case atomics.
declare zeroext i1 @__atomic_compare_exchange_4(ptr, ptr, i32, i32, i32)
declare zeroext i1 @__atomic_compare_exchange(i32, ptr, ptr, ptr, i32, i32)

; IR-LABEL: define i32 @cas_logic32(
; IR: %both = and i1 %low, %not_two
; IR-NEXT: %desired = select i1 %both, i32 %a, i32 %b
; IR-NEXT: %success = call zeroext i1 @__atomic_compare_exchange_4
; ASM-LABEL: cas_logic32:
; ASM: .long __atomic_compare_exchange_4
define i32 @cas_logic32(ptr %p, ptr %expected, i32 %a, i32 %b, i32 %choice) {
  %low = icmp ult i32 %choice, 4
  %not_two = icmp ne i32 %choice, 2
  %both = and i1 %low, %not_two
  %desired = select i1 %both, i32 %a, i32 %b
  %success = call zeroext i1 @__atomic_compare_exchange_4(ptr %p, ptr %expected, i32 %desired, i32 5, i32 2)
  %result = zext i1 %success to i32
  ret i32 %result
}

; IR-LABEL: define i32 @cas_float32(
; IR: %desired = select i1 %use_a, float %a, float %b
; IR-NEXT: %bits = bitcast float %desired to i32
; IR-NEXT: %success = call zeroext i1 @__atomic_compare_exchange_4
; ASM-LABEL: cas_float32:
; ASM: .long __atomic_compare_exchange_4
define i32 @cas_float32(ptr %p, ptr %expected, float %a, float %b, i32 %choice) {
  %use_a = icmp eq i32 %choice, 0
  %desired = select i1 %use_a, float %a, float %b
  %bits = bitcast float %desired to i32
  %success = call zeroext i1 @__atomic_compare_exchange_4(ptr %p, ptr %expected, i32 %bits, i32 5, i32 2)
  %result = zext i1 %success to i32
  ret i32 %result
}

; IR-LABEL: define i32 @cas_double64(
; IR: %desired = select i1 %use_a, double %a, double %b
; IR-NEXT: store double %desired, ptr %storage, align 4
; IR-NEXT: %success = call zeroext i1 @__atomic_compare_exchange(i32 8
; ASM-LABEL: cas_double64:
; ASM: .long __atomic_compare_exchange
define i32 @cas_double64(ptr %p, ptr %expected, double %a, double %b, i32 %choice) {
  %storage = alloca double, align 4
  %use_a = icmp eq i32 %choice, 0
  %desired = select i1 %use_a, double %a, double %b
  store double %desired, ptr %storage, align 4
  %success = call zeroext i1 @__atomic_compare_exchange(i32 8, ptr %p, ptr %expected, ptr %storage, i32 5, i32 2)
  %result = zext i1 %success to i32
  ret i32 %result
}
