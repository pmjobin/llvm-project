! RUN: llvm-mc -triple=sh-unknown-elf -filetype=asm %s | FileCheck %s --check-prefix=PRINT
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t.le.o
! RUN: llvm-readobj -r --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=RELOC,BE
! RUN: llvm-readobj -r --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=RELOC,LE
! RUN: llvm-readelf -r %t.be.o | FileCheck %s --check-prefix=READELF
! RUN: llvm-readelf -r %t.le.o | FileCheck %s --check-prefix=READELF

.text
.long local@GOTOFF+20
.long local@GOTOFF-20
.long external@GOT
.long function@PLT+36
.long function@PLT-36
.long got_anchor@GOTPC+12
.long got_anchor@GOTPC-12
.long _GLOBAL_OFFSET_TABLE_

.data
local:
.long 0

! PRINT: .long	local@GOTOFF+20
! PRINT: .long	local@GOTOFF-20
! PRINT: .long	external@GOT
! PRINT: .long	function@PLT+36
! PRINT: .long	function@PLT-36
! PRINT: .long	got_anchor@GOTPC+12
! PRINT: .long	got_anchor@GOTPC-12
! PRINT: .long	_GLOBAL_OFFSET_TABLE_

! RELOC: Section {{.*}} .rela.text {
! RELOC-NEXT: 0x0 R_SH_GOTOFF local 0x0
! RELOC-NEXT: 0x4 R_SH_GOTOFF local 0x0
! RELOC-NEXT: 0x8 R_SH_GOT32 external 0x0
! RELOC-NEXT: 0xC R_SH_PLT32 function 0x0
! RELOC-NEXT: 0x10 R_SH_PLT32 function 0x0
! RELOC-NEXT: 0x14 R_SH_GOTPC got_anchor 0x0
! RELOC-NEXT: 0x18 R_SH_GOTPC got_anchor 0x0
! RELOC-NEXT: 0x1C R_SH_GOTPC _GLOBAL_OFFSET_TABLE_ 0x0
! RELOC-NEXT: }

! BE: Hex dump of section '.text':
! BE-NEXT: 0x00000000 00000014 ffffffec 00000000 00000024
! BE-NEXT: 0x00000010 ffffffdc 0000000c fffffff4 00000000
! LE: Hex dump of section '.text':
! LE-NEXT: 0x00000000 14000000 ecffffff 00000000 24000000
! LE-NEXT: 0x00000010 dcffffff 0c000000 f4ffffff 00000000

! READELF: R_SH_GOTOFF
! READELF: R_SH_GOT32
! READELF: R_SH_PLT32
! READELF: R_SH_GOTPC
