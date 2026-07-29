; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --sections --symbols --relocations --hex-dump=.rodata %t.be.o | FileCheck %s --check-prefixes=OBJ,BE
; RUN: llvm-readobj --sections --symbols --relocations --hex-dump=.rodata %t.le.o | FileCheck %s --check-prefixes=OBJ,LE
; RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=DIS-BE
; RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=DIS-LE

define i32 @jump_table_object(i32 %x) {
entry:
	switch i32 %x, label %default [
		i32 10, label %c10
		i32 11, label %c11
		i32 12, label %c12
		i32 13, label %c13
		i32 14, label %c14
		i32 15, label %c15
	]
c10:
	ret i32 100
c11:
	ret i32 104
c12:
	ret i32 110
c13:
	ret i32 118
c14:
	ret i32 128
c15:
	ret i32 140
default:
	ret i32 -1
}

; BE: Format: elf32-sh
; LE: Format: elf32-shl

; OBJ: Name: .text
; OBJ: Flags [
; OBJ: SHF_ALLOC
; OBJ: SHF_EXECINSTR
; OBJ: AddressAlignment: 4
; OBJ: Name: .rodata
; OBJ: Flags [
; OBJ: SHF_ALLOC
; OBJ-NOT: SHF_EXECINSTR
; OBJ: Size: 24
; OBJ: AddressAlignment: 4

; The literal-island word containing the table base has one absolute
; relocation; JMP and the table-entry load have none.
; OBJ: Section {{.*}} .rela.text {
; OBJ-NEXT: 0x{{[0-9A-F]+}} R_SH_DIR32 .LJTI0_0 0x0
; OBJ-NEXT: }

; Each absolute four-byte entry has its own zero-addend DIR32 relocation.
; OBJ: Section {{.*}} .rela.rodata {
; OBJ-COUNT-6: R_SH_DIR32 .L0  0x0
; OBJ-NEXT: }
; OBJ-NOT: R_SH_REL32
; OBJ-NOT: R_SH_IND12W

; OBJ: Name: .LJTI0_0
; OBJ: Section: .rodata
; OBJ: Name: jump_table_object
; OBJ: Type: Function
; OBJ: Section: .text

; Unrelocated words are zero in both target byte orders; relocation supplies
; the complete absolute address.
; OBJ: Hex dump of section '.rodata':
; OBJ-NEXT: 0x00000000 00000000 00000000 00000000 00000000
; OBJ-NEXT: 0x00000010 00000000 00000000

; DIS-BE: {{[0-9a-f]+}}: 40 2b jmp	@r0
; DIS-BE-NEXT: {{[0-9a-f]+}}: 00 09 nop
; DIS-LE: {{[0-9a-f]+}}: 2b 40 jmp	@r0
; DIS-LE-NEXT: {{[0-9a-f]+}}: 09 00 nop
