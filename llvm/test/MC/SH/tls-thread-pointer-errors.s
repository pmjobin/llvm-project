! RUN: split-file %s %t
! RUN: not llvm-mc -triple=sh-unknown-elf %t/special.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SPECIAL
! RUN: not llvm-mc -triple=sh-unknown-elf %t/destination.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=DESTINATION
! RUN: not llvm-mc -triple=sh-unknown-elf %t/incomplete.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=INCOMPLETE
! RUN: not llvm-mc -triple=sh-unknown-elf %t/index.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=INDEX
! RUN: not llvm-mc -triple=sh-unknown-elf %t/index-base.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=INDEX-BASE

! SPECIAL: error: invalid operand for instruction
! DESTINATION: error: invalid operand for instruction
! INCOMPLETE: error: expected operand
! INDEX: error: SH indexed memory requires r0 as its index
! INDEX-BASE: error: expected GPR base in indexed memory operand

!--- special.s
stc vbr,r1

!--- destination.s
stc gbr,pc

!--- incomplete.s
stc gbr,

!--- index.s
mov.l @(r1,r12),r0

!--- index-base.s
mov.l @(r0,gbr),r0
