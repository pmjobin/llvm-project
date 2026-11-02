; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=OBJECT

define i32 @load_i8_byte_offset(ptr %base, i32 %byte_offset) nounwind {
; ASM-LABEL: load_i8_byte_offset:
; ASM-NEXT: mov	r4,r0
; ASM-NEXT: mov.b	@(r0,r5),r0
; ASM-NEXT: rts
; ASM-NEXT: extu.b	r0,r0
; OBJECT-LABEL: <load_i8_byte_offset>:
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} mov r4,r0
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} mov.b @(r0,r5),r0
	%address = getelementptr i8, ptr %base, i32 %byte_offset
	%value = load i8, ptr %address, align 1
	%extended = zext i8 %value to i32
	ret i32 %extended
}

define i32 @load_i16_byte_offset(ptr %base, i32 %byte_offset) nounwind {
; ASM-LABEL: load_i16_byte_offset:
; ASM-NEXT: mov	r4,r0
; ASM-NEXT: mov.w	@(r0,r5),r0
; ASM-NEXT: rts
; ASM-NEXT: extu.w	r0,r0
; OBJECT-LABEL: <load_i16_byte_offset>:
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} mov r4,r0
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} mov.w @(r0,r5),r0
	%address = getelementptr i8, ptr %base, i32 %byte_offset
	%value = load i16, ptr %address, align 2
	%extended = zext i16 %value to i32
	ret i32 %extended
}

define i32 @load_i32_byte_offset(ptr %base, i32 %byte_offset) nounwind {
; ASM-LABEL: load_i32_byte_offset:
; ASM-NEXT: mov	r4,r0
; ASM-NEXT: rts
; ASM-NEXT: mov.l	@(r0,r5),r0
; OBJECT-LABEL: <load_i32_byte_offset>:
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} mov r4,r0
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} rts
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} mov.l @(r0,r5),r0
	%address = getelementptr i8, ptr %base, i32 %byte_offset
	%value = load i32, ptr %address, align 4
	ret i32 %value
}

define void @store_i32_byte_offset(ptr %base, i32 %byte_offset, i32 %value) nounwind {
; ASM-LABEL: store_i32_byte_offset:
; ASM-NEXT: mov	r4,r0
; ASM-NEXT: rts
; ASM-NEXT: mov.l	r6,@(r0,r5)
; OBJECT-LABEL: <store_i32_byte_offset>:
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} mov r4,r0
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} rts
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} mov.l r6,@(r0,r5)
	%address = getelementptr i8, ptr %base, i32 %byte_offset
	store i32 %value, ptr %address, align 4
	ret void
}

define i32 @load_i16_element_index(ptr %base, i32 %index) nounwind {
; ASM-LABEL: load_i16_element_index:
; ASM-NEXT: mov	r5,r0
; ASM-NEXT: shll	r0
; ASM-NEXT: rts
; ASM-NEXT: mov.w	@(r0,r4),r0
; OBJECT-LABEL: <load_i16_element_index>:
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} mov r5,r0
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} shll r0
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} rts
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} mov.w @(r0,r4),r0
	%address = getelementptr i16, ptr %base, i32 %index
	%value = load i16, ptr %address, align 2
	%extended = sext i16 %value to i32
	ret i32 %extended
}

define i32 @load_i32_element_index(ptr %base, i32 %index) nounwind {
; ASM-LABEL: load_i32_element_index:
; ASM-NEXT: mov	r5,r0
; ASM-NEXT: shll2	r0
; ASM-NEXT: rts
; ASM-NEXT: mov.l	@(r0,r4),r0
; OBJECT-LABEL: <load_i32_element_index>:
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} mov r5,r0
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} shll2 r0
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} rts
; OBJECT-NEXT: {{[0-9a-f]+}}: {{.*}} mov.l @(r0,r4),r0
	%address = getelementptr i32, ptr %base, i32 %index
	%value = load i32, ptr %address, align 4
	ret i32 %value
}
