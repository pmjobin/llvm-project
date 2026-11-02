! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj -o %t.be.o %s
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj -o %t.le.o %s
! RUN: llvm-readobj --hex-dump=.data %t.be.o | FileCheck %s --check-prefix=BE
! RUN: llvm-readobj --hex-dump=.data %t.le.o | FileCheck %s --check-prefix=LE

! BE: Hex dump of section '.data':
! BE-NEXT: 0x00000000 01000100 00000100 00000000 00000100
! LE: Hex dump of section '.data':
! LE-NEXT: 0x00000000 01010001 00000001 00000000 00000000

.data
.byte .Lend - .Lstart
.short .Lend - .Lstart
.long .Lend - .Lstart
.quad .Lend - .Lstart
.Lstart:
.byte 0
.Lend:
