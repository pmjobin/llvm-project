; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s

%fields = type { i8, i8, i16, i32 }

define i32 @load_field0(ptr %p) nounwind {
; CHECK-LABEL: load_field0:
; CHECK-NEXT: mov.b	@r4,r0
; CHECK-NEXT: extu.b	r0,r0
	%field = getelementptr %fields, ptr %p, i32 0, i32 0
	%value = load i8, ptr %field, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @load_field1(ptr %p) nounwind {
; CHECK-LABEL: load_field1:
; CHECK-NEXT: add	#1,r4
; CHECK-NEXT: mov.b	@r4,r0
; CHECK-NEXT: extu.b	r0,r0
	%field = getelementptr %fields, ptr %p, i32 0, i32 1
	%value = load i8, ptr %field, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @load_field2(ptr %p) nounwind {
; CHECK-LABEL: load_field2:
; CHECK-NEXT: add	#2,r4
; CHECK-NEXT: mov.w	@r4,r0
	%field = getelementptr %fields, ptr %p, i32 0, i32 2
	%value = load i16, ptr %field, align 2
	%result = sext i16 %value to i32
	ret i32 %result
}

define i32 @load_field3(ptr %p) nounwind {
; CHECK-LABEL: load_field3:
; CHECK-NEXT: mov.l	@(4,r4),r0
	%field = getelementptr %fields, ptr %p, i32 0, i32 3
	%value = load i32, ptr %field, align 4
	ret i32 %value
}

define void @byte_array_constant(ptr %p, i32 %value) nounwind {
; CHECK-LABEL: byte_array_constant:
; CHECK-NEXT: add	#9,r4
; CHECK-NEXT: mov.b	r5,@r4
	%element = getelementptr [16 x i8], ptr %p, i32 0, i32 9
	%narrow = trunc i32 %value to i8
	store i8 %narrow, ptr %element, align 1
	ret void
}

define i32 @byte_array_variable(ptr %p, i32 %index) nounwind {
; CHECK-LABEL: byte_array_variable:
; CHECK: add	r5,r4
; CHECK-NEXT: mov.b	@r4,r0
; CHECK-NEXT: extu.b	r0,r0
	%element = getelementptr [16 x i8], ptr %p, i32 0, i32 %index
	%value = load i8, ptr %element, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define void @word_array_constant(ptr %p, i32 %value) nounwind {
; CHECK-LABEL: word_array_constant:
; CHECK-NEXT: add	#6,r4
; CHECK-NEXT: mov.w	r5,@r4
	%element = getelementptr [8 x i16], ptr %p, i32 0, i32 3
	%narrow = trunc i32 %value to i16
	store i16 %narrow, ptr %element, align 2
	ret void
}

define i32 @word_array_load_constant(ptr %p) nounwind {
; CHECK-LABEL: word_array_load_constant:
; CHECK-NEXT: add	#10,r4
; CHECK-NEXT: mov.w	@r4,r0
; CHECK-NEXT: extu.w	r0,r0
	%element = getelementptr [8 x i16], ptr %p, i32 0, i32 5
	%value = load i16, ptr %element, align 2
	%result = zext i16 %value to i32
	ret i32 %result
}
