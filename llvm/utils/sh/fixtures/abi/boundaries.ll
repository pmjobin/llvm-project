; Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

target triple = "shle-unknown-linux-gnu"


declare void @llvm.va_start.p0(ptr)
declare void @llvm.va_end.p0(ptr)
declare void @llvm.va_copy.p0.p0(ptr, ptr)

define i32 @first_stack_integer(i32 %a, i32 %b, i32 %c, i32 %d, ...) noinline {
  %ap = alloca [5 x ptr], align 4
  call void @llvm.va_start.p0(ptr %ap)
  %value = va_arg ptr %ap, i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %value
}

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

define double @double_after_float(float %fixed, ...) noinline {
  %ap = alloca [5 x ptr], align 4
  call void @llvm.va_start.p0(ptr %ap)
  %value = va_arg ptr %ap, double
  call void @llvm.va_end.p0(ptr %ap)
  ret double %value
}

define double @stack_double(double %a, double %b, double %c, double %d, ...) noinline {
  %ap = alloca [5 x ptr], align 4
  call void @llvm.va_start.p0(ptr %ap)
  %value = va_arg ptr %ap, double
  call void @llvm.va_end.p0(ptr %ap)
  ret double %value
}

define i32 @nested_with_realign(ptr nest %environment) nounwind {
  %local = alloca [16 x i8], align 16
  store volatile i8 0, ptr %local, align 16
  %value = load i32, ptr %environment, align 4
  ret i32 %value
}

define i32 @nested_with_large_frame(ptr nest %environment) nounwind optsize {
  %local = alloca [512 x i8], align 4
  call void asm sideeffect "", "*m"(ptr elementtype([512 x i8]) %local)
  %value = load i32, ptr %environment, align 4
  ret i32 %value
}

define void @nested_with_arguments(ptr sret({ i32 }) %out, ptr nest %environment,
                                  i32 %a, i32 %b, i32 %c, i32 %d) nounwind {
  %local = alloca [512 x i8], align 16
  call void asm sideeffect "", "*m"(ptr elementtype([512 x i8]) %local)
  %value = load i32, ptr %environment, align 4
  %ab = add i32 %a, %b
  %cd = add i32 %c, %d
  %abcd = add i32 %ab, %cd
  %sum = add i32 %value, %abcd
  store i32 %sum, ptr %out, align 4
  ret void
}
%packed = type <{ i8, float, double, ptr, [2 x double] }>

define void @packed_copy(ptr %out, ptr %in) noinline nounwind {
  %value = load %packed, ptr %in, align 1
  store %packed %value, ptr %out, align 1
  ret void
}

define void @packed_volatile_copy(ptr %out, ptr %in) noinline nounwind {
  %value = load volatile %packed, ptr %in, align 1
  store volatile %packed %value, ptr %out, align 1
  ret void
}

define i32 @nest_caller(ptr %environment) noinline nounwind {
  %a = call i32 @nested_with_realign(ptr nest %environment)
  %b = call i32 @nested_with_large_frame(ptr nest %environment)
  %out = alloca { i32 }, align 4
  call void @nested_with_arguments(ptr sret({ i32 }) %out,
       ptr nest %environment, i32 1, i32 2, i32 3, i32 4)
  %c = load i32, ptr %out, align 4
  %ab = add i32 %a, %b
  %abc = add i32 %ab, %c
  ret i32 %abc
}

declare ptr @llvm.returnaddress(i32 immarg)

define i32 @returnaddress_matches_pr() noinline nounwind {
  %reported = call ptr @llvm.returnaddress(i32 0)
  %incoming = call i32 asm sideeffect "sts pr,$0", "=r,~{r8}"()
  %bits = ptrtoint ptr %reported to i32
  %equal = icmp eq i32 %bits, %incoming
  %result = zext i1 %equal to i32
  ret i32 %result
}

define i32 @returnaddress_cfi() noinline nounwind uwtable {
  %reported = call ptr @llvm.returnaddress(i32 0)
  %incoming = call i32 asm sideeffect "sts pr,$0", "=r,~{r8},~{r9}"()
  %bits = ptrtoint ptr %reported to i32
  %equal = icmp eq i32 %bits, %incoming
  %result = zext i1 %equal to i32
  ret i32 %result
}
