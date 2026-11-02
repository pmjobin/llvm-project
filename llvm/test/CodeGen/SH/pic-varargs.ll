; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj -r %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj -r %t.le.o | FileCheck %s --check-prefix=OBJECT

@external_data = external global i32

declare i32 @external_variadic(i32, ...)

define i32 @call_variadic() {
entry:
	%result = call i32 (i32, ...) @external_variadic(i32 1, i32 2, i32 3, i32 4, i32 5, i32 6)
	ret i32 %result
}

define i32 @variadic_definition(i32 %fixed, ...) {
entry:
	%value = load volatile i32, ptr @external_data, align 4
	%result = add i32 %value, %fixed
	ret i32 %result
}

; ASM-LABEL: call_variadic:
; ASM: .cfi_offset r12
; ASM: mov	#1,r4
; ASM: mov	#2,r5
; ASM: mov	#3,r6
; ASM: mov	#4,r7
; ASM: mova	[[PLT:.LCPI[0-9_]+]],r0
; ASM-NEXT: mov.l	[[PLT]],{{r[0-9]+}}
; ASM-NEXT: add	r0,{{r[0-9]+}}
; ASM-NEXT: jsr
; ASM-NEXT: nop
; ASM: add	#8,r15
; ASM: .long	external_variadic@PLT

; ASM-LABEL: variadic_definition:
; ASM-DAG: mov.l	r5,
; ASM-DAG: mov.l	r6,
; ASM-DAG: mov.l	r7,
; ASM: mov.l	{{[^,]+}},r0
; ASM-NEXT: add	r12,r0
; ASM-NEXT: mov.l	@r0,r0
; ASM-NEXT: mov.l	@r0,r0
; ASM: .long	external_data@GOT

; OBJECT: R_SH_GOTPC _GLOBAL_OFFSET_TABLE_
; OBJECT-DAG: R_SH_PLT32 external_variadic
; OBJECT-DAG: R_SH_GOT32 external_data
; OBJECT-NOT: R_SH_DIR32
