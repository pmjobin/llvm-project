! RUN: llvm-mc -triple=sh-unknown-elf -filetype=asm %s | FileCheck %s

! CHECK: mov	#-128,r0
mov #-128,r0
! CHECK: mov	#-1,r1
mov #-1,r1
! CHECK: mov	#0,r2
mov #0,r2
! CHECK: mov	#127,r3
mov #127,r3

! CHECK: add	#-128,r4
add #-128,r4
! CHECK: add	#-1,r5
add #-1,r5
! CHECK: add	#0,r6
add #0,r6
! CHECK: add	#127,r7
add #127,r7
