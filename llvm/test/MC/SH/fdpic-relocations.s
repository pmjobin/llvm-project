! RUN: llvm-mc -triple=sh2eb-linux-muslfdpic -mcpu=sh2a -filetype=asm %s | FileCheck %s --check-prefix=PRINT
! RUN: llvm-mc -triple=sh2eb-linux-muslfdpic -mcpu=sh2a -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=sh2-linux-muslfdpic -mcpu=sh2a -filetype=obj %s -o %t.le.o
! RUN: llvm-readobj -h -r --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=HEADER,RELOC,BE
! RUN: llvm-readobj -h -r --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=HEADER,RELOC,LE

	.text
	movi20 #foo@GOT,r0
	movi20 #foo@GOTOFF,r1
	movi20 #foo@GOTFUNCDESC,r2
	movi20 #foo@GOTOFFFUNCDESC,r3
	movi20 #foo@GOTOFF-4,r4
	rts
	nop
	.long foo@GOTFUNCDESC
	.long foo@GOTOFFFUNCDESC
	.long foo@FUNCDESC

! PRINT: movi20 #foo@GOT,r0
! PRINT: movi20 #foo@GOTOFF,r1
! PRINT: movi20 #foo@GOTFUNCDESC,r2
! PRINT: movi20 #foo@GOTOFFFUNCDESC,r3
! PRINT: movi20 #foo@GOTOFF-4,r4
! PRINT: .long foo@GOTFUNCDESC
! PRINT: .long foo@GOTOFFFUNCDESC
! PRINT: .long foo@FUNCDESC

! HEADER: Flags [ (0x800D)
! HEADER: 0x8000

! RELOC: Section {{.*}} .rela.text {
! RELOC-NEXT: 0x0 R_SH_GOT20 foo 0x0
! RELOC-NEXT: 0x4 R_SH_GOTOFF20 foo 0x0
! RELOC-NEXT: 0x8 R_SH_GOTFUNCDESC20 foo 0x0
! RELOC-NEXT: 0xC R_SH_GOTOFFFUNCDESC20 foo 0x0
! RELOC-NEXT: 0x10 R_SH_GOTOFF20 foo 0xFFFFFFFC
! RELOC-NEXT: 0x18 R_SH_GOTFUNCDESC foo 0x0
! RELOC-NEXT: 0x1C R_SH_GOTOFFFUNCDESC foo 0x0
! RELOC-NEXT: 0x20 R_SH_FUNCDESC foo 0x0
! RELOC-NEXT: }

! BE: Hex dump of section '.text':
! BE-NEXT: 0x00000000 00000000 01000000 02000000 03000000
! BE-NEXT: 0x00000010 04000000 000b0009 00000000 00000000
! BE-NEXT: 0x00000020 00000000
! LE: Hex dump of section '.text':
! LE-NEXT: 0x00000000 00000000 00010000 00020000 00030000
! LE-NEXT: 0x00000010 00040000 0b000900 00000000 00000000
! LE-NEXT: 0x00000020 00000000
