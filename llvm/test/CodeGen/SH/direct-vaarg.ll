; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -target-abi=gnu-hard-float -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -target-abi=gnu-hard-float -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -target-abi=gnu-hard-float -O2 -enable-new-pm -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -target-abi=gnu-hard-float -O0 -enable-new-pm -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -enable-new-pm -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3e -target-abi=gnu-e-float -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -mattr=+fpu-single-mode -target-abi=gnu-hard-float-single -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -target-abi=gnu-hard-float -O0 -print-after=sh-prepare-ir -filetype=null %s -o /dev/null 2>&1 | FileCheck %s

; va_arg must survive preparation and be lowered according to the selected
; subtarget ABI. Splitting i64 before selecting the register/overflow area can
; incorrectly consume one register word and one stack word.
declare void @llvm.va_start.p0(ptr)
declare void @llvm.va_end.p0(ptr)
declare void @llvm.va_copy.p0.p0(ptr, ptr)

; CHECK-LABEL: define i32 @first_stack_integer(
; CHECK: va_arg ptr %ap, i32
define i32 @first_stack_integer(i32 %a, i32 %b, i32 %c, i32 %d, ...) noinline {
  %ap = alloca [5 x ptr], align 4
  call void @llvm.va_start.p0(ptr %ap)
  %value = va_arg ptr %ap, i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %value
}

; CHECK-LABEL: define i64 @pair_at_boundary(
; CHECK: va_arg ptr %ap, i64
; CHECK: va_arg ptr %ap, i32
define i64 @pair_at_boundary(i32 %a, i32 %b, i32 %c, ...) noinline {
  %ap = alloca [5 x ptr], align 4
  call void @llvm.va_start.p0(ptr %ap)
  %pair = va_arg ptr %ap, i64
  %next = va_arg ptr %ap, i32
  %wide = zext i32 %next to i64
  %sum = add i64 %pair, %wide
  call void @llvm.va_end.p0(ptr %ap)
  ret i64 %sum
}

; CHECK-LABEL: define ptr @copied_pointer(
; CHECK: va_arg ptr %copy, ptr
define ptr @copied_pointer(i32 %fixed, ...) noinline {
  %ap = alloca [5 x ptr], align 4
  %copy = alloca [5 x ptr], align 4
  call void @llvm.va_start.p0(ptr %ap)
  call void @llvm.va_copy.p0.p0(ptr %copy, ptr %ap)
  %value = va_arg ptr %copy, ptr
  call void @llvm.va_end.p0(ptr %copy)
  call void @llvm.va_end.p0(ptr %ap)
  ret ptr %value
}

; CHECK-LABEL: define double @double_after_float(
; CHECK: va_arg ptr %ap, double
define double @double_after_float(float %fixed, ...) noinline {
  %ap = alloca [5 x ptr], align 4
  call void @llvm.va_start.p0(ptr %ap)
  %value = va_arg ptr %ap, double
  call void @llvm.va_end.p0(ptr %ap)
  ret double %value
}

; CHECK-LABEL: define double @stack_double(
; CHECK: va_arg ptr %ap, double
define double @stack_double(double %a, double %b, double %c, double %d, ...) noinline {
  %ap = alloca [5 x ptr], align 4
  call void @llvm.va_start.p0(ptr %ap)
  %value = va_arg ptr %ap, double
  call void @llvm.va_end.p0(ptr %ap)
  ret double %value
}
