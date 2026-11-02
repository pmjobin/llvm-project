! RUN: llvm-mc -triple=sh-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=ASM,BE
! RUN: llvm-mc -triple=shle-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=ASM,LE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=asm %s | FileCheck %s --check-prefix=PRINT
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t.le.o
! RUN: llvm-readobj --hex-dump=.text --relocations %t.be.o | FileCheck %s --check-prefix=OBJ-BE
! RUN: llvm-readobj --hex-dump=.text --relocations %t.le.o | FileCheck %s --check-prefix=OBJ-LE
! RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=DIS

! ASM: rotl	r4
! BE-SAME: encoding: [0x44,0x04]
! LE-SAME: encoding: [0x04,0x44]
! PRINT: rotl	r4
! DIS: rotl	r4
rotl r4

! ASM: rotr	r4
! BE-SAME: encoding: [0x44,0x05]
! LE-SAME: encoding: [0x05,0x44]
! PRINT: rotr	r4
! DIS: rotr	r4
rotr r4

! ASM: addv	r5,r4
! BE-SAME: encoding: [0x34,0x5f]
! LE-SAME: encoding: [0x5f,0x34]
! PRINT: addv	r5,r4
! DIS: addv	r5,r4
addv r5,r4

! ASM: subv	r5,r4
! BE-SAME: encoding: [0x34,0x5b]
! LE-SAME: encoding: [0x5b,0x34]
! PRINT: subv	r5,r4
! DIS: subv	r5,r4
subv r5,r4

! OBJ-BE: Relocations [
! OBJ-BE-NEXT: ]
! OBJ-BE: Hex dump of section '.text':
! OBJ-BE-NEXT: 0x00000000 44044405 345f345b

! OBJ-LE: Relocations [
! OBJ-LE-NEXT: ]
! OBJ-LE: Hex dump of section '.text':
! OBJ-LE-NEXT: 0x00000000 04440544 5f345b34
