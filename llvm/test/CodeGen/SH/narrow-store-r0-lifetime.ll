; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh3 -O2 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O2 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s

; The shift helper returns in R0, but the following indexed store needs a
; different R0 value. Bind the narrow store to the physical register with
; chain and glue, instead of imposing R0Reg on its virtual input. Otherwise
; scheduling the index copy first can make register allocation impossible.
define i32 @narrow_return_store(ptr %base, ptr %output, i32 %count) {
; CHECK-LABEL: name: narrow_return_store
; CHECK: JSR
; CHECK: MOVW_store_disp $r0,
; CHECK: MOVB_store_indexed
	%shifted = lshr i32 1, %count
	%value = trunc i32 %shifted to i16
	%field = getelementptr i8, ptr %base, i32 516
	store i16 %value, ptr %field, align 4
	%address = getelementptr i8, ptr %output, i32 %count
	store i8 0, ptr %address, align 1
	%previous = getelementptr i8, ptr %base, i32 512
	%result = load i32, ptr %previous, align 4
	ret i32 %result
}
