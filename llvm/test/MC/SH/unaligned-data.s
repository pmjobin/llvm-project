! RUN: llvm-mc -triple=sh-unknown-none-elf -filetype=asm %s | FileCheck %s --check-prefix=ASM
! RUN: llvm-mc -triple=shle-unknown-none-elf -filetype=asm %s | FileCheck %s --check-prefix=ASM
! RUN: llvm-mc -triple=sh-unknown-none-elf -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-none-elf -filetype=obj %s -o %t.le.o
! RUN: llvm-readobj --hex-dump=.data.absolute --hex-dump=.data.reloc --hex-dump=.data.subtract --hex-dump=.debug_test --relocations %t.be.o | FileCheck %s --check-prefix=OBJ-BE
! RUN: llvm-readobj --hex-dump=.data.absolute --hex-dump=.data.reloc --hex-dump=.data.subtract --hex-dump=.debug_test --relocations %t.le.o | FileCheck %s --check-prefix=OBJ-LE

	.section	.data.absolute,"aw",@progbits
absolute:
	.byte	0xa1
	.uaword	0x1234
	.ualong	0x12345678
	.uaquad	0x0123456789abcdef
	.byte	0xa2

! ASM:      absolute:
! ASM-NEXT: .byte 161
! ASM-NEXT: .uaword 4660
! ASM-NEXT: .ualong 305419896
! ASM-NEXT: .uaquad 81985529216486895
! ASM-NEXT: .byte 162

	.section	.data.reloc,"aw",@progbits
reloc:
	.byte	0xaa
	.uaword	external_symbol+3
	.ualong	external_symbol+3
	.byte	0xbb

! ASM:      reloc:
! ASM-NEXT: .byte 170
! ASM-NEXT: .uaword external_symbol+3
! ASM-NEXT: .ualong external_symbol+3
! ASM-NEXT: .byte 187

	.section	.data.subtract,"aw",@progbits
subtract:
	.byte	0xcc
	.uaword	external_symbol-5
	.ualong	external_symbol-5
	.byte	0xdd

! ASM:      subtract:
! ASM-NEXT: .byte 204
! ASM-NEXT: .uaword external_symbol-5
! ASM-NEXT: .ualong external_symbol-5
! ASM-NEXT: .byte 221

	.section	.debug_test,"",@progbits
debug:
	.byte	0xee
	.short	0x1234
	.long	0x12345678
	.quad	0x0123456789abcdef
	.byte	0xff

! ASM:      debug:
! ASM-NEXT: .byte 238
! ASM-NEXT: .uaword 4660
! ASM-NEXT: .ualong 305419896
! ASM-NEXT: .uaquad 81985529216486895
! ASM-NEXT: .byte 255

! OBJ-BE:      Section {{.*}} .rela.data.reloc {
! OBJ-BE-NEXT: 0x1 R_SH_DIR16 external_symbol 0x3
! OBJ-BE-NEXT: 0x3 R_SH_DIR32 external_symbol 0x0
! OBJ-BE:      Section {{.*}} .rela.data.subtract {
! OBJ-BE-NEXT: 0x1 R_SH_DIR16 external_symbol 0xFFFFFFFB
! OBJ-BE-NEXT: 0x3 R_SH_DIR32 external_symbol 0x0
! OBJ-BE:      Hex dump of section '.data.absolute':
! OBJ-BE-NEXT: 0x00000000 a1123412 34567801 23456789 abcdefa2
! OBJ-BE:      Hex dump of section '.data.reloc':
! OBJ-BE-NEXT: 0x00000000 aa000000 000003bb
! OBJ-BE:      Hex dump of section '.data.subtract':
! OBJ-BE-NEXT: 0x00000000 cc0000ff fffffbdd
! OBJ-BE:      Hex dump of section '.debug_test':
! OBJ-BE-NEXT: 0x00000000 ee123412 34567801 23456789 abcdefff

! OBJ-LE:      Section {{.*}} .rela.data.reloc {
! OBJ-LE-NEXT: 0x1 R_SH_DIR16 external_symbol 0x3
! OBJ-LE-NEXT: 0x3 R_SH_DIR32 external_symbol 0x0
! OBJ-LE:      Section {{.*}} .rela.data.subtract {
! OBJ-LE-NEXT: 0x1 R_SH_DIR16 external_symbol 0xFFFFFFFB
! OBJ-LE-NEXT: 0x3 R_SH_DIR32 external_symbol 0x0
! OBJ-LE:      Hex dump of section '.data.absolute':
! OBJ-LE-NEXT: 0x00000000 a1341278 563412ef cdab8967 452301a2
! OBJ-LE:      Hex dump of section '.data.reloc':
! OBJ-LE-NEXT: 0x00000000 aa000003 000000bb
! OBJ-LE:      Hex dump of section '.data.subtract':
! OBJ-LE-NEXT: 0x00000000 cc0000fb ffffffdd
! OBJ-LE:      Hex dump of section '.debug_test':
! OBJ-LE-NEXT: 0x00000000 ee341278 563412ef cdab8967 452301ff
