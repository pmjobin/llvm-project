; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM

declare i32 @read_value()
declare ptr @get_buffer()

; Both the byte loaded before the call and its return value must survive the
; R0 clobber of the halfword load. Keep the narrow access widths and extensions.
define i32 @read_fields_across_call(ptr %base) nounwind optsize {
; ISEL-LABEL: name: read_fields_across_call
; ISEL: MOVB_load_frame
; ISEL: JSR
; ISEL: MOVW_load_frame
; ASM-LABEL: read_fields_across_call:
; ASM: mov.b @(5,[[BASE:r[0-9]+]]),r0
; ASM: jsr
; ASM: extu.b r0,[[BYTE:r[0-9]+]]
; ASM: mov r0,[[SUM:r[0-9]+]]
; ASM: add [[BYTE]],[[SUM]]
; ASM: mov.w @(6,[[BASE]]),r0
; ASM: exts.w r0,r0
; ASM: add [[SUM]],r0

	%byte.address = getelementptr i8, ptr %base, i32 5
	%byte = load volatile i8, ptr %byte.address, align 1
	%unsigned = zext i8 %byte to i32
	%called = call i32 @read_value()
	%word.address = getelementptr i8, ptr %base, i32 6
	%word = load volatile i16, ptr %word.address, align 2
	%signed = sext i16 %word to i32
	%first = add i32 %unsigned, %called
	%result = add i32 %first, %signed
	ret i32 %result
}

; The returned pointer starts in R0 and remains live after the store. The
; store's early clobber must keep that base separate from its value temporary.
define ptr @store_word_with_returned_base(i16 %value) nounwind optsize {
; ISEL-LABEL: name: store_word_with_returned_base
; ISEL: JSR
; ISEL: MOVW_store_frame %{{[0-9]+}}, %{{[0-9]+}}, 6, implicit-def dead early-clobber $r0
; ASM-LABEL: store_word_with_returned_base:
; ASM: jsr
; ASM: mov r4,[[VALUE:r[0-9]+]]
; ASM: mov r0,[[POINTER:r[1-9][0-9]*]]
; ASM: mov [[VALUE]],r0
; ASM: mov.w r0,@(6,[[POINTER]])
; ASM: mov [[POINTER]],r0

	%base = call ptr @get_buffer()
	%address = getelementptr i8, ptr %base, i32 6
	store volatile i16 %value, ptr %address, align 2
	ret ptr %base
}
