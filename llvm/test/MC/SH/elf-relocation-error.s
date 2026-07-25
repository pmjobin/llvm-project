! RUN: rm -f %t.be.o %t.le.o
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj -o %t.be.o %s 2>&1 | FileCheck %s --check-prefix=BE
! RUN: not test -e %t.be.o
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj -o %t.le.o %s 2>&1 | FileCheck %s --check-prefix=LE
! RUN: not test -e %t.le.o

! BE: error: SH relocations are not yet supported
! BE-NOT: assertion
! BE-NOT: LLVM ERROR
! LE: error: SH relocations are not yet supported
! LE-NOT: assertion
! LE-NOT: LLVM ERROR

.long external_symbol
