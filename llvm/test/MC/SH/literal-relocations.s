! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t.le.o
! RUN: llvm-readobj -r --hex-dump=.text --hex-dump=.data %t.be.o | FileCheck %s --check-prefixes=RELOC,BE
! RUN: llvm-readobj -r --hex-dump=.text --hex-dump=.data %t.le.o | FileCheck %s --check-prefixes=RELOC,LE
! RUN: llvm-readelf -r %t.be.o | FileCheck %s --check-prefix=READELF
! RUN: llvm-readelf -r %t.le.o | FileCheck %s --check-prefix=READELF

.text
mov.l .Lpool,r0
rts
nop
.p2align 2
.Lpool:
.long data_symbol+4
.long external_global-4
.long defined_function
.long external_function

.globl defined_function
.type defined_function,@function
defined_function:
rts
nop

.data
.globl data_symbol
.type data_symbol,@object
data_symbol:
.long 0x12345678
.long data_symbol
.long external_function+4

.bss
.globl bss_symbol
.type bss_symbol,@object
bss_symbol:
.space 4

! RELOC: Section {{.*}} .rela.text {
! RELOC-NEXT: 0x8 R_SH_DIR32 data_symbol
! RELOC-NEXT: 0xC R_SH_DIR32 external_global
! RELOC-NEXT: 0x10 R_SH_DIR32 defined_function
! RELOC-NEXT: 0x14 R_SH_DIR32 external_function
! RELOC-NEXT: }
! RELOC: Section {{.*}} .rela.data {
! RELOC-NEXT: 0x4 R_SH_DIR32 data_symbol
! RELOC-NEXT: 0x8 R_SH_DIR32 external_function
! RELOC-NEXT: }
! RELOC-NOT: R_SH_NONE
! BE: Hex dump of section '.text':
! BE-NEXT: 0x00000000 d001000b 00090009 00000004 fffffffc
! BE-NEXT: 0x00000010 00000000 00000000 000b0009
! LE: Hex dump of section '.text':
! LE-NEXT: 0x00000000 01d00b00 09000900 04000000 fcffffff
! LE-NEXT: 0x00000010 00000000 00000000 0b000900
! BE: Hex dump of section '.data':
! BE-NEXT: 0x00000000 12345678 00000000 00000004
! LE: Hex dump of section '.data':
! LE-NEXT: 0x00000000 78563412 00000000 04000000

! READELF: Relocation section '.rela.text'
! READELF: R_SH_DIR32
! READELF: Relocation section '.rela.data'
! READELF: R_SH_DIR32
! READELF-NOT: R_SH_NONE
