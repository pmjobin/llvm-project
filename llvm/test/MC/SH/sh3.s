! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh3 -show-encoding %s | FileCheck %s --check-prefixes=CHECK,BE
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh3 -show-encoding %s | FileCheck %s --check-prefixes=CHECK,LE
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh3 -filetype=asm %s | FileCheck %s --check-prefix=PRINT

! CHECK: shad	r2,r3
! BE-SAME: encoding: [0x43,0x2c]
! LE-SAME: encoding: [0x2c,0x43]
! PRINT: shad	r2,r3
shad r2,r3

! CHECK: shld	r2,r3
! BE-SAME: encoding: [0x43,0x2d]
! LE-SAME: encoding: [0x2d,0x43]
! PRINT: shld	r2,r3
shld r2,r3

! CHECK: clrs
! BE-SAME: encoding: [0x00,0x48]
! LE-SAME: encoding: [0x48,0x00]
! PRINT: clrs
clrs

! CHECK: sets
! BE-SAME: encoding: [0x00,0x58]
! LE-SAME: encoding: [0x58,0x00]
! PRINT: sets
sets

! CHECK: ldc	r2,ssr
! BE-SAME: encoding: [0x42,0x3e]
! LE-SAME: encoding: [0x3e,0x42]
! PRINT: ldc	r2,ssr
ldc r2,ssr

! CHECK: ldc.l	@r2+,ssr
! BE-SAME: encoding: [0x42,0x37]
! LE-SAME: encoding: [0x37,0x42]
! PRINT: ldc.l	@r2+,ssr
ldc.l @r2+,ssr

! CHECK: stc	ssr,r3
! BE-SAME: encoding: [0x03,0x32]
! LE-SAME: encoding: [0x32,0x03]
! PRINT: stc	ssr,r3
stc ssr,r3

! CHECK: stc.l	ssr,@-r3
! BE-SAME: encoding: [0x43,0x33]
! LE-SAME: encoding: [0x33,0x43]
! PRINT: stc.l	ssr,@-r3
stc.l ssr,@-r3

! CHECK: ldc	r2,spc
! BE-SAME: encoding: [0x42,0x4e]
! LE-SAME: encoding: [0x4e,0x42]
! PRINT: ldc	r2,spc
ldc r2,spc

! CHECK: ldc.l	@r2+,spc
! BE-SAME: encoding: [0x42,0x47]
! LE-SAME: encoding: [0x47,0x42]
! PRINT: ldc.l	@r2+,spc
ldc.l @r2+,spc

! CHECK: stc	spc,r3
! BE-SAME: encoding: [0x03,0x42]
! LE-SAME: encoding: [0x42,0x03]
! PRINT: stc	spc,r3
stc spc,r3

! CHECK: stc.l	spc,@-r3
! BE-SAME: encoding: [0x43,0x43]
! LE-SAME: encoding: [0x43,0x43]
! PRINT: stc.l	spc,@-r3
stc.l spc,@-r3

! CHECK: ldc	r2,r3_bank
! BE-SAME: encoding: [0x42,0xbe]
! LE-SAME: encoding: [0xbe,0x42]
! PRINT: ldc	r2,r3_bank
ldc r2,r3_bank

! CHECK: ldc.l	@r2+,r3_bank
! BE-SAME: encoding: [0x42,0xb7]
! LE-SAME: encoding: [0xb7,0x42]
! PRINT: ldc.l	@r2+,r3_bank
ldc.l @r2+,r3_bank

! CHECK: stc	r2_bank,r3
! BE-SAME: encoding: [0x03,0xa2]
! LE-SAME: encoding: [0xa2,0x03]
! PRINT: stc	r2_bank,r3
stc r2_bank,r3

! CHECK: stc.l	r2_bank,@-r3
! BE-SAME: encoding: [0x43,0xa3]
! LE-SAME: encoding: [0xa3,0x43]
! PRINT: stc.l	r2_bank,@-r3
stc.l r2_bank,@-r3

! CHECK: ldtlb
! BE-SAME: encoding: [0x00,0x38]
! LE-SAME: encoding: [0x38,0x00]
! PRINT: ldtlb
ldtlb

! CHECK: pref	@r3
! BE-SAME: encoding: [0x03,0x83]
! LE-SAME: encoding: [0x83,0x03]
! PRINT: pref	@r3
pref @r3
