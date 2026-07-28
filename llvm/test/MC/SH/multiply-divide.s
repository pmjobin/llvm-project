! RUN: llvm-mc -triple=sh-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=ASM,BE
! RUN: llvm-mc -triple=shle-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=ASM,LE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=asm %s | FileCheck %s --check-prefix=PRINT
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t.le.o
! RUN: llvm-readobj --hex-dump=.text --relocations %t.be.o | FileCheck %s --check-prefix=OBJ-BE
! RUN: llvm-readobj --hex-dump=.text --relocations %t.le.o | FileCheck %s --check-prefix=OBJ-LE
! RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=DIS

! ASM: mul.l	r5,r4
! BE-SAME: encoding: [0x04,0x57]
! LE-SAME: encoding: [0x57,0x04]
! PRINT: mul.l	r5,r4
! DIS: mul.l	r5,r4
mul.l r5,r4

! ASM: sts	macl,r4
! BE-SAME: encoding: [0x04,0x1a]
! LE-SAME: encoding: [0x1a,0x04]
! PRINT: sts	macl,r4
! DIS: sts	macl,r4
sts macl,r4

! ASM: div0u
! BE-SAME: encoding: [0x00,0x19]
! LE-SAME: encoding: [0x19,0x00]
! PRINT: div0u
! DIS: div0u
div0u

! ASM: div0s	r5,r4
! BE-SAME: encoding: [0x24,0x57]
! LE-SAME: encoding: [0x57,0x24]
! PRINT: div0s	r5,r4
! DIS: div0s	r5,r4
div0s r5,r4

! ASM: div1	r5,r4
! BE-SAME: encoding: [0x34,0x54]
! LE-SAME: encoding: [0x54,0x34]
! PRINT: div1	r5,r4
! DIS: div1	r5,r4
div1 r5,r4

! ASM: rotcl	r4
! BE-SAME: encoding: [0x44,0x24]
! LE-SAME: encoding: [0x24,0x44]
! PRINT: rotcl	r4
! DIS: rotcl	r4
rotcl r4

! ASM: addc	r5,r4
! BE-SAME: encoding: [0x34,0x5e]
! LE-SAME: encoding: [0x5e,0x34]
! PRINT: addc	r5,r4
! DIS: addc	r5,r4
addc r5,r4

! ASM: subc	r5,r4
! BE-SAME: encoding: [0x34,0x5a]
! LE-SAME: encoding: [0x5a,0x34]
! PRINT: subc	r5,r4
! DIS: subc	r5,r4
subc r5,r4

! OBJ-BE: Relocations [
! OBJ-BE-NEXT: ]
! OBJ-BE: Hex dump of section '.text':
! OBJ-BE-NEXT: 0x00000000 0457041a 00192457 34544424 345e345a

! OBJ-LE: Relocations [
! OBJ-LE-NEXT: ]
! OBJ-LE: Hex dump of section '.text':
! OBJ-LE-NEXT: 0x00000000 57041a04 19005724 54342444 5e345a34
