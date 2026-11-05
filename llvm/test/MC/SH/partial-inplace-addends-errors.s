! RUN: not llvm-mc -triple=shle-unknown-none-elf -mcpu=sh4 -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s
bt external+1
! CHECK: error: SH relocation addend must be a multiple of 2
bt external+256
! CHECK: error: SH relocation addend is out of range
bf external-258
! CHECK: error: SH relocation addend is out of range
mov.l external+2,r0
! CHECK: error: SH relocation addend must be a multiple of 4
mov.l external-4,r0
! CHECK: error: SH relocation addend is out of range
mov.w external+512,r0
! CHECK: error: SH relocation addend is out of range
