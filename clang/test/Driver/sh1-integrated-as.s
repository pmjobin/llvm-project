! RUN: %clang -target sh-unknown-none-elf -m1 -c %s -o %t.be.o
! RUN: %clang -target shle-unknown-none-elf -mcpu=sh1 -c %s -o %t.le.o
! RUN: llvm-readobj --file-headers --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
! RUN: llvm-readobj --file-headers --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=COMMON,LE

! COMMON: Flags [ (0x1)
! BE: Hex dump of section '.text':
! BE-NEXT: 0x00000000 6213000b 0009
! LE: Hex dump of section '.text':
! LE-NEXT: 0x00000000 13620b00 0900

	.text
	.globl sh1_integrated_assembler
	.type sh1_integrated_assembler,@function
sh1_integrated_assembler:
	mov	r1,r2
	rts
	nop
	.size sh1_integrated_assembler,.-sh1_integrated_assembler
