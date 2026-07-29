; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/narrow.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=PROMOTED
; RUN: not --crash llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/float.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=PROMOTED
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/vaarg-narrow.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VAARG
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/sret-only.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=NO-FIXED
; RUN: not --crash llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %t/sret-only.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=NO-FIXED

; PROMOTED: LLVM ERROR: SH variadic arguments must use supported default-promoted ABI types
; VAARG: LLVM ERROR: SH va_arg requires a supported default-promoted ABI type
; NO-FIXED: LLVM ERROR: SH va_start requires at least one fixed parameter

;--- narrow.ll
declare void @sink(i32, ...)
define void @narrow() {
  call void (i32, ...) @sink(i32 0, i8 1)
  ret void
}

;--- float.ll
declare void @sink(i32, ...)
define void @float_arg() {
  call void (i32, ...) @sink(i32 0, float 1.000000e+00)
  ret void
}

;--- vaarg-narrow.ll
declare void @llvm.va_start.p0(ptr)
define void @bad_vaarg(i32 %count, ...) {
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %value = va_arg ptr %ap, i16
  ret void
}

;--- sret-only.ll
%S12 = type { i32, i32, i32 }

declare void @llvm.va_start.p0(ptr)

define void @bad(ptr sret(%S12) %out, ...) {
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  ret void
}
