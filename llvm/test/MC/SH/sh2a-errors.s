! RUN: not llvm-mc -triple=sh-unknown-elf -mcpu=sh2 -show-encoding %s 2>&1 | FileCheck %s --check-prefix=SH2
! RUN: not llvm-mc -triple=sh-unknown-elf -mcpu=sh2a -mattr=-single-fpu,-double-fpu -show-encoding %s 2>&1 | FileCheck %s --check-prefix=NOFPU

movi20 #4,r4
! SH2: error:

fmov.s fr2,@(2048,r4)
! NOFPU: error: instruction requires an unavailable feature
