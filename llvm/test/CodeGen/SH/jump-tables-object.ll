; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --sections --symbols --relocations --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=OBJ,BE --implicit-check-not=.rodata --implicit-check-not=.rela.text --implicit-check-not=.LJTI
; RUN: llvm-readobj --sections --symbols --relocations --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=OBJ,LE --implicit-check-not=.rodata --implicit-check-not=.rela.text --implicit-check-not=.LJTI
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
; OBJ: Size: 64
; OBJ: AddressAlignment: 4

; The inline target-relative entries need no relocations. The GNU SH FDE
; encoding remains PC-relative in static and PIC objects.
; OBJ: Section {{.*}} .rela.eh_frame {
; OBJ-NEXT: 0x{{[0-9A-F]+}} R_SH_REL32 .L0 0x0
; OBJ-NEXT: }
; OBJ-NOT: R_SH_DIR32
; OBJ-NOT: R_SH_IND12W

; OBJ: Name: jump_table_object
; OBJ: Type: Function
; OBJ: Section: .text

; The six signed 16-bit displacements use the target byte order.
; BE: Hex dump of section '.text':
; BE: 0x00000010 00090009 000e0020 0018001c 00120024
; LE: Hex dump of section '.text':
; LE: 0x00000010 09000900 0e002000 18001c00 12002400

; DIS-BE: {{[0-9a-f]+}}: 00 23 braf	r0
; DIS-BE-NEXT: {{[0-9a-f]+}}: 00 09 nop
; DIS-LE: {{[0-9a-f]+}}: 23 00 braf	r0
; DIS-LE-NEXT: {{[0-9a-f]+}}: 09 00 nop
