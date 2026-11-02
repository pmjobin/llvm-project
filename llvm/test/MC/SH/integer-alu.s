! RUN: llvm-mc -triple=sh-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=CHECK,BE
! RUN: llvm-mc -triple=shle-unknown-elf -show-encoding %s | FileCheck %s --check-prefixes=CHECK,LE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=asm %s | FileCheck %s --check-prefix=PRINT

! CHECK: sub	r5,r4
! BE-SAME: encoding: [0x34,0x58]
! LE-SAME: encoding: [0x58,0x34]
! PRINT: sub	r5,r4
sub r5,r4

! CHECK: neg	r5,r4
! BE-SAME: encoding: [0x64,0x5b]
! LE-SAME: encoding: [0x5b,0x64]
! PRINT: neg	r5,r4
neg r5,r4

! CHECK: and	r5,r4
! BE-SAME: encoding: [0x24,0x59]
! LE-SAME: encoding: [0x59,0x24]
! PRINT: and	r5,r4
and r5,r4

! CHECK: or	r5,r4
! BE-SAME: encoding: [0x24,0x5b]
! LE-SAME: encoding: [0x5b,0x24]
! PRINT: or	r5,r4
or r5,r4

! CHECK: xor	r5,r4
! BE-SAME: encoding: [0x24,0x5a]
! LE-SAME: encoding: [0x5a,0x24]
! PRINT: xor	r5,r4
xor r5,r4

! CHECK: not	r5,r4
! BE-SAME: encoding: [0x64,0x57]
! LE-SAME: encoding: [0x57,0x64]
! PRINT: not	r5,r4
not r5,r4

! CHECK: tst	r5,r4
! BE-SAME: encoding: [0x24,0x58]
! LE-SAME: encoding: [0x58,0x24]
! PRINT: tst	r5,r4
tst r5,r4

! CHECK: dt	r4
! BE-SAME: encoding: [0x44,0x10]
! LE-SAME: encoding: [0x10,0x44]
! PRINT: dt	r4
dt r4

! CHECK: shll	r4
! BE-SAME: encoding: [0x44,0x00]
! LE-SAME: encoding: [0x00,0x44]
! PRINT: shll	r4
shll r4

! CHECK: shlr	r4
! BE-SAME: encoding: [0x44,0x01]
! LE-SAME: encoding: [0x01,0x44]
! PRINT: shlr	r4
shlr r4

! CHECK: shar	r4
! BE-SAME: encoding: [0x44,0x21]
! LE-SAME: encoding: [0x21,0x44]
! PRINT: shar	r4
shar r4

! CHECK: shll2	r4
! BE-SAME: encoding: [0x44,0x08]
! LE-SAME: encoding: [0x08,0x44]
! PRINT: shll2	r4
shll2 r4

! CHECK: shlr2	r4
! BE-SAME: encoding: [0x44,0x09]
! LE-SAME: encoding: [0x09,0x44]
! PRINT: shlr2	r4
shlr2 r4

! CHECK: shll8	r4
! BE-SAME: encoding: [0x44,0x18]
! LE-SAME: encoding: [0x18,0x44]
! PRINT: shll8	r4
shll8 r4

! CHECK: shlr8	r4
! BE-SAME: encoding: [0x44,0x19]
! LE-SAME: encoding: [0x19,0x44]
! PRINT: shlr8	r4
shlr8 r4

! CHECK: shll16	r4
! BE-SAME: encoding: [0x44,0x28]
! LE-SAME: encoding: [0x28,0x44]
! PRINT: shll16	r4
shll16 r4

! CHECK: shlr16	r4
! BE-SAME: encoding: [0x44,0x29]
! LE-SAME: encoding: [0x29,0x44]
! PRINT: shlr16	r4
shlr16 r4
