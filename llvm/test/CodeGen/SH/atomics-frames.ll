; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=prolog-epilog < %s | FileCheck %s --check-prefix=FRAME

%ByVal = type { i32, i32 }
%AtomicResult = type { i32, i64 }

declare void @llvm.va_start(ptr)
declare void @llvm.va_end(ptr)
declare i32 @six_args(i32, i32, i32, i32, i32, i32)

; ASM-LABEL: variadic_atomic:
; ASM: sts.l	pr,@-r15
; ASM: lds.l	@r15+,pr
; ASM: .long	__atomic_load
define i64 @variadic_atomic(ptr %p, i32 %fixed, ...) {
  %ap = alloca ptr, align 4
  call void @llvm.va_start(ptr %ap)
  %value = load atomic i64, ptr %p acquire, align 4
  call void @llvm.va_end(ptr %ap)
  ret i64 %value
}

; ASM-LABEL: byval_atomic:
; ASM: .long	__atomic_compare_exchange_4
define i32 @byval_atomic(ptr byval(%ByVal) align 4 %arg, ptr %p, i32 %desired) {
  %expected = load i32, ptr %arg, align 4
  %pair = cmpxchg ptr %p, i32 %expected, i32 %desired seq_cst acquire, align 4
  %old = extractvalue { i32, i1 } %pair, 0
  ret i32 %old
}

; ASM-LABEL: outgoing_frame_atomic:
; ASM: .long	__atomic_load_4
; ASM: .long	six_args
define i32 @outgoing_frame_atomic(ptr %p, i32 %v) {
  %loaded = load atomic i32, ptr %p seq_cst, align 4
  %called = call i32 @six_args(i32 %v, i32 %v, i32 %v, i32 %v, i32 %v, i32 %v)
  %sum = add i32 %loaded, %called
  ret i32 %sum
}

; ASM-LABEL: aggregate_return_atomic:
; ASM: .long	__atomic_load_8
define void @aggregate_return_atomic(ptr noalias sret(%AtomicResult) align 4 %result, ptr %p) {
  %value = load atomic i64, ptr %p acquire, align 8
  %first = getelementptr inbounds %AtomicResult, ptr %result, i32 0, i32 0
  %second = getelementptr inbounds %AtomicResult, ptr %result, i32 0, i32 1
  store i32 1, ptr %first, align 4
  store i64 %value, ptr %second, align 4
  ret void
}

; ASM-LABEL: pressure_atomic:
; ASM: .long	__atomic_fetch_add_4
define i32 @pressure_atomic(ptr %p, i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i32 %f, i32 %g, i32 %h) {
  %old = atomicrmw add ptr %p, i32 %a acq_rel, align 4
  %s0 = add i32 %old, %b
  %s1 = add i32 %s0, %c
  %s2 = add i32 %s1, %d
  %s3 = add i32 %s2, %e
  %s4 = add i32 %s3, %f
  %s5 = add i32 %s4, %g
  %s6 = add i32 %s5, %h
  ret i32 %s6
}

; FRAME-LABEL: name:            extended_atomic_frame
; FRAME: stackSize:       72
; FRAME-LABEL: body:
; FRAME-NOT: %stack.
; FRAME-NOT: %fixed-stack.
; FRAME-NOT: frame-index
; FRAME: $r15 = frame-setup STS_L_PR
; FRAME: $r15 = frame-setup ADDri $r15, -68
; FRAME: $r15 = frame-destroy ADDri $r15, 68
; FRAME: $r15 = frame-destroy LDS_L_PR
; ASM-LABEL: extended_atomic_frame:
; ASM: sts.l	pr,@-r15
; ASM: add	#-68,r15
; ASM: add	#68,r15
; ASM: lds.l	@r15+,pr
; ASM: .long	__atomic_load_8
; ASM: .long	__atomic_compare_exchange_8
define i64 @extended_atomic_frame(ptr %p, i64 %limit) {
  %old = atomicrmw uinc_wrap ptr %p, i64 %limit seq_cst, align 8
  ret i64 %old
}
