! RUN: llvm-mc -triple=sh-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=CHECK,BE
! RUN: llvm-mc -triple=shle-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=CHECK,LE

! CHECK: nop
! BE-SAME: encoding: [0x00,0x09]
! LE-SAME: encoding: [0x09,0x00]
nop

! CHECK: rts
! BE-SAME: encoding: [0x00,0x0b]
! LE-SAME: encoding: [0x0b,0x00]
rts

! CHECK: mov	r4,r0
! BE-SAME: encoding: [0x60,0x43]
! LE-SAME: encoding: [0x43,0x60]
mov r4,r0

! CHECK: add	r5,r4
! BE-SAME: encoding: [0x34,0x5c]
! LE-SAME: encoding: [0x5c,0x34]
add r5,r4

! CHECK: mov	#-1,r3
! BE-SAME: encoding: [0xe3,0xff]
! LE-SAME: encoding: [0xff,0xe3]
mov #-1,r3

! CHECK: add	#-1,r3
! BE-SAME: encoding: [0x73,0xff]
! LE-SAME: encoding: [0xff,0x73]
add #-1,r3
