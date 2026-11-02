! RUN: llvm-mc -triple=sh-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=ASM,BE
! RUN: llvm-mc -triple=shle-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=ASM,LE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t.le.o
! RUN: llvm-readobj --hex-dump=.text --relocations %t.be.o | FileCheck %s --check-prefix=OBJ-BE
! RUN: llvm-readobj --hex-dump=.text --relocations %t.le.o | FileCheck %s --check-prefix=OBJ-LE
! RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=DIS

! ASM: jmp	@r0
! BE-SAME: encoding: [0x40,0x2b]
! LE-SAME: encoding: [0x2b,0x40]
jmp @r0

! ASM: jmp	@r4
! BE-SAME: encoding: [0x44,0x2b]
! LE-SAME: encoding: [0x2b,0x44]
jmp @r4

! ASM: jmp	@r15
! BE-SAME: encoding: [0x4f,0x2b]
! LE-SAME: encoding: [0x2b,0x4f]
jmp @r15

! Preserve the neighboring delayed indirect-call encoding and decoding.
! ASM: jsr	@r4
! BE-SAME: encoding: [0x44,0x0b]
! LE-SAME: encoding: [0x0b,0x44]
jsr @r4

! OBJ-BE: Relocations [
! OBJ-BE-NEXT: ]
! OBJ-BE: Hex dump of section '.text':
! OBJ-BE-NEXT: 0x00000000 402b442b 4f2b440b

! OBJ-LE: Relocations [
! OBJ-LE-NEXT: ]
! OBJ-LE: Hex dump of section '.text':
! OBJ-LE-NEXT: 0x00000000 2b402b44 2b4f0b44

! DIS: 0: {{.*}} jmp	@r0
! DIS: 2: {{.*}} jmp	@r4
! DIS: 4: {{.*}} jmp	@r15
! DIS: 6: {{.*}} jsr	@r4
