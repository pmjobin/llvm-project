! RUN: llvm-mc -triple=sh-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=ASM,BE
! RUN: llvm-mc -triple=shle-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=ASM,LE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=asm %s | FileCheck %s --check-prefix=PRINT
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t.le.o
! RUN: llvm-readobj --hex-dump=.text --relocations %t.be.o | FileCheck %s --check-prefix=OBJ-BE
! RUN: llvm-readobj --hex-dump=.text --relocations %t.le.o | FileCheck %s --check-prefix=OBJ-LE
! RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=DIS

! ASM: tas.b	@r4
! BE-SAME: encoding: [0x44,0x1b]
! LE-SAME: encoding: [0x1b,0x44]
! PRINT: tas.b	@r4
! DIS: tas.b	@r4
tas.b @r4

! ASM: movt	r4
! BE-SAME: encoding: [0x04,0x29]
! LE-SAME: encoding: [0x29,0x04]
! PRINT: movt	r4
! DIS: movt	r4
movt r4

! OBJ-BE: Relocations [
! OBJ-BE-NEXT: ]
! OBJ-BE: Hex dump of section '.text':
! OBJ-BE-NEXT: 0x00000000 441b0429

! OBJ-LE: Relocations [
! OBJ-LE-NEXT: ]
! OBJ-LE: Hex dump of section '.text':
! OBJ-LE-NEXT: 0x00000000 1b442904
