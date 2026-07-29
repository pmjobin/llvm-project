; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=SH
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=SHLE

%S5 = type { i32, i8 }
%S12 = type { i32, i32, i32 }

declare void @llvm.va_start.p0(ptr)
declare void @llvm.va_copy.p0(ptr, ptr)
declare void @llvm.va_end.p0(ptr)
declare i32 @sink(i32, ...)

define i32 @sum(i32 %count, ...) {
; SH-LABEL: sum:
; SH-DAG: mov r5,r0
; SH-DAG: mov.l r0,@(4,r15)
; SH-DAG: mov.l r6,@(8,r15)
; SH-DAG: mov.l r7,@(12,r15)
; SHLE-LABEL: sum:
; SHLE-DAG: mov r5,r0
; SHLE-DAG: mov.l r0,@(4,r15)
; SHLE-DAG: mov.l r6,@(8,r15)
; SHLE-DAG: mov.l r7,@(12,r15)
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %first = va_arg ptr %ap, i32
  %second = va_arg ptr %ap, i32
  %result = add i32 %first, %second
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %result
}

define i32 @after_four(i32 %a, i32 %b, i32 %c, i32 %d, ...) {
; SH-LABEL: after_four:
; SH: mov.l @(4,r15),r0
; SHLE-LABEL: after_four:
; SHLE: mov.l @(4,r15),r0
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %value = va_arg ptr %ap, i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %value
}

define i32 @take_split_i64(i32 %a, i32 %b, i32 %c, ...) {
; SH-LABEL: take_split_i64:
; SH: mov.l r7,@(4,r15)
; SH: mov.l @(8,r15),r0
; SHLE-LABEL: take_split_i64:
; SHLE: mov r7,r0
; SHLE: mov.l r0,@(4,r15)
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %value = va_arg ptr %ap, i64
  %low = trunc i64 %value to i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %low
}

define %S5 @take_s5(i32 %tag, ...) {
; SH-LABEL: take_s5:
; SH-DAG: mov.l r7,@(20,r15)
; SH-DAG: mov.l r1,@(16,r15)
; SH-DAG: mov.l r1,@(8,r15)
; SH-DAG: mov.l r0,@(12,r15)
; SHLE-LABEL: take_s5:
; SHLE-DAG: mov.l r7,@(20,r15)
; SHLE-DAG: mov.l r6,@(16,r15)
; SHLE-DAG: mov.l r6,@(8,r15)
; SHLE-DAG: mov.l r0,@(12,r15)
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %value = va_arg ptr %ap, %S5
  call void @llvm.va_end.p0(ptr %ap)
  ret %S5 %value
}

define i32 @copy_args(i32 %count, ...) {
; SH-LABEL: copy_args:
; SH-NOT: __va_
; SHLE-LABEL: copy_args:
; SHLE-NOT: __va_
  %ap = alloca ptr, align 4
  %aq = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %first = va_arg ptr %ap, i32
  call void @llvm.va_copy.p0(ptr %aq, ptr %ap)
  %one = va_arg ptr %ap, i32
  %two = va_arg ptr %aq, i32
  %result = add i32 %first, %one
  %out = add i32 %result, %two
  call void @llvm.va_end.p0(ptr %aq)
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %out
}

define i32 @byval_then_vararg(ptr byval(%S5) align 4 %named, ...) {
; SH-LABEL: byval_then_vararg:
; SH-DAG: mov r6,r0
; SH-DAG: mov.l r7,
; SHLE-LABEL: byval_then_vararg:
; SHLE-DAG: mov r6,r0
; SHLE-DAG: mov.l r7,
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %value = va_arg ptr %ap, i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %value
}

define void @sret_vararg(ptr sret(%S12) align 4 %out, i32 %tag, ...) {
; SH-LABEL: sret_vararg:
; SH-DAG: mov.l r5,
; SH-DAG: mov.l r6,
; SH-DAG: mov.l r7,
; SH-DAG: mov r2,r0
; SHLE-LABEL: sret_vararg:
; SHLE-DAG: mov.l r5,
; SHLE-DAG: mov.l r6,
; SHLE-DAG: mov.l r7,
; SHLE-DAG: mov r2,r0
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %value = va_arg ptr %ap, i32
  %first = insertvalue %S12 poison, i32 %value, 0
  %second = insertvalue %S12 %first, i32 %tag, 1
  %result = insertvalue %S12 %second, i32 0, 2
  store %S12 %result, ptr %out, align 4
  call void @llvm.va_end.p0(ptr %ap)
  ret void
}

define i32 @caller() {
; SH-LABEL: caller:
; SH: add #-8,r15
; SH: mov #1,r4
; SH: mov #2,r5
; SH: mov #3,r6
; SH: mov #4,r7
; SH: jsr @r0
; SH: nop
; SH: add #8,r15
; SHLE-LABEL: caller:
; SHLE: add #-8,r15
; SHLE: mov #1,r4
; SHLE: mov #2,r5
; SHLE: mov #3,r6
; SHLE: mov #4,r7
; SHLE: jsr @r0
; SHLE: nop
; SHLE: add #8,r15
  %result = call i32 (i32, ...) @sink(i32 1, i32 2, i32 3, i32 4, i32 5, i32 6)
  ret i32 %result
}
