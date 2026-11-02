! RUN: llvm-mc -triple=sh-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=ASM,BE
! RUN: llvm-mc -triple=shle-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=ASM,LE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t.le.o
! RUN: llvm-objdump -d %t.be.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -d %t.le.o | FileCheck %s --check-prefix=DIS

! ASM: lds	r2,pr
! BE-SAME: encoding: [0x42,0x2a]
! LE-SAME: encoding: [0x2a,0x42]
! DIS: lds	r2,pr
lds r2,pr
