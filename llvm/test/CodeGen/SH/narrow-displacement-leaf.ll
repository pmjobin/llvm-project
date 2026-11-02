; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM

declare void @consume(i32)

define i32 @leaf_byte_load(ptr %base, i32 %bias) nounwind optsize {
	%address = getelementptr i8, ptr %base, i32 5
	%value = load i8, ptr %address, align 1
	%extended = zext i8 %value to i32
	%result = add i32 %extended, %bias
	ret i32 %result
}

define void @leaf_word_store(ptr %base, i16 %value) nounwind optsize {
	%address = getelementptr i8, ptr %base, i32 6
	store i16 %value, ptr %address, align 2
	ret void
}

define void @nonleaf_byte_load(ptr %base) nounwind optsize {
	%address = getelementptr i8, ptr %base, i32 5
	%value = load i8, ptr %address, align 1
	%extended = zext i8 %value to i32
	call void @consume(i32 %extended)
	ret void
}

; ISEL-LABEL: name:            leaf_byte_load
; ISEL: %{{[0-9]+}}:gpr = MOVB_load_frame %{{[0-9]+}}, 5, implicit-def dead $r0
; ISEL-LABEL: name:            leaf_word_store
; ISEL: MOVW_store_frame %{{[0-9]+}}, %{{[0-9]+}}, 6, implicit-def dead early-clobber $r0
; ISEL-LABEL: name:            nonleaf_byte_load
; ISEL: %{{[0-9]+}}:gpr = MOVB_load_frame %{{[0-9]+}}, 5, implicit-def dead $r0
; ISEL: JSR

; ASM-LABEL: leaf_byte_load:
; ASM: mov.b @(5,r4),r0
; ASM-LABEL: leaf_word_store:
; ASM: mov.w r0,@(6,r4)
; ASM-LABEL: nonleaf_byte_load:
; ASM: mov.b @(5,r4),r0
; ASM: extu.b r0,r4
