! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4al-dsp -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4al-dsp -filetype=obj %s -o %t.le.o
! RUN: llvm-readobj --relocations --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=RELOC,BE
! RUN: llvm-readobj --relocations --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=RELOC,LE

! RELOC: 0x0 R_SH_LOOP_START - 0xA
! RELOC-NEXT: 0x0 R_SH_LOOP_END - 0x14
! RELOC-NEXT: 0x2 R_SH_LOOP_START - 0xA
! RELOC-NEXT: 0x2 R_SH_LOOP_END - 0x14
! RELOC: 0x6 R_SH_LOOP_START loop_start 0x0
! RELOC-NEXT: 0x6 R_SH_LOOP_END loop_end 0x0
! RELOC-NEXT: 0x8 R_SH_LOOP_START loop_start 0x0
! RELOC-NEXT: 0x8 R_SH_LOOP_END loop_end 0x0
! BE: 0x00000000 8c8c8e8e 44148c8c 8e8e8202 00090009
! LE: 0x00000000 8c8c8e8e 14448c8c 8e8e0282 09000900

	repeat 10 20 r4
	repeat loop_start loop_end #2
loop_start:
	nop
loop_end:
	nop
