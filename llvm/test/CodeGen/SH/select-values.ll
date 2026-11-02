; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s -o /dev/null

%S4 = type { i32 }
%S8 = type { i32, i32 }
%S12 = type { i32, i32, i32 }
%Packed = type <{ i32 }>

declare i32 @callee(i32)

define i32 @select_i8(i32 %condition.value, i32 %a, i32 %b) {
	%condition = icmp ne i32 %condition.value, 0
	%a8 = trunc i32 %a to i8
	%b8 = trunc i32 %b to i8
	%selected = select i1 %condition, i8 %a8, i8 %b8
	%result = zext i8 %selected to i32
	ret i32 %result
}

define i32 @select_i16(i32 %condition.value, i32 %a, i32 %b) {
	%condition = icmp ne i32 %condition.value, 0
	%a16 = trunc i32 %a to i16
	%b16 = trunc i32 %b to i16
	%selected = select i1 %condition, i16 %a16, i16 %b16
	%result = zext i16 %selected to i32
	ret i32 %result
}

define i32 @select_i32(i32 %condition.value, i32 %a, i32 %b) {
; ASM-LABEL: select_i32:
; ASM: tst
; ASM: bf
	%condition = icmp ne i32 %condition.value, 0
	%selected = select i1 %condition, i32 %a, i32 %b
	ret i32 %selected
}

define i64 @select_i64(i32 %condition.value, i64 %a, i64 %b) {
	%condition = icmp ne i32 %condition.value, 0
	%selected = select i1 %condition, i64 %a, i64 %b
	ret i64 %selected
}

define ptr @select_pointer(i32 %condition.value, ptr %a, ptr %b) {
	%condition = icmp ne i32 %condition.value, 0
	%selected = select i1 %condition, ptr %a, ptr %b
	ret ptr %selected
}

define i32 @select_true_constant(i32 %a, i32 %b) {
	%selected = select i1 true, i32 %a, i32 %b
	ret i32 %selected
}

define i32 @select_false_constant(i32 %a, i32 %b) {
	%selected = select i1 false, i32 %a, i32 %b
	ret i32 %selected
}

define i32 @select_constant_arms(i32 %a, i32 %b) {
	%condition = icmp slt i32 %a, %b
	%selected = select i1 %condition, i32 17, i32 -9
	ret i32 %selected
}

define i32 @select_loaded_values(ptr %a, ptr %b, i32 %selector) {
	%left = load i32, ptr %a, align 1
	%right = load i32, ptr %b, align 1
	%condition = icmp ugt i32 %selector, 1
	%selected = select i1 %condition, i32 %left, i32 %right
	ret i32 %selected
}

define i32 @select_live_across_call(i32 %a, i32 %b, i32 %argument) {
	%condition = icmp ule i32 %a, %b
	%called = call i32 @callee(i32 %argument)
	%selected = select i1 %condition, i32 %called, i32 %argument
	ret i32 %selected
}

define i32 @selected_value_live_across_call(i32 %a, i32 %b, i32 %argument) {
	%condition = icmp ule i32 %a, %b
	%selected = select i1 %condition, i32 %a, i32 %b
	%called = call i32 @callee(i32 %argument)
	%result = add i32 %selected, %called
	ret i32 %result
}

define i32 @nested_selects(i32 %a, i32 %b, i32 %c) {
	%first.condition = icmp sge i32 %a, %b
	%second.condition = icmp ult i32 %b, %c
	%first = select i1 %first.condition, i32 %a, i32 %b
	%second = select i1 %second.condition, i32 %first, i32 %c
	ret i32 %second
}

define void @select_store(ptr %out, i32 %a, i32 %b) {
	%condition = icmp sgt i32 %a, %b
	%selected = select i1 %condition, i32 %a, i32 %b
	store i32 %selected, ptr %out, align 1
	ret void
}

define i32 @select_feeds_comparison(i32 %selector, i32 %a, i32 %b, i32 %limit) {
	%condition = icmp ne i32 %selector, 0
	%selected = select i1 %condition, i32 %a, i32 %b
	%compared = icmp uge i32 %selected, %limit
	%result = zext i1 %compared to i32
	ret i32 %result
}

define i32 @select_in_loop(i32 %count, i32 %a, i32 %b) {
entry:
	br label %loop
loop:
	%index = phi i32 [ 0, %entry ], [ %next, %loop ]
	%accumulator = phi i32 [ 0, %entry ], [ %sum, %loop ]
	%condition = icmp ult i32 %index, %a
	%selected = select i1 %condition, i32 %a, i32 %b
	%sum = add i32 %accumulator, %selected
	%next = add i32 %index, 1
	%done = icmp eq i32 %next, %count
	br i1 %done, label %exit, label %loop
exit:
	ret i32 %sum
}

define i32 @two_selects(i32 %a, i32 %b, i32 %x, i32 %y) {
	%condition = icmp ult i32 %a, %b
	%first = select i1 %condition, i32 %a, i32 %b
	%second = select i1 %condition, i32 %x, i32 %y
	%result = add i32 %first, %second
	ret i32 %result
}

define %S4 @select_s4(i32 %selector, %S4 %a, %S4 %b) {
	%condition = icmp ne i32 %selector, 0
	%selected = select i1 %condition, %S4 %a, %S4 %b
	ret %S4 %selected
}

define %S8 @select_s8(i32 %selector, %S8 %a, %S8 %b) {
	%condition = icmp ne i32 %selector, 0
	%selected = select i1 %condition, %S8 %a, %S8 %b
	ret %S8 %selected
}

define %Packed @select_packed(i32 %selector, %Packed %a, %Packed %b) {
	%condition = icmp ne i32 %selector, 0
	%selected = select i1 %condition, %Packed %a, %Packed %b
	ret %Packed %selected
}

define void @select_sret(ptr sret(%S12) %out, i32 %selector, ptr %a, ptr %b) {
	%left = load %S12, ptr %a, align 1
	%right = load %S12, ptr %b, align 1
	%condition = icmp ne i32 %selector, 0
	%selected = select i1 %condition, %S12 %left, %S12 %right
	store %S12 %selected, ptr %out, align 1
	ret void
}

define ptr @select_aggregate_address(i32 %selector, ptr %a, ptr %b) {
	%condition = icmp ne i32 %selector, 0
	%selected = select i1 %condition, ptr %a, ptr %b
	ret ptr %selected
}
