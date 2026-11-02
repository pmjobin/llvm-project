; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh2-linux-muslfdpic -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llc -mtriple=sh2eb-linux-muslfdpic -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llvm-readobj --file-headers --sections --relocations %t.le.o | FileCheck %s --check-prefixes=OBJ,LE
; RUN: llvm-readobj --file-headers --sections --relocations %t.be.o | FileCheck %s --check-prefixes=OBJ,BE

@value = internal constant i32 0, align 4
@target = internal global i32 0, align 4
@relro_pointer = internal constant ptr @target, align 4
@hidden_pointer = external hidden constant ptr

define i32 @load() {
; ASM-LABEL: load:
; ASM: mova	[[LOAD_LITERAL:.LCPI[0-9_]+]],r0
; ASM-NEXT: mov.l	[[LOAD_LITERAL]],[[LOAD_ADDRESS:r[1-9][0-9]*]]
; ASM-NEXT: add	r0,[[LOAD_ADDRESS]]
; ASM-NEXT: mov.l	@[[LOAD_ADDRESS]],r0
	%result = load volatile i32, ptr @value, align 4
	ret i32 %result
}

define void @store() {
; ASM-LABEL: store:
; ASM: mova	[[STORE_LITERAL:.LCPI[0-9_]+]],r0
; ASM-NEXT: mov.l	[[STORE_LITERAL]],[[STORE_ADDRESS:r[1-9][0-9]*]]
; ASM-NEXT: add	r0,[[STORE_ADDRESS]]
; ASM-NEXT: mov	#1,r0
; ASM-NEXT: mov.l	r0,@[[STORE_ADDRESS]]
	store volatile i32 1, ptr @value, align 4
	ret void
}

define void @store_relro_pointer() {
; ASM-LABEL: store_relro_pointer:
; ASM-DAG: mov	r12,[[GOT_BASE:r[0-9]+]]
; ASM-DAG: mov.l	[[RELRO_LITERAL:.LCPI[0-9_]+]],[[GOT_SLOT:r[0-9]+]]
; ASM-DAG: mov.l	@([[GOT_BASE]],[[GOT_SLOT]]),[[RELRO_ADDRESS:r[0-9]+]]
; ASM-DAG: mov	#0,[[ZERO:r[0-9]+]]
; ASM: mov.l	[[ZERO]],@[[RELRO_ADDRESS]]
; ASM: [[RELRO_LITERAL]]:
; ASM-NEXT: .long	relro_pointer@GOT
	store volatile ptr null, ptr @relro_pointer, align 4
	ret void
}

define ptr @load_hidden_pointer() {
; ASM-LABEL: load_hidden_pointer:
; ASM-DAG: mov	r12,[[HIDDEN_GOT_BASE:r[0-9]+]]
; ASM-DAG: mov.l	[[HIDDEN_LITERAL:.LCPI[0-9_]+]],[[HIDDEN_GOT_SLOT:r[0-9]+]]
; ASM: mov.l	@([[HIDDEN_GOT_BASE]],[[HIDDEN_GOT_SLOT]]),[[HIDDEN_ADDRESS:r[0-9]+]]
; ASM-NEXT: rts
; ASM-NEXT: mov.l	@[[HIDDEN_ADDRESS]],r0
; ASM: [[HIDDEN_LITERAL]]:
; ASM-NEXT: .long	hidden_pointer@GOT
	%result = load ptr, ptr @hidden_pointer, align 4
	ret ptr %result
}

; LE: Format: elf32-shl
; BE: Format: elf32-sh
; OBJ: Flags [ (0x8002)
; OBJ: Name: .data.rel.ro
; OBJ-COUNT-2: R_SH_REL32 value 0x0
; OBJ-NOT: R_SH_REL32 relro_pointer
; OBJ-NOT: R_SH_GOTOFF relro_pointer
; OBJ: R_SH_GOT32 relro_pointer 0x0
; OBJ-NOT: R_SH_REL32 hidden_pointer
; OBJ-NOT: R_SH_GOTOFF hidden_pointer
; OBJ: R_SH_GOT32 hidden_pointer 0x0
; OBJ: R_SH_DIR32 target 0x0
