! RUN: llvm-mc -triple=sh-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=CHECK,BE
! RUN: llvm-mc -triple=shle-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=CHECK,LE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=asm %s | FileCheck %s --check-prefix=PRINT

! CHECK: mov.l	@r4,r0
! BE-SAME: encoding: [0x60,0x42]
! LE-SAME: encoding: [0x42,0x60]
! PRINT: mov.l	@r4,r0
mov.l @r4,r0

! CHECK: mov.l	r5,@r4
! BE-SAME: encoding: [0x24,0x52]
! LE-SAME: encoding: [0x52,0x24]
! PRINT: mov.l	r5,@r4
mov.l r5,@r4

! CHECK: mov.l	@(12,r4),r0
! BE-SAME: encoding: [0x50,0x43]
! LE-SAME: encoding: [0x43,0x50]
! PRINT: mov.l	@(12,r4),r0
mov.l @(12,r4),r0

! CHECK: mov.l	r5,@(12,r4)
! BE-SAME: encoding: [0x14,0x53]
! LE-SAME: encoding: [0x53,0x14]
! PRINT: mov.l	r5,@(12,r4)
mov.l r5,@(12,r4)

! CHECK: mov.l	@(0,r15),r14
! BE-SAME: encoding: [0x5e,0xf0]
! LE-SAME: encoding: [0xf0,0x5e]
! PRINT: mov.l	@(0,r15),r14
mov.l @(0,r15),r14

! CHECK: mov.l	r14,@(60,r15)
! BE-SAME: encoding: [0x1f,0xef]
! LE-SAME: encoding: [0xef,0x1f]
! PRINT: mov.l	r14,@(60,r15)
mov.l r14,@(60,r15)
