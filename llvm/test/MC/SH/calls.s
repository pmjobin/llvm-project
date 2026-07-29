! RUN: llvm-mc -triple=sh-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=ASM,BE
! RUN: llvm-mc -triple=shle-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=ASM,LE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t.le.o
! RUN: llvm-readobj --hex-dump=.text --relocations %t.be.o | FileCheck %s --check-prefix=OBJ-BE
! RUN: llvm-readobj --hex-dump=.text --relocations %t.le.o | FileCheck %s --check-prefix=OBJ-LE
! RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=DIS

! P = 0, S = 4: (4 - (0 + 4)) / 2 = 0.
! ASM: bsr	.Lzero
! ASM-NEXT: fixup A - offset: 0, value: .Lzero, kind: fixup_SH_BSR12_2
bsr .Lzero
nop
.Lzero:
nop

! P = 6, S = 12: (12 - (6 + 4)) / 2 = 1.
bsr .Lforward
nop
nop
.Lforward:
nop

! P = 14, S = 4: (4 - (14 + 4)) / 2 = -7.
bsr .Lzero
nop

! ASM: jsr	@r4
! BE-SAME: encoding: [0x44,0x0b]
! LE-SAME: encoding: [0x0b,0x44]
jsr @r4

! ASM: sts.l	pr,@-r15
! BE-SAME: encoding: [0x4f,0x22]
! LE-SAME: encoding: [0x22,0x4f]
sts.l pr,@-r15

! ASM: lds.l	@r15+,pr
! BE-SAME: encoding: [0x4f,0x26]
! LE-SAME: encoding: [0x26,0x4f]
lds.l @r15+,pr

! OBJ-BE: Relocations [
! OBJ-BE-NEXT: ]
! OBJ-BE: Hex dump of section '.text':
! OBJ-BE-NEXT: 0x00000000 b0000009 0009b001 00090009 0009bff9
! OBJ-BE-NEXT: 0x00000010 0009440b 4f224f26

! OBJ-LE: Relocations [
! OBJ-LE-NEXT: ]
! OBJ-LE: Hex dump of section '.text':
! OBJ-LE-NEXT: 0x00000000 00b00900 090001b0 09000900 0900f9bf
! OBJ-LE-NEXT: 0x00000010 09000b44 224f264f

! DIS: 0: {{.*}} bsr	0x4
! DIS: 6: {{.*}} bsr	0xc
! DIS: e: {{.*}} bsr	0x4
! DIS: 12: {{.*}} jsr	@r4
! DIS: 14: {{.*}} sts.l	pr,@-r15
! DIS: 16: {{.*}} lds.l	@r15+,pr
