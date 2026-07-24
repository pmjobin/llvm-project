! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj -o %t %s 2>&1 | FileCheck %s

! CHECK: error: SH object emission is not supported
nop
