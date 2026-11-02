; Expanded aggregate va_arg witness for shle-unknown-elf.
; Original C7 input and one-time conversion are documented in SHRecovery.json.

%S5 = type { i32, i8 }
%S12 = type { i32, i32, i32 }

declare void @llvm.va_start.p0(ptr)
declare void @llvm.va_copy.p0(ptr, ptr)
declare void @llvm.va_end.p0(ptr)
declare i32 @sink(i32, ...)

define i32 @sum(i32 %count, ...) {
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %first = va_arg ptr %ap, i32
  %second = va_arg ptr %ap, i32
  %result = add i32 %first, %second
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %result
}

define i32 @after_four(i32 %a, i32 %b, i32 %c, i32 %d, ...) {
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %value = va_arg ptr %ap, i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %value
}

define i32 @take_split_i64(i32 %a, i32 %b, i32 %c, ...) {
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %value = va_arg ptr %ap, i64
  %low = trunc i64 %value to i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %low
}

define %S5 @take_s5(i32 %tag, ...) {
  %vaarg.tmp = alloca %S5, align 4
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %vaarg.cursor = load ptr, ptr %ap, align 4
  %vaarg.source = getelementptr i8, ptr %vaarg.cursor, i64 0
  %vaarg.chunk = load i32, ptr %vaarg.source, align 4
  %vaarg.destination = getelementptr i8, ptr %vaarg.tmp, i64 0
  store i32 %vaarg.chunk, ptr %vaarg.destination, align 4
  %vaarg.source1 = getelementptr i8, ptr %vaarg.cursor, i64 4
  %vaarg.chunk2 = load i32, ptr %vaarg.source1, align 4
  %vaarg.destination3 = getelementptr i8, ptr %vaarg.tmp, i64 4
  store i32 %vaarg.chunk2, ptr %vaarg.destination3, align 4
  %1 = getelementptr i8, ptr %vaarg.tmp, i64 0
  %2 = load i32, ptr %1, align 4
  %3 = insertvalue %S5 poison, i32 %2, 0
  %4 = getelementptr i8, ptr %vaarg.tmp, i64 4
  %5 = load i8, ptr %4, align 4
  %6 = insertvalue %S5 %3, i8 %5, 1
  %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
  store ptr %vaarg.next, ptr %ap, align 4
  call void @llvm.va_end.p0(ptr %ap)
  ret %S5 %6
}

define i32 @copy_args(i32 %count, ...) {
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
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %value = va_arg ptr %ap, i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %value
}

define void @sret_vararg(ptr sret(%S12) align 4 %out, i32 %tag, ...) {
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
  %result = call i32 (i32, ...) @sink(i32 1, i32 2, i32 3, i32 4, i32 5, i32 6)
  ret i32 %result
}
