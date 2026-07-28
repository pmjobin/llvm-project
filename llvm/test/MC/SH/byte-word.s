! RUN: llvm-mc -triple=sh-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=CHECK,BE
! RUN: llvm-mc -triple=shle-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=CHECK,LE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=asm %s | FileCheck %s --check-prefix=PRINT

! CHECK: mov.b	r5,@r4
! BE-SAME: encoding: [0x24,0x50]
! LE-SAME: encoding: [0x50,0x24]
! PRINT: mov.b	r5,@r4
mov.b r5,@r4

! CHECK: mov.w	r5,@r4
! BE-SAME: encoding: [0x24,0x51]
! LE-SAME: encoding: [0x51,0x24]
! PRINT: mov.w	r5,@r4
mov.w r5,@r4

! CHECK: mov.b	@r5,r4
! BE-SAME: encoding: [0x64,0x50]
! LE-SAME: encoding: [0x50,0x64]
! PRINT: mov.b	@r5,r4
mov.b @r5,r4

! CHECK: mov.w	@r5,r4
! BE-SAME: encoding: [0x64,0x51]
! LE-SAME: encoding: [0x51,0x64]
! PRINT: mov.w	@r5,r4
mov.w @r5,r4

! CHECK: mov.b	r0,@(3,r4)
! BE-SAME: encoding: [0x80,0x43]
! LE-SAME: encoding: [0x43,0x80]
! PRINT: mov.b	r0,@(3,r4)
mov.b r0,@(3,r4)

! CHECK: mov.w	r0,@(6,r4)
! BE-SAME: encoding: [0x81,0x43]
! LE-SAME: encoding: [0x43,0x81]
! PRINT: mov.w	r0,@(6,r4)
mov.w r0,@(6,r4)

! CHECK: mov.b	@(3,r4),r0
! BE-SAME: encoding: [0x84,0x43]
! LE-SAME: encoding: [0x43,0x84]
! PRINT: mov.b	@(3,r4),r0
mov.b @(3,r4),r0

! CHECK: mov.w	@(6,r4),r0
! BE-SAME: encoding: [0x85,0x43]
! LE-SAME: encoding: [0x43,0x85]
! PRINT: mov.w	@(6,r4),r0
mov.w @(6,r4),r0

! CHECK: mov.b	r0,@(0,r4)
! BE-SAME: encoding: [0x80,0x40]
! LE-SAME: encoding: [0x40,0x80]
mov.b r0,@(0,r4)

! CHECK: mov.b	r0,@(15,r4)
! BE-SAME: encoding: [0x80,0x4f]
! LE-SAME: encoding: [0x4f,0x80]
mov.b r0,@(15,r4)

! CHECK: mov.b	@(0,r4),r0
! BE-SAME: encoding: [0x84,0x40]
! LE-SAME: encoding: [0x40,0x84]
mov.b @(0,r4),r0

! CHECK: mov.b	@(15,r4),r0
! BE-SAME: encoding: [0x84,0x4f]
! LE-SAME: encoding: [0x4f,0x84]
mov.b @(15,r4),r0

! CHECK: mov.w	r0,@(0,r4)
! BE-SAME: encoding: [0x81,0x40]
! LE-SAME: encoding: [0x40,0x81]
mov.w r0,@(0,r4)

! CHECK: mov.w	r0,@(30,r4)
! BE-SAME: encoding: [0x81,0x4f]
! LE-SAME: encoding: [0x4f,0x81]
mov.w r0,@(30,r4)

! CHECK: mov.w	@(0,r4),r0
! BE-SAME: encoding: [0x85,0x40]
! LE-SAME: encoding: [0x40,0x85]
mov.w @(0,r4),r0

! CHECK: mov.w	@(30,r4),r0
! BE-SAME: encoding: [0x85,0x4f]
! LE-SAME: encoding: [0x4f,0x85]
mov.w @(30,r4),r0

! CHECK: extu.b	r5,r4
! BE-SAME: encoding: [0x64,0x5c]
! LE-SAME: encoding: [0x5c,0x64]
! PRINT: extu.b	r5,r4
extu.b r5,r4

! CHECK: extu.w	r5,r4
! BE-SAME: encoding: [0x64,0x5d]
! LE-SAME: encoding: [0x5d,0x64]
! PRINT: extu.w	r5,r4
extu.w r5,r4

! CHECK: exts.b	r5,r4
! BE-SAME: encoding: [0x64,0x5e]
! LE-SAME: encoding: [0x5e,0x64]
! PRINT: exts.b	r5,r4
exts.b r5,r4

! CHECK: exts.w	r5,r4
! BE-SAME: encoding: [0x64,0x5f]
! LE-SAME: encoding: [0x5f,0x64]
! PRINT: exts.w	r5,r4
exts.w r5,r4
