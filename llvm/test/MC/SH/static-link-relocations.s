! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t.le.o
! RUN: llvm-readobj -r --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=RELOC,BE
! RUN: llvm-readobj -r --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=RELOC,LE
! RUN: llvm-readelf -r %t.be.o | FileCheck %s --check-prefix=READELF
! RUN: llvm-readelf -r %t.le.o | FileCheck %s --check-prefix=READELF
! RUN: llvm-objdump -dr %t.be.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -dr %t.le.o | FileCheck %s --check-prefix=DIS

.text
bsr .Llocal
nop
.Llocal:
nop
bsr external
nop
bsr data_target
nop
bsr external+4
nop
bsr external-4
nop
.long external - .
.long external + 4 - .
.long external - 4 - .
.Lrelative_base:
nop
.long external - .Lrelative_base

.data
data_target:
.long 0

! RELOC: Section {{.*}} .rela.text {
! RELOC-NEXT: 0x6 R_SH_IND12W external 0xFFFFFFFC
! RELOC-NEXT: 0xA R_SH_IND12W .data 0xFFFFFFFC
! RELOC-NEXT: 0xE R_SH_IND12W external 0x0
! RELOC-NEXT: 0x12 R_SH_IND12W external 0xFFFFFFF8
! RELOC-NEXT: 0x16 R_SH_REL32 external 0x0
! RELOC-NEXT: 0x1A R_SH_REL32 external 0x0
! RELOC-NEXT: 0x1E R_SH_REL32 external 0x0
! RELOC-NEXT: 0x24 R_SH_REL32 external 0x0
! RELOC-NEXT: }
! RELOC-NOT: R_SH_NONE

! BE: Hex dump of section '.text':
! BE-NEXT: 0x00000000 b0000009 0009b000 0009b000 0009b000
! BE-NEXT: 0x00000010 0009b000 00090000 00000000 0004ffff
! BE-NEXT: 0x00000020 fffc0009 00000002
! LE: Hex dump of section '.text':
! LE-NEXT: 0x00000000 00b00900 090000b0 090000b0 090000b0
! LE-NEXT: 0x00000010 090000b0 09000000 00000400 0000fcff
! LE-NEXT: 0x00000020 ffff0900 02000000

! READELF: R_SH_IND12W
! READELF: R_SH_REL32
! READELF-NOT: R_SH_NONE

! DIS: bsr
! DIS-NEXT: {{.*}}nop
! DIS: R_SH_IND12W
! DIS: R_SH_REL32
