; Current LLVM requires frontends to expand aggregate va_arg.
; The original cases now use legal per-endian IR in Inputs; scalar va_arg
; instructions and ABI/stack checks are retained. No verifier is disabled here.
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %S/Inputs/varargs-aggregates-be.ll -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %S/Inputs/varargs-aggregates-le.ll -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -print-after=sh-prepare-ir %S/Inputs/varargs-aggregates-be.ll -o /dev/null 2>&1 | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -print-after=sh-prepare-ir %S/Inputs/varargs-aggregates-le.ll -o /dev/null 2>&1 | FileCheck %s --check-prefixes=COMMON,LE
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
; COMMON-LABEL: define i32 @call_p1
; COMMON: call i32 (i32, ...) @take_p1
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
; COMMON-LABEL: define i32 @call_p2
; COMMON: call i32 (i32, ...) @take_p2
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
; COMMON-LABEL: define i32 @call_p3
; COMMON: call i32 (i32, ...) @take_p3
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
; COMMON-LABEL: define i32 @call_p5
; COMMON: call i32 (i32, ...) @take_p5
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
; COMMON-LABEL: define i32 @call_p6
; COMMON: call i32 (i32, ...) @take_p6
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
; COMMON-LABEL: define i32 @call_p7
; COMMON: call i32 (i32, ...) @take_p7
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
; COMMON-LABEL: define i32 @call_p9
; COMMON: call i32 (i32, ...) @take_p9
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
; COMMON-LABEL: define i32 @call_a5
; COMMON: call i32 (i32, ...) @take_a5
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
; COMMON-LABEL: define i32 @call_a7
; COMMON: call i32 (i32, ...) @take_a7
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
; COMMON-LABEL: define i32 @call_n5
; COMMON: call i32 (i32, ...) @take_n5
