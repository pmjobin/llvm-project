; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --sections --symbols --relocations --hex-dump=.data --hex-dump=.rodata %t.be.o | FileCheck %s --check-prefixes=OBJ,BE
; RUN: llvm-readobj --sections --symbols --relocations --hex-dump=.data --hex-dump=.rodata %t.le.o | FileCheck %s --check-prefixes=OBJ,LE

@label_pointer = global ptr blockaddress(@dispatch_global, %case_a), align 4
@label_array = constant [3 x ptr] [
	ptr blockaddress(@dispatch_global, %case_a),
	ptr blockaddress(@dispatch_global, %case_b),
	ptr blockaddress(@dispatch_global, %case_a)
], align 4

define i32 @dispatch_global(ptr %address) {
entry:
	indirectbr ptr %address, [label %case_a, label %case_b]
case_a:
	ret i32 17
case_b:
	ret i32 43
}

; ASM-LABEL: dispatch_global:
; ASM: jmp	@r4
; ASM-NEXT: nop
; ASM: [[CASE_A:.Ltmp[0-9]+]]:
; ASM: [[CASE_B:.Ltmp[0-9]+]]:
; ASM: label_pointer:
; ASM-NEXT: .long	[[CASE_A]]
; ASM: label_array:
; ASM-NEXT: .long	[[CASE_A]]
; ASM-NEXT: .long	[[CASE_B]]
; ASM-NEXT: .long	[[CASE_A]]

; BE: Format: elf32-sh
; LE: Format: elf32-shl

; OBJ: Name: .data
; OBJ: Size: 4
; OBJ: AddressAlignment: 4
; OBJ: Name: .rodata
; OBJ: Size: 12
; OBJ: AddressAlignment: 4

; OBJ: Section {{.*}} .rela.data {
; OBJ-NEXT: 0x0 R_SH_DIR32 .Ltmp0 0x0
; OBJ-NEXT: }
; OBJ: Section {{.*}} .rela.rodata {
; OBJ-NEXT: 0x0 R_SH_DIR32 .Ltmp0 0x0
; OBJ-NEXT: 0x4 R_SH_DIR32 .Ltmp1 0x0
; OBJ-NEXT: 0x8 R_SH_DIR32 .Ltmp0 0x0
; OBJ-NEXT: }

; OBJ: Name: .Ltmp0
; OBJ: Value: 0x4
; OBJ: Section: .text
; OBJ: Name: .Ltmp1
; OBJ: Value: 0xA
; OBJ: Section: .text
; OBJ: Name: label_pointer
; OBJ: Size: 4
; OBJ: Section: .data
; OBJ: Name: label_array
; OBJ: Size: 12
; OBJ: Section: .rodata

; OBJ: Hex dump of section '.data':
; OBJ-NEXT: 0x00000000 00000000
; OBJ: Hex dump of section '.rodata':
; OBJ-NEXT: 0x00000000 00000000 00000000 00000000
