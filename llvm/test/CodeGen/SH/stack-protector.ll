; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj < %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -relocation-model=pic -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --relocations --symbols %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj --relocations --symbols %t.le.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-as %s -o %t.bc
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null

declare void @consume(ptr)

define i32 @protected_buffer(i32 %value) sspreq {
; ASM-LABEL: protected_buffer:
; ASM-DAG: __stack_chk_guard
; ASM-DAG: cmp/eq
; ASM-DAG: __stack_chk_fail
	%buffer = alloca [32 x i8], align 4
	%first = getelementptr [32 x i8], ptr %buffer, i32 0, i32 0
	%byte = trunc i32 %value to i8
	store i8 %byte, ptr %first, align 1
	call void @consume(ptr %first)
	%loaded = load i8, ptr %first, align 1
	%result = zext i8 %loaded to i32
	ret i32 %result
}

; OBJECT: R_SH_GOT{{32|OFF}} __stack_chk_guard
; OBJECT: R_SH_PLT32 __stack_chk_fail
; OBJECT: Name: __stack_chk_guard
; OBJECT: Name: __stack_chk_fail
