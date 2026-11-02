; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-before=sh-delay-slot-filler < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs < %s -o /dev/null

declare i32 @callee(i32)

define i32 @zext_eq(i32 %a, i32 %b) {
; ISEL-LABEL: name: zext_eq
; ISEL: CMP_EQ
; ISEL-NEXT: $r0 = MOVT implicit $tbit
; ASM-LABEL: zext_eq:
; ASM: cmp/eq
; ASM-NEXT: rts
; ASM-NEXT: movt
	%condition = icmp eq i32 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @sext_ne(i32 %a, i32 %b) {
	%condition = icmp ne i32 %a, %b
	%result = sext i1 %condition to i32
	ret i32 %result
}

define i32 @select_i32(i32 %a, i32 %b) {
	%condition = icmp slt i32 %a, %b
	%result = select i1 %condition, i32 %a, i32 %b
	ret i32 %result
}

define void @store_i1(ptr %p, i32 %a, i32 %b) {
	%condition = icmp ugt i32 %a, %b
	store i1 %condition, ptr %p, align 1
	ret void
}

define i32 @live_across_call(i32 %a, i32 %b, i32 %value) {
	%condition = icmp sge i32 %a, %b
	%called = call i32 @callee(i32 %value)
	%boolean = zext i1 %condition to i32
	%result = add i32 %called, %boolean
	ret i32 %result
}

define i32 @value_i32_ne(i32 %a, i32 %b) {
; ASM-LABEL: value_i32_ne:
; ASM: cmp/eq
; ASM: movt
; ASM: xor
	%condition = icmp ne i32 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i32_slt(i32 %a, i32 %b) {
	%condition = icmp slt i32 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i32_sle(i32 %a, i32 %b) {
	%condition = icmp sle i32 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i32_sgt(i32 %a, i32 %b) {
	%condition = icmp sgt i32 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i32_sge(i32 %a, i32 %b) {
	%condition = icmp sge i32 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i32_ult(i32 %a, i32 %b) {
	%condition = icmp ult i32 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i32_ule(i32 %a, i32 %b) {
	%condition = icmp ule i32 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i32_ugt(i32 %a, i32 %b) {
	%condition = icmp ugt i32 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i32_uge(i32 %a, i32 %b) {
	%condition = icmp uge i32 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i8_eq(i32 %a, i32 %b) {
	%a8 = trunc i32 %a to i8
	%b8 = trunc i32 %b to i8
	%condition = icmp eq i8 %a8, %b8
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i8_ne(i32 %a, i32 %b) {
	%a8 = trunc i32 %a to i8
	%b8 = trunc i32 %b to i8
	%condition = icmp ne i8 %a8, %b8
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i8_slt(i32 %a, i32 %b) {
	%a8 = trunc i32 %a to i8
	%b8 = trunc i32 %b to i8
	%condition = icmp slt i8 %a8, %b8
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i8_sle(i32 %a, i32 %b) {
	%a8 = trunc i32 %a to i8
	%b8 = trunc i32 %b to i8
	%condition = icmp sle i8 %a8, %b8
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i8_sgt(i32 %a, i32 %b) {
	%a8 = trunc i32 %a to i8
	%b8 = trunc i32 %b to i8
	%condition = icmp sgt i8 %a8, %b8
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i8_sge(i32 %a, i32 %b) {
	%a8 = trunc i32 %a to i8
	%b8 = trunc i32 %b to i8
	%condition = icmp sge i8 %a8, %b8
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i8_ult(i32 %a, i32 %b) {
	%a8 = trunc i32 %a to i8
	%b8 = trunc i32 %b to i8
	%condition = icmp ult i8 %a8, %b8
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i8_ule(i32 %a, i32 %b) {
	%a8 = trunc i32 %a to i8
	%b8 = trunc i32 %b to i8
	%condition = icmp ule i8 %a8, %b8
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i8_ugt(i32 %a, i32 %b) {
	%a8 = trunc i32 %a to i8
	%b8 = trunc i32 %b to i8
	%condition = icmp ugt i8 %a8, %b8
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i8_uge(i32 %a, i32 %b) {
	%a8 = trunc i32 %a to i8
	%b8 = trunc i32 %b to i8
	%condition = icmp uge i8 %a8, %b8
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i16_eq(i32 %a, i32 %b) {
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%condition = icmp eq i16 %a16, %b16
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i16_ne(i32 %a, i32 %b) {
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%condition = icmp ne i16 %a16, %b16
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i16_slt(i32 %a, i32 %b) {
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%condition = icmp slt i16 %a16, %b16
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i16_sle(i32 %a, i32 %b) {
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%condition = icmp sle i16 %a16, %b16
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i16_sgt(i32 %a, i32 %b) {
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%condition = icmp sgt i16 %a16, %b16
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i16_sge(i32 %a, i32 %b) {
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%condition = icmp sge i16 %a16, %b16
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i16_ult(i32 %a, i32 %b) {
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%condition = icmp ult i16 %a16, %b16
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i16_ule(i32 %a, i32 %b) {
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%condition = icmp ule i16 %a16, %b16
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i16_ugt(i32 %a, i32 %b) {
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%condition = icmp ugt i16 %a16, %b16
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i16_uge(i32 %a, i32 %b) {
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%condition = icmp uge i16 %a16, %b16
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_ptr_eq(ptr %a, ptr %b) {
	%condition = icmp eq ptr %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_ptr_ne(ptr %a, ptr %b) {
	%condition = icmp ne ptr %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_ptr_slt(ptr %a, ptr %b) {
	%condition = icmp slt ptr %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_ptr_sle(ptr %a, ptr %b) {
	%condition = icmp sle ptr %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_ptr_sgt(ptr %a, ptr %b) {
	%condition = icmp sgt ptr %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_ptr_sge(ptr %a, ptr %b) {
	%condition = icmp sge ptr %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_ptr_ult(ptr %a, ptr %b) {
	%condition = icmp ult ptr %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_ptr_ule(ptr %a, ptr %b) {
	%condition = icmp ule ptr %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_ptr_ugt(ptr %a, ptr %b) {
	%condition = icmp ugt ptr %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_ptr_uge(ptr %a, ptr %b) {
	%condition = icmp uge ptr %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i64_eq(i64 %a, i64 %b) {
	%condition = icmp eq i64 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i64_ne(i64 %a, i64 %b) {
	%condition = icmp ne i64 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i64_slt(i64 %a, i64 %b) {
	%condition = icmp slt i64 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i64_sle(i64 %a, i64 %b) {
	%condition = icmp sle i64 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i64_sgt(i64 %a, i64 %b) {
	%condition = icmp sgt i64 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i64_sge(i64 %a, i64 %b) {
	%condition = icmp sge i64 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i64_ult(i64 %a, i64 %b) {
	%condition = icmp ult i64 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i64_ule(i64 %a, i64 %b) {
	%condition = icmp ule i64 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i64_ugt(i64 %a, i64 %b) {
	%condition = icmp ugt i64 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @value_i64_uge(i64 %a, i64 %b) {
	%condition = icmp uge i64 %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define void @extension_widths(ptr %z8, ptr %z16, ptr %z32, ptr %s8, ptr %s16, ptr %s32, i32 %a, i32 %b) {
	%condition = icmp ult i32 %a, %b
	%zero8 = zext i1 %condition to i8
	%zero16 = zext i1 %condition to i16
	%zero32 = zext i1 %condition to i32
	%sign8 = sext i1 %condition to i8
	%sign16 = sext i1 %condition to i16
	%sign32 = sext i1 %condition to i32
	store i8 %zero8, ptr %z8, align 1
	store i16 %zero16, ptr %z16, align 1
	store i32 %zero32, ptr %z32, align 1
	store i8 %sign8, ptr %s8, align 1
	store i16 %sign16, ptr %s16, align 1
	store i32 %sign32, ptr %s32, align 1
	ret void
}

define i32 @logical_arithmetic(i32 %a, i32 %b, i32 %value) {
	%eq = icmp eq i32 %a, %b
	%lt = icmp slt i32 %a, %b
	%xor = xor i1 %eq, true
	%and = and i1 %xor, %lt
	%or = or i1 %and, %eq
	%boolean = zext i1 %or to i32
	%result = add i32 %boolean, %value
	ret i32 %result
}

define i32 @truncate_widen_chain(i32 %a, i32 %b) {
	%condition = icmp uge i32 %a, %b
	%wide = zext i1 %condition to i32
	%narrow = trunc i32 %wide to i8
	%result = zext i8 %narrow to i32
	ret i32 %result
}

define i32 @comparison_constants(i32 %value) {
	%zero = icmp eq i32 %value, 0
	%one = icmp ne i32 %value, 1
	%minus.one = icmp sgt i32 %value, -1
	%minimum = icmp eq i32 %value, -2147483648
	%maximum = icmp eq i32 %value, 2147483647
	%unsigned.maximum = icmp ult i32 %value, -1
	%swapped = icmp slt i32 1, %value
	%zero.value = zext i1 %zero to i32
	%one.value = zext i1 %one to i32
	%minus.one.value = zext i1 %minus.one to i32
	%minimum.value = zext i1 %minimum to i32
	%maximum.value = zext i1 %maximum to i32
	%unsigned.maximum.value = zext i1 %unsigned.maximum to i32
	%swapped.value = zext i1 %swapped to i32
	%sum.0 = add i32 %zero.value, %one.value
	%sum.1 = add i32 %sum.0, %minus.one.value
	%sum.2 = add i32 %sum.1, %minimum.value
	%sum.3 = add i32 %sum.2, %maximum.value
	%sum.4 = add i32 %sum.3, %unsigned.maximum.value
	%sum.5 = add i32 %sum.4, %swapped.value
	ret i32 %sum.5
}

define i32 @boolean_array_index(ptr %base, i32 %a, i32 %b) {
	%condition = icmp ult i32 %a, %b
	%index = zext i1 %condition to i32
	%element = getelementptr i32, ptr %base, i32 %index
	%result = load i32, ptr %element, align 1
	ret i32 %result
}

define i32 @phi_value(i32 %a, i32 %b, i32 %selector) {
entry:
	%first = icmp eq i32 %a, %b
	%choose = icmp ne i32 %selector, 0
	br i1 %choose, label %left, label %right
left:
	%second = icmp ugt i32 %a, %b
	br label %merge
right:
	br label %merge
merge:
	%condition = phi i1 [ %second, %left ], [ %first, %right ]
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i32 @load_store_round_trip(ptr %p, i32 %a, i32 %b) {
	%condition = icmp sle i32 %a, %b
	store i1 %condition, ptr %p, align 1
	%reloaded = load i1, ptr %p, align 1
	%result = zext i1 %reloaded to i32
	ret i32 %result
}

define i32 @mixed_branch_and_value(ptr %p, i32 %a, i32 %b) {
	%condition = icmp sgt i32 %a, %b
	%value = zext i1 %condition to i32
	store i1 %condition, ptr %p, align 1
	br i1 %condition, label %true, label %false
true:
	%true.result = add i32 %value, 10
	ret i32 %true.result
false:
	%false.result = add i32 %value, 20
	ret i32 %false.result
}

define i32 @multiple_comparisons(i32 %a, i32 %b, i32 %c) {
	%first = icmp ult i32 %a, %b
	%second = icmp sge i32 %b, %c
	%first.value = zext i1 %first to i32
	%second.value = zext i1 %second to i32
	%scaled = shl i32 %first.value, 1
	%result = or i32 %scaled, %second.value
	ret i32 %result
}

define i32 @branch_only(i32 %a, i32 %b) {
; ASM-LABEL: branch_only:
; ASM: cmp/hi
; ASM-NOT: movt
; ASM: {{bt|bf}}
	%condition = icmp ugt i32 %a, %b
	br i1 %condition, label %true, label %false
true:
	ret i32 %a
false:
	ret i32 %b
}
