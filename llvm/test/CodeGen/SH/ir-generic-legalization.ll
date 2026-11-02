; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null

%Pair = type { i1, i8 }
%Nested = type { i1, [2 x %Pair] }

declare i24 @external_i24(i24)

; CHECK-LABEL: narrow_call:
; CHECK: jsr
; CHECK: rts
define i24 @narrow_call(i24 %value) {
	%result = call i24 @external_i24(i24 %value)
	ret i24 %result
}

; CHECK-LABEL: unusual_integer_memory:
; CHECK: mov.b
; CHECK: mov.b
; CHECK: rts
define i32 @unusual_integer_memory(ptr %address, i40 %increment) {
	%loaded = load i32, ptr %address, align 1
	%extended = zext i32 %loaded to i40
	%sum = add i40 %extended, %increment
	%result = trunc i40 %sum to i32
	store i32 %result, ptr %address, align 1
	ret i32 %result
}

; CHECK-LABEL: nested_i1_phi:
; CHECK: mov.b
; CHECK: rts
define void @nested_i1_phi(i1 %condition, ptr %left, ptr %right, ptr %destination) {
entry:
	br i1 %condition, label %left.block, label %right.block

left.block:
	br label %merge

right.block:
	br label %merge

merge:
	%source = phi ptr [ %left, %left.block ], [ %right, %right.block ]
	%value = load %Nested, ptr %source, align 1
	store %Nested %value, ptr %destination, align 1
	ret void
}

; CHECK-LABEL: vector_select_memory:
; CHECK: mov.b
; CHECK: rts
define void @vector_select_memory(i1 %condition, ptr %left, ptr %right, ptr %destination) {
	%left.value = load <4 x i8>, ptr %left, align 1
	%right.value = load <4 x i8>, ptr %right, align 1
	%selected = select i1 %condition, <4 x i8> %left.value, <4 x i8> %right.value
	store <4 x i8> %selected, ptr %destination, align 1
	ret void
}

; CHECK-LABEL: half_memory_is_bit_movement:
; CHECK: mov.b
; CHECK: rts
define void @half_memory_is_bit_movement(ptr %destination, ptr %source) {
	%value = load half, ptr %source, align 1
	store half %value, ptr %destination, align 1
	ret void
}
