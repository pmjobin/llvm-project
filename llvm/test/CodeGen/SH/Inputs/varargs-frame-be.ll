; Expanded aggregate va_arg witness for sh-unknown-elf.
; Original C7 input and one-time conversion are documented in SHRecovery.json.

%R8 = type { i32, i32 }
%B8 = type [8 x i8]
%P5 = type <{ i8, i8, i8, i8, i8 }>

declare void @llvm.va_start.p0(ptr)
declare void @llvm.va_end.p0(ptr)
declare i32 @nested5(i32, i32, i32, i32, i32)
declare i32 @nested8(i32, i32, i32, i32, i32, i32, i32, i32)

define i32 @nonleaf_save12(i32 %fixed, ...) nounwind {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 %fixed, i32 3, i32 4, i32 5)
	%second = va_arg ptr %ap, i32
	%sum = add i32 %called, %second
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %sum
}

define i32 @nonleaf_save8(i32 %a, i32 %b, ...) nounwind {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 %a, i32 %b, i32 4, i32 5)
	%second = va_arg ptr %ap, i32
	%sum = add i32 %called, %second
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %sum
}

define i32 @nonleaf_save4(i32 %a, i32 %b, i32 %c, ...) nounwind {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 %a, i32 %b, i32 %c, i32 5)
	%second = va_arg ptr %ap, i32
	%sum = add i32 %called, %second
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %sum
}

define i32 @nonleaf_save0(i32 %a, i32 %b, i32 %c, i32 %d, ...) nounwind {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 %a, i32 %b, i32 %c, i32 %d)
	%second = va_arg ptr %ap, i32
	%sum = add i32 %called, %second
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %sum
}

define i32 @leaf_save12(i32 %fixed, ...) nounwind {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%value = va_arg ptr %ap, i32
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %value
}

define i32 @leaf_save0(i32 %a, i32 %b, i32 %c, i32 %d, ...) nounwind {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%value = va_arg ptr %ap, i32
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %value
}

define i32 @nonleaf_pressure(ptr %base, ...) nounwind {
	%slot = alloca i32, align 4
	%ap = alloca ptr, align 4
	store volatile i32 99, ptr %slot, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%p0 = getelementptr i8, ptr %base, i32 0
	%p1 = getelementptr i8, ptr %base, i32 4
	%p2 = getelementptr i8, ptr %base, i32 8
	%p3 = getelementptr i8, ptr %base, i32 12
	%p4 = getelementptr i8, ptr %base, i32 16
	%p5 = getelementptr i8, ptr %base, i32 20
	%p6 = getelementptr i8, ptr %base, i32 24
	%p7 = getelementptr i8, ptr %base, i32 28
	%v0 = load volatile i32, ptr %p0, align 4
	%v1 = load volatile i32, ptr %p1, align 4
	%v2 = load volatile i32, ptr %p2, align 4
	%v3 = load volatile i32, ptr %p3, align 4
	%v4 = load volatile i32, ptr %p4, align 4
	%v5 = load volatile i32, ptr %p5, align 4
	%v6 = load volatile i32, ptr %p6, align 4
	%v7 = load volatile i32, ptr %p7, align 4
	%called = call i32 @nested8(i32 %first, i32 %v0, i32 %v1, i32 %v2, i32 %v3, i32 %v4, i32 %v5, i32 %v6)
	%second = va_arg ptr %ap, i32
	%local = load volatile i32, ptr %slot, align 4
	%s0 = add i32 %called, %second
	%s1 = add i32 %s0, %local
	%s2 = add i32 %s1, %v0
	%s3 = add i32 %s2, %v1
	%s4 = add i32 %s3, %v2
	%s5 = add i32 %s4, %v3
	%s6 = add i32 %s5, %v4
	%s7 = add i32 %s6, %v5
	%s8 = add i32 %s7, %v6
	%result = add i32 %s8, %v7
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

define i32 @nonleaf_frame_limit(i32 %fixed, ...) nounwind {
	%locals = alloca [10 x i32], align 4
	%last = getelementptr [10 x i32], ptr %locals, i32 0, i32 9
	%ap = alloca ptr, align 4
	store volatile i32 %fixed, ptr %locals, align 4
	store volatile i32 %fixed, ptr %last, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 2, i32 3, i32 4, i32 5)
	%second = va_arg ptr %ap, i32
	%local = load volatile i32, ptr %last, align 4
	%sum = add i32 %called, %second
	%result = add i32 %sum, %local
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

define i64 @nonleaf_i64_return(i32 %fixed, ...) nounwind {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 2, i32 3, i32 4, i32 5)
	%second = va_arg ptr %ap, i32
	%low = zext i32 %called to i64
	%high = zext i32 %second to i64
	%shifted = shl i64 %high, 32
	%result = or i64 %shifted, %low
	call void @llvm.va_end.p0(ptr %ap)
	ret i64 %result
}

define %R8 @nonleaf_aggregate_return(i32 %fixed, ...) nounwind {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 2, i32 3, i32 4, i32 5)
	%second = va_arg ptr %ap, i32
	%part = insertvalue %R8 poison, i32 %called, 0
	%result = insertvalue %R8 %part, i32 %second, 1
	call void @llvm.va_end.p0(ptr %ap)
	ret %R8 %result
}

define i32 @nonleaf_named_byval(ptr byval(%B8) align 4 %named, ...) nounwind {
	%last = getelementptr %B8, ptr %named, i32 0, i32 7
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 2, i32 3, i32 4, i32 5)
	%second = va_arg ptr %ap, i32
	%byte = load i8, ptr %last, align 1
	%extended = zext i8 %byte to i32
	%sum = add i32 %called, %second
	%result = add i32 %sum, %extended
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

define i32 @nonleaf_packed_temporary(i32 %fixed, ...) nounwind {
  %vaarg.tmp4 = alloca %P5, align 4
  %vaarg.tmp = alloca %P5, align 4
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %vaarg.cursor = load ptr, ptr %ap, align 4
  %vaarg.source = getelementptr i8, ptr %vaarg.cursor, i64 0
  %vaarg.chunk = load i32, ptr %vaarg.source, align 4
  %vaarg.destination = getelementptr i8, ptr %vaarg.tmp, i64 0
  store i32 %vaarg.chunk, ptr %vaarg.destination, align 4
  %vaarg.source1 = getelementptr i8, ptr %vaarg.cursor, i64 7
  %vaarg.byte = load i8, ptr %vaarg.source1, align 1
  %vaarg.destination2 = getelementptr i8, ptr %vaarg.tmp, i64 4
  store i8 %vaarg.byte, ptr %vaarg.destination2, align 1
  %1 = getelementptr i8, ptr %vaarg.tmp, i64 0
  %2 = load i8, ptr %1, align 4
  %3 = insertvalue %P5 poison, i8 %2, 0
  %4 = getelementptr i8, ptr %vaarg.tmp, i64 1
  %5 = load i8, ptr %4, align 1
  %6 = insertvalue %P5 %3, i8 %5, 1
  %7 = getelementptr i8, ptr %vaarg.tmp, i64 2
  %8 = load i8, ptr %7, align 2
  %9 = insertvalue %P5 %6, i8 %8, 2
  %10 = getelementptr i8, ptr %vaarg.tmp, i64 3
  %11 = load i8, ptr %10, align 1
  %12 = insertvalue %P5 %9, i8 %11, 3
  %13 = getelementptr i8, ptr %vaarg.tmp, i64 4
  %14 = load i8, ptr %13, align 4
  %15 = insertvalue %P5 %12, i8 %14, 4
  %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
  store ptr %vaarg.next, ptr %ap, align 4
  %first.byte = extractvalue %P5 %15, 4
  %first.extended = zext i8 %first.byte to i32
  %called = call i32 @nested5(i32 %first.extended, i32 2, i32 3, i32 4, i32 5)
  %vaarg.cursor3 = load ptr, ptr %ap, align 4
  %vaarg.source5 = getelementptr i8, ptr %vaarg.cursor3, i64 0
  %vaarg.chunk6 = load i32, ptr %vaarg.source5, align 4
  %vaarg.destination7 = getelementptr i8, ptr %vaarg.tmp4, i64 0
  store i32 %vaarg.chunk6, ptr %vaarg.destination7, align 4
  %vaarg.source8 = getelementptr i8, ptr %vaarg.cursor3, i64 7
  %vaarg.byte9 = load i8, ptr %vaarg.source8, align 1
  %vaarg.destination10 = getelementptr i8, ptr %vaarg.tmp4, i64 4
  store i8 %vaarg.byte9, ptr %vaarg.destination10, align 1
  %16 = getelementptr i8, ptr %vaarg.tmp4, i64 0
  %17 = load i8, ptr %16, align 4
  %18 = insertvalue %P5 poison, i8 %17, 0
  %19 = getelementptr i8, ptr %vaarg.tmp4, i64 1
  %20 = load i8, ptr %19, align 1
  %21 = insertvalue %P5 %18, i8 %20, 1
  %22 = getelementptr i8, ptr %vaarg.tmp4, i64 2
  %23 = load i8, ptr %22, align 2
  %24 = insertvalue %P5 %21, i8 %23, 2
  %25 = getelementptr i8, ptr %vaarg.tmp4, i64 3
  %26 = load i8, ptr %25, align 1
  %27 = insertvalue %P5 %24, i8 %26, 3
  %28 = getelementptr i8, ptr %vaarg.tmp4, i64 4
  %29 = load i8, ptr %28, align 4
  %30 = insertvalue %P5 %27, i8 %29, 4
  %vaarg.next11 = getelementptr i8, ptr %vaarg.cursor3, i64 8
  store ptr %vaarg.next11, ptr %ap, align 4
  %second.byte = extractvalue %P5 %30, 4
  %second.extended = zext i8 %second.byte to i32
  %result = add i32 %called, %second.extended
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %result
}

define i32 @nonleaf_named_byval_split(i32 %a, i32 %b, i32 %c, ptr byval(%B8) align 4 %named, ...) nounwind {
	%last = getelementptr %B8, ptr %named, i32 0, i32 7
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%first = va_arg ptr %ap, i32
	%called = call i32 @nested5(i32 %first, i32 %a, i32 %b, i32 %c, i32 5)
	%second = va_arg ptr %ap, i32
	%byte = load i8, ptr %last, align 1
	%extended = zext i8 %byte to i32
	%sum = add i32 %called, %second
	%result = add i32 %sum, %extended
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}
