; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=O0
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=O2
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=O0
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=O2

%fields = type { i8, i8, i16, i32 }

define i32 @stack_struct(i32 %byte0, i32 %byte1, i32 %word, i32 %long) {
; O0-LABEL: stack_struct:
; O0: add	#-{{[0-9]+}},r15
; O0: mov.b
; O0: mov.b
; O0: mov.w
; O0: mov.l
; O0: mov.b
; O0: extu.b
; O0: mov.b
; O0: extu.b
; O0: mov.w
; O0: extu.w
; O0: mov.l
; O0: add	#{{[0-9]+}},r15
; O2-LABEL: stack_struct:
; O2-NEXT: add	#-8,r15
; O2-NEXT: mov.b	r4,@r15
; O2: mov.b	r0,@(1,r15)
; O2: mov.w	r0,@(2,r15)
; O2: mov.l	r7,@(4,r15)
; O2: mov.b	@r15,{{r[0-9]+}}
; O2: mov.b	@(1,r15),r0
; O2: mov.w	@(2,r15),r0
; O2: mov.l	@(4,r15),r0
; O2: add	#8,r15
	%object = alloca %fields, align 4
	%field0 = getelementptr %fields, ptr %object, i32 0, i32 0
	%field1 = getelementptr %fields, ptr %object, i32 0, i32 1
	%field2 = getelementptr %fields, ptr %object, i32 0, i32 2
	%field3 = getelementptr %fields, ptr %object, i32 0, i32 3
	%narrow0 = trunc i32 %byte0 to i8
	%narrow1 = trunc i32 %byte1 to i8
	%narrow2 = trunc i32 %word to i16
	store volatile i8 %narrow0, ptr %field0, align 1
	store volatile i8 %narrow1, ptr %field1, align 1
	store volatile i16 %narrow2, ptr %field2, align 2
	store volatile i32 %long, ptr %field3, align 4
	%load0 = load volatile i8, ptr %field0, align 1
	%load1 = load volatile i8, ptr %field1, align 1
	%load2 = load volatile i16, ptr %field2, align 2
	%load3 = load volatile i32, ptr %field3, align 4
	%extend0 = zext i8 %load0 to i32
	%extend1 = zext i8 %load1 to i32
	%extend2 = zext i16 %load2 to i32
	%sum0 = add i32 %extend0, %extend1
	%sum1 = add i32 %extend2, %load3
	%result = add i32 %sum0, %sum1
	ret i32 %result
}

define i32 @stack_arrays(i32 %byte, i32 %word) {
; O0-LABEL: stack_arrays:
; O0: add	#-{{[0-9]+}},r15
; O0: mov.b
; O0: mov.w
; O0: mov.b
; O0: extu.b
; O0: mov.w
; O0: extu.w
; O0: add	#{{[0-9]+}},r15
; O2-LABEL: stack_arrays:
; O2-NEXT: add	#-8,r15
; O2: mov.b	r0,@(6,r15)
; O2: mov.w	r0,@(2,r15)
; O2: mov.b	@(6,r15),r0
; O2: mov.w	@(2,r15),r0
; O2: add	#8,r15
	%bytes = alloca [3 x i8], align 1
	%words = alloca [2 x i16], align 2
	%byte_element = getelementptr [3 x i8], ptr %bytes, i32 0, i32 1
	%word_element = getelementptr [2 x i16], ptr %words, i32 0, i32 1
	%narrow_byte = trunc i32 %byte to i8
	%narrow_word = trunc i32 %word to i16
	store volatile i8 %narrow_byte, ptr %byte_element, align 1
	store volatile i16 %narrow_word, ptr %word_element, align 2
	%load_byte = load volatile i8, ptr %byte_element, align 1
	%load_word = load volatile i16, ptr %word_element, align 2
	%extend_byte = zext i8 %load_byte to i32
	%extend_word = zext i16 %load_word to i32
	%result = add i32 %extend_byte, %extend_word
	ret i32 %result
}
