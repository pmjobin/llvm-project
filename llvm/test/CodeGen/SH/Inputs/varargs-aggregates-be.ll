; Expanded aggregate va_arg witness for sh-unknown-elf.
; Original C7 input and one-time conversion are documented in SHRecovery.json.

%P1 = type <{ i8 }>
%P2 = type <{ i8, i8 }>
%P3 = type <{ i8, i8, i8 }>
%P5 = type <{ i8, i8, i8, i8, i8 }>
%P6 = type <{ i8, i8, i8, i8, i8, i8 }>
%P7 = type <{ i8, i8, i8, i8, i8, i8, i8 }>
%P9 = type <{ i8, i8, i8, i8, i8, i8, i8, i8, i8 }>
%N5 = type <{ %P3, i8, i8 }>

declare void @llvm.va_start.p0(ptr)
declare void @llvm.va_end.p0(ptr)

define i32 @take_p1(i32 %tag, ...) {
  %vaarg.tmp = alloca %P1, align 4
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %vaarg.cursor = load ptr, ptr %ap, align 4
  %vaarg.source = getelementptr i8, ptr %vaarg.cursor, i64 3
  %vaarg.byte = load i8, ptr %vaarg.source, align 1
  %vaarg.destination = getelementptr i8, ptr %vaarg.tmp, i64 0
  store i8 %vaarg.byte, ptr %vaarg.destination, align 1
  %1 = getelementptr i8, ptr %vaarg.tmp, i64 0
  %2 = load i8, ptr %1, align 4
  %3 = insertvalue %P1 poison, i8 %2, 0
  %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 4
  store ptr %vaarg.next, ptr %ap, align 4
  %byte = extractvalue %P1 %3, 0
  %result = zext i8 %byte to i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %result
}

define i32 @call_p1() {
	%result = call i32 (i32, ...) @take_p1(i32 1, %P1 zeroinitializer)
	ret i32 %result
}

define i32 @take_p2(i32 %tag, ...) {
  %vaarg.tmp = alloca %P2, align 4
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %vaarg.cursor = load ptr, ptr %ap, align 4
  %vaarg.source = getelementptr i8, ptr %vaarg.cursor, i64 2
  %vaarg.byte = load i8, ptr %vaarg.source, align 1
  %vaarg.destination = getelementptr i8, ptr %vaarg.tmp, i64 0
  store i8 %vaarg.byte, ptr %vaarg.destination, align 1
  %vaarg.source1 = getelementptr i8, ptr %vaarg.cursor, i64 3
  %vaarg.byte2 = load i8, ptr %vaarg.source1, align 1
  %vaarg.destination3 = getelementptr i8, ptr %vaarg.tmp, i64 1
  store i8 %vaarg.byte2, ptr %vaarg.destination3, align 1
  %1 = getelementptr i8, ptr %vaarg.tmp, i64 0
  %2 = load i8, ptr %1, align 4
  %3 = insertvalue %P2 poison, i8 %2, 0
  %4 = getelementptr i8, ptr %vaarg.tmp, i64 1
  %5 = load i8, ptr %4, align 1
  %6 = insertvalue %P2 %3, i8 %5, 1
  %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 4
  store ptr %vaarg.next, ptr %ap, align 4
  %byte = extractvalue %P2 %6, 1
  %result = zext i8 %byte to i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %result
}

define i32 @call_p2() {
	%result = call i32 (i32, ...) @take_p2(i32 1, %P2 zeroinitializer)
	ret i32 %result
}

define i32 @take_p3(i32 %tag, ...) {
  %vaarg.tmp = alloca %P3, align 4
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %vaarg.cursor = load ptr, ptr %ap, align 4
  %vaarg.source = getelementptr i8, ptr %vaarg.cursor, i64 1
  %vaarg.byte = load i8, ptr %vaarg.source, align 1
  %vaarg.destination = getelementptr i8, ptr %vaarg.tmp, i64 0
  store i8 %vaarg.byte, ptr %vaarg.destination, align 1
  %vaarg.source1 = getelementptr i8, ptr %vaarg.cursor, i64 2
  %vaarg.byte2 = load i8, ptr %vaarg.source1, align 1
  %vaarg.destination3 = getelementptr i8, ptr %vaarg.tmp, i64 1
  store i8 %vaarg.byte2, ptr %vaarg.destination3, align 1
  %vaarg.source4 = getelementptr i8, ptr %vaarg.cursor, i64 3
  %vaarg.byte5 = load i8, ptr %vaarg.source4, align 1
  %vaarg.destination6 = getelementptr i8, ptr %vaarg.tmp, i64 2
  store i8 %vaarg.byte5, ptr %vaarg.destination6, align 1
  %1 = getelementptr i8, ptr %vaarg.tmp, i64 0
  %2 = load i8, ptr %1, align 4
  %3 = insertvalue %P3 poison, i8 %2, 0
  %4 = getelementptr i8, ptr %vaarg.tmp, i64 1
  %5 = load i8, ptr %4, align 1
  %6 = insertvalue %P3 %3, i8 %5, 1
  %7 = getelementptr i8, ptr %vaarg.tmp, i64 2
  %8 = load i8, ptr %7, align 2
  %9 = insertvalue %P3 %6, i8 %8, 2
  %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 4
  store ptr %vaarg.next, ptr %ap, align 4
  %byte = extractvalue %P3 %9, 2
  %result = zext i8 %byte to i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %result
}

define i32 @call_p3() {
	%result = call i32 (i32, ...) @take_p3(i32 1, %P3 zeroinitializer)
	ret i32 %result
}

define i32 @take_p5(i32 %tag, ...) {
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
  %byte = extractvalue %P5 %15, 4
  %result = zext i8 %byte to i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %result
}

define i32 @call_p5() {
	%result = call i32 (i32, ...) @take_p5(i32 1, %P5 zeroinitializer)
	ret i32 %result
}

define i32 @take_p6(i32 %tag, ...) {
  %vaarg.tmp = alloca %P6, align 4
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %vaarg.cursor = load ptr, ptr %ap, align 4
  %vaarg.source = getelementptr i8, ptr %vaarg.cursor, i64 0
  %vaarg.chunk = load i32, ptr %vaarg.source, align 4
  %vaarg.destination = getelementptr i8, ptr %vaarg.tmp, i64 0
  store i32 %vaarg.chunk, ptr %vaarg.destination, align 4
  %vaarg.source1 = getelementptr i8, ptr %vaarg.cursor, i64 6
  %vaarg.byte = load i8, ptr %vaarg.source1, align 1
  %vaarg.destination2 = getelementptr i8, ptr %vaarg.tmp, i64 4
  store i8 %vaarg.byte, ptr %vaarg.destination2, align 1
  %vaarg.source3 = getelementptr i8, ptr %vaarg.cursor, i64 7
  %vaarg.byte4 = load i8, ptr %vaarg.source3, align 1
  %vaarg.destination5 = getelementptr i8, ptr %vaarg.tmp, i64 5
  store i8 %vaarg.byte4, ptr %vaarg.destination5, align 1
  %1 = getelementptr i8, ptr %vaarg.tmp, i64 0
  %2 = load i8, ptr %1, align 4
  %3 = insertvalue %P6 poison, i8 %2, 0
  %4 = getelementptr i8, ptr %vaarg.tmp, i64 1
  %5 = load i8, ptr %4, align 1
  %6 = insertvalue %P6 %3, i8 %5, 1
  %7 = getelementptr i8, ptr %vaarg.tmp, i64 2
  %8 = load i8, ptr %7, align 2
  %9 = insertvalue %P6 %6, i8 %8, 2
  %10 = getelementptr i8, ptr %vaarg.tmp, i64 3
  %11 = load i8, ptr %10, align 1
  %12 = insertvalue %P6 %9, i8 %11, 3
  %13 = getelementptr i8, ptr %vaarg.tmp, i64 4
  %14 = load i8, ptr %13, align 4
  %15 = insertvalue %P6 %12, i8 %14, 4
  %16 = getelementptr i8, ptr %vaarg.tmp, i64 5
  %17 = load i8, ptr %16, align 1
  %18 = insertvalue %P6 %15, i8 %17, 5
  %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
  store ptr %vaarg.next, ptr %ap, align 4
  %byte = extractvalue %P6 %18, 5
  %result = zext i8 %byte to i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %result
}

define i32 @call_p6() {
	%result = call i32 (i32, ...) @take_p6(i32 1, %P6 zeroinitializer)
	ret i32 %result
}

define i32 @take_p7(i32 %tag, ...) {
  %vaarg.tmp = alloca %P7, align 4
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %vaarg.cursor = load ptr, ptr %ap, align 4
  %vaarg.source = getelementptr i8, ptr %vaarg.cursor, i64 0
  %vaarg.chunk = load i32, ptr %vaarg.source, align 4
  %vaarg.destination = getelementptr i8, ptr %vaarg.tmp, i64 0
  store i32 %vaarg.chunk, ptr %vaarg.destination, align 4
  %vaarg.source1 = getelementptr i8, ptr %vaarg.cursor, i64 5
  %vaarg.byte = load i8, ptr %vaarg.source1, align 1
  %vaarg.destination2 = getelementptr i8, ptr %vaarg.tmp, i64 4
  store i8 %vaarg.byte, ptr %vaarg.destination2, align 1
  %vaarg.source3 = getelementptr i8, ptr %vaarg.cursor, i64 6
  %vaarg.byte4 = load i8, ptr %vaarg.source3, align 1
  %vaarg.destination5 = getelementptr i8, ptr %vaarg.tmp, i64 5
  store i8 %vaarg.byte4, ptr %vaarg.destination5, align 1
  %vaarg.source6 = getelementptr i8, ptr %vaarg.cursor, i64 7
  %vaarg.byte7 = load i8, ptr %vaarg.source6, align 1
  %vaarg.destination8 = getelementptr i8, ptr %vaarg.tmp, i64 6
  store i8 %vaarg.byte7, ptr %vaarg.destination8, align 1
  %1 = getelementptr i8, ptr %vaarg.tmp, i64 0
  %2 = load i8, ptr %1, align 4
  %3 = insertvalue %P7 poison, i8 %2, 0
  %4 = getelementptr i8, ptr %vaarg.tmp, i64 1
  %5 = load i8, ptr %4, align 1
  %6 = insertvalue %P7 %3, i8 %5, 1
  %7 = getelementptr i8, ptr %vaarg.tmp, i64 2
  %8 = load i8, ptr %7, align 2
  %9 = insertvalue %P7 %6, i8 %8, 2
  %10 = getelementptr i8, ptr %vaarg.tmp, i64 3
  %11 = load i8, ptr %10, align 1
  %12 = insertvalue %P7 %9, i8 %11, 3
  %13 = getelementptr i8, ptr %vaarg.tmp, i64 4
  %14 = load i8, ptr %13, align 4
  %15 = insertvalue %P7 %12, i8 %14, 4
  %16 = getelementptr i8, ptr %vaarg.tmp, i64 5
  %17 = load i8, ptr %16, align 1
  %18 = insertvalue %P7 %15, i8 %17, 5
  %19 = getelementptr i8, ptr %vaarg.tmp, i64 6
  %20 = load i8, ptr %19, align 2
  %21 = insertvalue %P7 %18, i8 %20, 6
  %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
  store ptr %vaarg.next, ptr %ap, align 4
  %byte = extractvalue %P7 %21, 6
  %result = zext i8 %byte to i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %result
}

define i32 @call_p7() {
	%result = call i32 (i32, ...) @take_p7(i32 1, %P7 zeroinitializer)
	ret i32 %result
}

define i32 @take_p9(i32 %tag, ...) {
  %vaarg.tmp = alloca %P9, align 4
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
  %vaarg.source4 = getelementptr i8, ptr %vaarg.cursor, i64 11
  %vaarg.byte = load i8, ptr %vaarg.source4, align 1
  %vaarg.destination5 = getelementptr i8, ptr %vaarg.tmp, i64 8
  store i8 %vaarg.byte, ptr %vaarg.destination5, align 1
  %1 = getelementptr i8, ptr %vaarg.tmp, i64 0
  %2 = load i8, ptr %1, align 4
  %3 = insertvalue %P9 poison, i8 %2, 0
  %4 = getelementptr i8, ptr %vaarg.tmp, i64 1
  %5 = load i8, ptr %4, align 1
  %6 = insertvalue %P9 %3, i8 %5, 1
  %7 = getelementptr i8, ptr %vaarg.tmp, i64 2
  %8 = load i8, ptr %7, align 2
  %9 = insertvalue %P9 %6, i8 %8, 2
  %10 = getelementptr i8, ptr %vaarg.tmp, i64 3
  %11 = load i8, ptr %10, align 1
  %12 = insertvalue %P9 %9, i8 %11, 3
  %13 = getelementptr i8, ptr %vaarg.tmp, i64 4
  %14 = load i8, ptr %13, align 4
  %15 = insertvalue %P9 %12, i8 %14, 4
  %16 = getelementptr i8, ptr %vaarg.tmp, i64 5
  %17 = load i8, ptr %16, align 1
  %18 = insertvalue %P9 %15, i8 %17, 5
  %19 = getelementptr i8, ptr %vaarg.tmp, i64 6
  %20 = load i8, ptr %19, align 2
  %21 = insertvalue %P9 %18, i8 %20, 6
  %22 = getelementptr i8, ptr %vaarg.tmp, i64 7
  %23 = load i8, ptr %22, align 1
  %24 = insertvalue %P9 %21, i8 %23, 7
  %25 = getelementptr i8, ptr %vaarg.tmp, i64 8
  %26 = load i8, ptr %25, align 4
  %27 = insertvalue %P9 %24, i8 %26, 8
  %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 12
  store ptr %vaarg.next, ptr %ap, align 4
  %byte = extractvalue %P9 %27, 8
  %result = zext i8 %byte to i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %result
}

define i32 @call_p9() {
	%result = call i32 (i32, ...) @take_p9(i32 1, %P9 zeroinitializer)
	ret i32 %result
}

define i32 @take_a5(i32 %tag, ...) {
  %vaarg.tmp = alloca [5 x i8], align 4
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
  %3 = insertvalue [5 x i8] poison, i8 %2, 0
  %4 = getelementptr i8, ptr %vaarg.tmp, i64 1
  %5 = load i8, ptr %4, align 1
  %6 = insertvalue [5 x i8] %3, i8 %5, 1
  %7 = getelementptr i8, ptr %vaarg.tmp, i64 2
  %8 = load i8, ptr %7, align 2
  %9 = insertvalue [5 x i8] %6, i8 %8, 2
  %10 = getelementptr i8, ptr %vaarg.tmp, i64 3
  %11 = load i8, ptr %10, align 1
  %12 = insertvalue [5 x i8] %9, i8 %11, 3
  %13 = getelementptr i8, ptr %vaarg.tmp, i64 4
  %14 = load i8, ptr %13, align 4
  %15 = insertvalue [5 x i8] %12, i8 %14, 4
  %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
  store ptr %vaarg.next, ptr %ap, align 4
  %byte = extractvalue [5 x i8] %15, 4
  %result = zext i8 %byte to i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %result
}

define i32 @call_a5() {
	%result = call i32 (i32, ...) @take_a5(i32 1, [5 x i8] zeroinitializer)
	ret i32 %result
}

define i32 @take_a7(i32 %tag, ...) {
  %vaarg.tmp = alloca [7 x i8], align 4
  %ap = alloca ptr, align 4
  call void @llvm.va_start.p0(ptr %ap)
  %vaarg.cursor = load ptr, ptr %ap, align 4
  %vaarg.source = getelementptr i8, ptr %vaarg.cursor, i64 0
  %vaarg.chunk = load i32, ptr %vaarg.source, align 4
  %vaarg.destination = getelementptr i8, ptr %vaarg.tmp, i64 0
  store i32 %vaarg.chunk, ptr %vaarg.destination, align 4
  %vaarg.source1 = getelementptr i8, ptr %vaarg.cursor, i64 5
  %vaarg.byte = load i8, ptr %vaarg.source1, align 1
  %vaarg.destination2 = getelementptr i8, ptr %vaarg.tmp, i64 4
  store i8 %vaarg.byte, ptr %vaarg.destination2, align 1
  %vaarg.source3 = getelementptr i8, ptr %vaarg.cursor, i64 6
  %vaarg.byte4 = load i8, ptr %vaarg.source3, align 1
  %vaarg.destination5 = getelementptr i8, ptr %vaarg.tmp, i64 5
  store i8 %vaarg.byte4, ptr %vaarg.destination5, align 1
  %vaarg.source6 = getelementptr i8, ptr %vaarg.cursor, i64 7
  %vaarg.byte7 = load i8, ptr %vaarg.source6, align 1
  %vaarg.destination8 = getelementptr i8, ptr %vaarg.tmp, i64 6
  store i8 %vaarg.byte7, ptr %vaarg.destination8, align 1
  %1 = getelementptr i8, ptr %vaarg.tmp, i64 0
  %2 = load i8, ptr %1, align 4
  %3 = insertvalue [7 x i8] poison, i8 %2, 0
  %4 = getelementptr i8, ptr %vaarg.tmp, i64 1
  %5 = load i8, ptr %4, align 1
  %6 = insertvalue [7 x i8] %3, i8 %5, 1
  %7 = getelementptr i8, ptr %vaarg.tmp, i64 2
  %8 = load i8, ptr %7, align 2
  %9 = insertvalue [7 x i8] %6, i8 %8, 2
  %10 = getelementptr i8, ptr %vaarg.tmp, i64 3
  %11 = load i8, ptr %10, align 1
  %12 = insertvalue [7 x i8] %9, i8 %11, 3
  %13 = getelementptr i8, ptr %vaarg.tmp, i64 4
  %14 = load i8, ptr %13, align 4
  %15 = insertvalue [7 x i8] %12, i8 %14, 4
  %16 = getelementptr i8, ptr %vaarg.tmp, i64 5
  %17 = load i8, ptr %16, align 1
  %18 = insertvalue [7 x i8] %15, i8 %17, 5
  %19 = getelementptr i8, ptr %vaarg.tmp, i64 6
  %20 = load i8, ptr %19, align 2
  %21 = insertvalue [7 x i8] %18, i8 %20, 6
  %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
  store ptr %vaarg.next, ptr %ap, align 4
  %byte = extractvalue [7 x i8] %21, 6
  %result = zext i8 %byte to i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %result
}

define i32 @call_a7() {
	%result = call i32 (i32, ...) @take_a7(i32 1, [7 x i8] zeroinitializer)
	ret i32 %result
}

define i32 @take_n5(i32 %tag, ...) {
  %vaarg.tmp = alloca %N5, align 4
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
  %3 = insertvalue %N5 poison, i8 %2, 0, 0
  %4 = getelementptr i8, ptr %vaarg.tmp, i64 1
  %5 = load i8, ptr %4, align 1
  %6 = insertvalue %N5 %3, i8 %5, 0, 1
  %7 = getelementptr i8, ptr %vaarg.tmp, i64 2
  %8 = load i8, ptr %7, align 2
  %9 = insertvalue %N5 %6, i8 %8, 0, 2
  %10 = getelementptr i8, ptr %vaarg.tmp, i64 3
  %11 = load i8, ptr %10, align 1
  %12 = insertvalue %N5 %9, i8 %11, 1
  %13 = getelementptr i8, ptr %vaarg.tmp, i64 4
  %14 = load i8, ptr %13, align 4
  %15 = insertvalue %N5 %12, i8 %14, 2
  %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
  store ptr %vaarg.next, ptr %ap, align 4
  %byte = extractvalue %N5 %15, 2
  %result = zext i8 %byte to i32
  call void @llvm.va_end.p0(ptr %ap)
  ret i32 %result
}

define i32 @call_n5() {
	%result = call i32 (i32, ...) @take_n5(i32 1, %N5 zeroinitializer)
	ret i32 %result
}
