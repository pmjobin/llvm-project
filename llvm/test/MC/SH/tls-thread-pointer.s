! RUN: llvm-mc -triple=sh-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=CHECK,BE
! RUN: llvm-mc -triple=shle-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=CHECK,LE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=asm %s | FileCheck %s --check-prefix=PRINT

! CHECK: stc	gbr,r0
! BE-SAME: encoding: [0x00,0x12]
! LE-SAME: encoding: [0x12,0x00]
! PRINT: stc	gbr,r0
stc gbr,r0

! CHECK: stc	gbr,r7
! BE-SAME: encoding: [0x07,0x12]
! LE-SAME: encoding: [0x12,0x07]
! PRINT: stc	gbr,r7
stc gbr,r7

! CHECK: stc	gbr,r15
! BE-SAME: encoding: [0x0f,0x12]
! LE-SAME: encoding: [0x12,0x0f]
! PRINT: stc	gbr,r15
stc gbr,r15

! CHECK: mov.l	@(r0,r12),r0
! BE-SAME: encoding: [0x00,0xce]
! LE-SAME: encoding: [0xce,0x00]
! PRINT: mov.l	@(r0,r12),r0
mov.l @(r0,r12),r0
