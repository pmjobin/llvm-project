; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -print-after=sh-lower-i64-stack-align %s -o /dev/null 2>&1 | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -print-after=sh-lower-i64-stack-align %s -o /dev/null 2>&1 | FileCheck %s --check-prefixes=COMMON,LE

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

; BE-LABEL: define i32 @take_p1
; BE: %vaarg.cursor = load ptr, ptr %ap, align 4
; BE: getelementptr i8, ptr %vaarg.cursor, i64 3
; BE-NEXT: %vaarg.byte = load i8
; BE-NOT: getelementptr i8, ptr %vaarg.cursor
; BE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 4
; LE-LABEL: define i32 @take_p1
; LE: %vaarg.cursor = load ptr, ptr %ap, align 4
; LE: getelementptr i8, ptr %vaarg.cursor, i64 0
; LE-NEXT: %vaarg.byte = load i8
; LE-NOT: getelementptr i8, ptr %vaarg.cursor
; LE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 4
define i32 @take_p1(i32 %tag, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%value = va_arg ptr %ap, %P1
	%byte = extractvalue %P1 %value, 0
	%result = zext i8 %byte to i32
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

; COMMON-LABEL: define i32 @call_p1
; COMMON: call i32 (i32, ...) @take_p1
define i32 @call_p1() {
	%result = call i32 (i32, ...) @take_p1(i32 1, %P1 zeroinitializer)
	ret i32 %result
}

; BE-LABEL: define i32 @take_p2
; BE: %vaarg.cursor = load ptr, ptr %ap, align 4
; BE: getelementptr i8, ptr %vaarg.cursor, i64 2
; BE-NEXT: %vaarg.byte = load i8
; BE: getelementptr i8, ptr %vaarg.cursor, i64 3
; BE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; BE-NOT: getelementptr i8, ptr %vaarg.cursor
; BE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 4
; LE-LABEL: define i32 @take_p2
; LE: %vaarg.cursor = load ptr, ptr %ap, align 4
; LE: getelementptr i8, ptr %vaarg.cursor, i64 0
; LE-NEXT: %vaarg.byte = load i8
; LE: getelementptr i8, ptr %vaarg.cursor, i64 1
; LE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; LE-NOT: getelementptr i8, ptr %vaarg.cursor
; LE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 4
define i32 @take_p2(i32 %tag, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%value = va_arg ptr %ap, %P2
	%byte = extractvalue %P2 %value, 1
	%result = zext i8 %byte to i32
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

; COMMON-LABEL: define i32 @call_p2
; COMMON: call i32 (i32, ...) @take_p2
define i32 @call_p2() {
	%result = call i32 (i32, ...) @take_p2(i32 1, %P2 zeroinitializer)
	ret i32 %result
}

; BE-LABEL: define i32 @take_p3
; BE: %vaarg.cursor = load ptr, ptr %ap, align 4
; BE: getelementptr i8, ptr %vaarg.cursor, i64 1
; BE-NEXT: %vaarg.byte = load i8
; BE: getelementptr i8, ptr %vaarg.cursor, i64 2
; BE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; BE: getelementptr i8, ptr %vaarg.cursor, i64 3
; BE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; BE-NOT: getelementptr i8, ptr %vaarg.cursor
; BE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 4
; LE-LABEL: define i32 @take_p3
; LE: %vaarg.cursor = load ptr, ptr %ap, align 4
; LE: getelementptr i8, ptr %vaarg.cursor, i64 0
; LE-NEXT: %vaarg.byte = load i8
; LE: getelementptr i8, ptr %vaarg.cursor, i64 1
; LE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; LE: getelementptr i8, ptr %vaarg.cursor, i64 2
; LE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; LE-NOT: getelementptr i8, ptr %vaarg.cursor
; LE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 4
define i32 @take_p3(i32 %tag, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%value = va_arg ptr %ap, %P3
	%byte = extractvalue %P3 %value, 2
	%result = zext i8 %byte to i32
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

; COMMON-LABEL: define i32 @call_p3
; COMMON: call i32 (i32, ...) @take_p3
define i32 @call_p3() {
	%result = call i32 (i32, ...) @take_p3(i32 1, %P3 zeroinitializer)
	ret i32 %result
}

; BE-LABEL: define i32 @take_p5
; BE: %vaarg.cursor = load ptr, ptr %ap, align 4
; BE: getelementptr i8, ptr %vaarg.cursor, i64 0
; BE-NEXT: %vaarg.chunk = load i32
; BE: getelementptr i8, ptr %vaarg.cursor, i64 7
; BE-NEXT: %vaarg.byte = load i8
; BE-NOT: getelementptr i8, ptr %vaarg.cursor
; BE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
; LE-LABEL: define i32 @take_p5
; LE: %vaarg.cursor = load ptr, ptr %ap, align 4
; LE: getelementptr i8, ptr %vaarg.cursor, i64 0
; LE-NEXT: %vaarg.chunk = load i32
; LE: getelementptr i8, ptr %vaarg.cursor, i64 4
; LE-NEXT: %vaarg.byte = load i8
; LE-NOT: getelementptr i8, ptr %vaarg.cursor
; LE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
define i32 @take_p5(i32 %tag, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%value = va_arg ptr %ap, %P5
	%byte = extractvalue %P5 %value, 4
	%result = zext i8 %byte to i32
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

; COMMON-LABEL: define i32 @call_p5
; COMMON: call i32 (i32, ...) @take_p5
define i32 @call_p5() {
	%result = call i32 (i32, ...) @take_p5(i32 1, %P5 zeroinitializer)
	ret i32 %result
}

; BE-LABEL: define i32 @take_p6
; BE: %vaarg.cursor = load ptr, ptr %ap, align 4
; BE: getelementptr i8, ptr %vaarg.cursor, i64 0
; BE-NEXT: %vaarg.chunk = load i32
; BE: getelementptr i8, ptr %vaarg.cursor, i64 6
; BE-NEXT: %vaarg.byte = load i8
; BE: getelementptr i8, ptr %vaarg.cursor, i64 7
; BE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; BE-NOT: getelementptr i8, ptr %vaarg.cursor
; BE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
; LE-LABEL: define i32 @take_p6
; LE: %vaarg.cursor = load ptr, ptr %ap, align 4
; LE: getelementptr i8, ptr %vaarg.cursor, i64 0
; LE-NEXT: %vaarg.chunk = load i32
; LE: getelementptr i8, ptr %vaarg.cursor, i64 4
; LE-NEXT: %vaarg.byte = load i8
; LE: getelementptr i8, ptr %vaarg.cursor, i64 5
; LE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; LE-NOT: getelementptr i8, ptr %vaarg.cursor
; LE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
define i32 @take_p6(i32 %tag, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%value = va_arg ptr %ap, %P6
	%byte = extractvalue %P6 %value, 5
	%result = zext i8 %byte to i32
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

; COMMON-LABEL: define i32 @call_p6
; COMMON: call i32 (i32, ...) @take_p6
define i32 @call_p6() {
	%result = call i32 (i32, ...) @take_p6(i32 1, %P6 zeroinitializer)
	ret i32 %result
}

; BE-LABEL: define i32 @take_p7
; BE: %vaarg.cursor = load ptr, ptr %ap, align 4
; BE: getelementptr i8, ptr %vaarg.cursor, i64 0
; BE-NEXT: %vaarg.chunk = load i32
; BE: getelementptr i8, ptr %vaarg.cursor, i64 5
; BE-NEXT: %vaarg.byte = load i8
; BE: getelementptr i8, ptr %vaarg.cursor, i64 6
; BE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; BE: getelementptr i8, ptr %vaarg.cursor, i64 7
; BE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; BE-NOT: getelementptr i8, ptr %vaarg.cursor
; BE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
; LE-LABEL: define i32 @take_p7
; LE: %vaarg.cursor = load ptr, ptr %ap, align 4
; LE: getelementptr i8, ptr %vaarg.cursor, i64 0
; LE-NEXT: %vaarg.chunk = load i32
; LE: getelementptr i8, ptr %vaarg.cursor, i64 4
; LE-NEXT: %vaarg.byte = load i8
; LE: getelementptr i8, ptr %vaarg.cursor, i64 5
; LE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; LE: getelementptr i8, ptr %vaarg.cursor, i64 6
; LE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; LE-NOT: getelementptr i8, ptr %vaarg.cursor
; LE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
define i32 @take_p7(i32 %tag, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%value = va_arg ptr %ap, %P7
	%byte = extractvalue %P7 %value, 6
	%result = zext i8 %byte to i32
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

; COMMON-LABEL: define i32 @call_p7
; COMMON: call i32 (i32, ...) @take_p7
define i32 @call_p7() {
	%result = call i32 (i32, ...) @take_p7(i32 1, %P7 zeroinitializer)
	ret i32 %result
}

; BE-LABEL: define i32 @take_p9
; BE: %vaarg.cursor = load ptr, ptr %ap, align 4
; BE: getelementptr i8, ptr %vaarg.cursor, i64 0
; BE-NEXT: %vaarg.chunk = load i32
; BE: getelementptr i8, ptr %vaarg.cursor, i64 4
; BE-NEXT: %vaarg.chunk{{[0-9]*}} = load i32
; BE: getelementptr i8, ptr %vaarg.cursor, i64 11
; BE-NEXT: %vaarg.byte = load i8
; BE-NOT: getelementptr i8, ptr %vaarg.cursor
; BE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 12
; LE-LABEL: define i32 @take_p9
; LE: %vaarg.cursor = load ptr, ptr %ap, align 4
; LE: getelementptr i8, ptr %vaarg.cursor, i64 0
; LE-NEXT: %vaarg.chunk = load i32
; LE: getelementptr i8, ptr %vaarg.cursor, i64 4
; LE-NEXT: %vaarg.chunk{{[0-9]*}} = load i32
; LE: getelementptr i8, ptr %vaarg.cursor, i64 8
; LE-NEXT: %vaarg.byte = load i8
; LE-NOT: getelementptr i8, ptr %vaarg.cursor
; LE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 12
define i32 @take_p9(i32 %tag, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%value = va_arg ptr %ap, %P9
	%byte = extractvalue %P9 %value, 8
	%result = zext i8 %byte to i32
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

; COMMON-LABEL: define i32 @call_p9
; COMMON: call i32 (i32, ...) @take_p9
define i32 @call_p9() {
	%result = call i32 (i32, ...) @take_p9(i32 1, %P9 zeroinitializer)
	ret i32 %result
}

; BE-LABEL: define i32 @take_a5
; BE: %vaarg.cursor = load ptr, ptr %ap, align 4
; BE: getelementptr i8, ptr %vaarg.cursor, i64 0
; BE-NEXT: %vaarg.chunk = load i32
; BE: getelementptr i8, ptr %vaarg.cursor, i64 7
; BE-NEXT: %vaarg.byte = load i8
; BE-NOT: getelementptr i8, ptr %vaarg.cursor
; BE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
; LE-LABEL: define i32 @take_a5
; LE: %vaarg.cursor = load ptr, ptr %ap, align 4
; LE: getelementptr i8, ptr %vaarg.cursor, i64 0
; LE-NEXT: %vaarg.chunk = load i32
; LE: getelementptr i8, ptr %vaarg.cursor, i64 4
; LE-NEXT: %vaarg.byte = load i8
; LE-NOT: getelementptr i8, ptr %vaarg.cursor
; LE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
define i32 @take_a5(i32 %tag, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%value = va_arg ptr %ap, [5 x i8]
	%byte = extractvalue [5 x i8] %value, 4
	%result = zext i8 %byte to i32
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

; COMMON-LABEL: define i32 @call_a5
; COMMON: call i32 (i32, ...) @take_a5
define i32 @call_a5() {
	%result = call i32 (i32, ...) @take_a5(i32 1, [5 x i8] zeroinitializer)
	ret i32 %result
}

; BE-LABEL: define i32 @take_a7
; BE: %vaarg.cursor = load ptr, ptr %ap, align 4
; BE: getelementptr i8, ptr %vaarg.cursor, i64 0
; BE-NEXT: %vaarg.chunk = load i32
; BE: getelementptr i8, ptr %vaarg.cursor, i64 5
; BE-NEXT: %vaarg.byte = load i8
; BE: getelementptr i8, ptr %vaarg.cursor, i64 6
; BE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; BE: getelementptr i8, ptr %vaarg.cursor, i64 7
; BE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; BE-NOT: getelementptr i8, ptr %vaarg.cursor
; BE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
; LE-LABEL: define i32 @take_a7
; LE: %vaarg.cursor = load ptr, ptr %ap, align 4
; LE: getelementptr i8, ptr %vaarg.cursor, i64 0
; LE-NEXT: %vaarg.chunk = load i32
; LE: getelementptr i8, ptr %vaarg.cursor, i64 4
; LE-NEXT: %vaarg.byte = load i8
; LE: getelementptr i8, ptr %vaarg.cursor, i64 5
; LE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; LE: getelementptr i8, ptr %vaarg.cursor, i64 6
; LE-NEXT: %vaarg.byte{{[0-9]*}} = load i8
; LE-NOT: getelementptr i8, ptr %vaarg.cursor
; LE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
define i32 @take_a7(i32 %tag, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%value = va_arg ptr %ap, [7 x i8]
	%byte = extractvalue [7 x i8] %value, 6
	%result = zext i8 %byte to i32
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

; COMMON-LABEL: define i32 @call_a7
; COMMON: call i32 (i32, ...) @take_a7
define i32 @call_a7() {
	%result = call i32 (i32, ...) @take_a7(i32 1, [7 x i8] zeroinitializer)
	ret i32 %result
}

; BE-LABEL: define i32 @take_n5
; BE: %vaarg.cursor = load ptr, ptr %ap, align 4
; BE: getelementptr i8, ptr %vaarg.cursor, i64 0
; BE-NEXT: %vaarg.chunk = load i32
; BE: getelementptr i8, ptr %vaarg.cursor, i64 7
; BE-NEXT: %vaarg.byte = load i8
; BE-NOT: getelementptr i8, ptr %vaarg.cursor
; BE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
; LE-LABEL: define i32 @take_n5
; LE: %vaarg.cursor = load ptr, ptr %ap, align 4
; LE: getelementptr i8, ptr %vaarg.cursor, i64 0
; LE-NEXT: %vaarg.chunk = load i32
; LE: getelementptr i8, ptr %vaarg.cursor, i64 4
; LE-NEXT: %vaarg.byte = load i8
; LE-NOT: getelementptr i8, ptr %vaarg.cursor
; LE: %vaarg.next = getelementptr i8, ptr %vaarg.cursor, i64 8
define i32 @take_n5(i32 %tag, ...) {
	%ap = alloca ptr, align 4
	call void @llvm.va_start.p0(ptr %ap)
	%value = va_arg ptr %ap, %N5
	%byte = extractvalue %N5 %value, 2
	%result = zext i8 %byte to i32
	call void @llvm.va_end.p0(ptr %ap)
	ret i32 %result
}

; COMMON-LABEL: define i32 @call_n5
; COMMON: call i32 (i32, ...) @take_n5
define i32 @call_n5() {
	%result = call i32 (i32, ...) @take_n5(i32 1, %N5 zeroinitializer)
	ret i32 %result
}
