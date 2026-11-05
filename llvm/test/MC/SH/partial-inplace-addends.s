! RUN: llvm-mc -triple=shle-unknown-none-elf -mcpu=sh4 -filetype=obj %s -o %t.le.o
! RUN: llvm-mc -triple=sh-unknown-none-elf -mcpu=sh4 -filetype=obj %s -o %t.be.o
! RUN: llvm-readobj -r -x .text %t.le.o | FileCheck %s --check-prefixes=RELOC,LE
! RUN: llvm-readobj -r -x .text %t.be.o | FileCheck %s --check-prefixes=RELOC,BE

! RELA does not imply an explicit addend for these SH relocations. GNU SH
! linkers ignore r_addend for DIR8WPN, DIR8WPL and DIR8WPZ. Preserve the
! scaled addend in the instruction field, without including the PC+4 bias.
bt external+2
bf external-2
mov.l external+4,r0
mov.w external+2,r1
mova external+8,r0
bsr external+2
nop

! RELOC:      0x0 R_SH_DIR8WPN external 0x0
! RELOC-NEXT: 0x2 R_SH_DIR8WPN external 0x0
! RELOC-NEXT: 0x4 R_SH_DIR8WPL external 0x0
! RELOC-NEXT: 0x6 R_SH_DIR8WPZ external 0x0
! RELOC-NEXT: 0x8 R_SH_DIR8WPL external 0x0
! RELOC-NEXT: 0xA R_SH_IND12W external 0xFFFFFFFE
! LE: 0x00000000 0189ff8b 01d00191 02c700b0 0900
! BE: 0x00000000 89018bff d0019101 c702b000 0009
