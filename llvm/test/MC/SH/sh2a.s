! RUN: llvm-mc -triple=sh-unknown-none-elf -mcpu=sh2a -show-encoding %s | FileCheck %s --check-prefixes=ASM,BE
! RUN: llvm-mc -triple=shle-unknown-none-elf -mcpu=sh2a -show-encoding %s | FileCheck %s --check-prefixes=ASM,LE
! RUN: llvm-mc -triple=sh-unknown-none-elf -mcpu=sh2a -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-none-elf -mcpu=sh2a -filetype=obj %s -o %t.le.o
! RUN: llvm-objdump -d --mcpu=sh2a %t.be.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -d --mcpu=sh2a %t.le.o | FileCheck %s --check-prefix=DIS

ldc r5,tbr
! ASM: ldc r5,tbr
stc tbr,r4
! ASM: stc tbr,r4
mov.b @-r5,r0
! ASM: mov.b @-r5,r0
mov.w @-r5,r0
! ASM: mov.w @-r5,r0
mov.l @-r5,r0
! ASM: mov.l @-r5,r0
mov.b r0,@r4+
! ASM: mov.b r0,@r4+
mov.w r0,@r4+
! ASM: mov.w r0,@r4+
mov.l r0,@r4+
! ASM: mov.l r0,@r4+

mov.b r5,@(2048,r4)
! ASM: mov.b r5,@(2048,r4)
! BE-SAME: encoding: [0x34,0x51,0x08,0x00]
! LE-SAME: encoding: [0x51,0x34,0x00,0x08]
mov.b @(2048,r5),r4
! ASM: mov.b @(2048,r5),r4
! BE-SAME: encoding: [0x34,0x51,0x48,0x00]
! LE-SAME: encoding: [0x51,0x34,0x00,0x48]
movu.b @(2048,r5),r4
! ASM: movu.b @(2048,r5),r4
! BE-SAME: encoding: [0x34,0x51,0x88,0x00]
! LE-SAME: encoding: [0x51,0x34,0x00,0x88]
mov.w r5,@(2048,r4)
! ASM: mov.w r5,@(2048,r4)
! BE-SAME: encoding: [0x34,0x51,0x14,0x00]
! LE-SAME: encoding: [0x51,0x34,0x00,0x14]
mov.w @(2048,r5),r4
! ASM: mov.w @(2048,r5),r4
! BE-SAME: encoding: [0x34,0x51,0x54,0x00]
! LE-SAME: encoding: [0x51,0x34,0x00,0x54]
movu.w @(2048,r5),r4
! ASM: movu.w @(2048,r5),r4
! BE-SAME: encoding: [0x34,0x51,0x94,0x00]
! LE-SAME: encoding: [0x51,0x34,0x00,0x94]
mov.l r5,@(4096,r4)
! ASM: mov.l r5,@(4096,r4)
! BE-SAME: encoding: [0x34,0x51,0x24,0x00]
! LE-SAME: encoding: [0x51,0x34,0x00,0x24]
mov.l @(4096,r5),r4
! ASM: mov.l @(4096,r5),r4
! BE-SAME: encoding: [0x34,0x51,0x64,0x00]
! LE-SAME: encoding: [0x51,0x34,0x00,0x64]

bclr #4,r4
! ASM: bclr #4,r4
bld #4,r4
! ASM: bld #4,r4
bset #4,r4
! ASM: bset #4,r4
bst #4,r4
! ASM: bst #4,r4
bclr.b #4,@(2048,r4)
! ASM: bclr.b #4,@(2048,r4)
! BE-SAME: encoding: [0x34,0x49,0x08,0x00]
! LE-SAME: encoding: [0x49,0x34,0x00,0x08]
bld.b #4,@(2048,r4)
! ASM: bld.b #4,@(2048,r4)
! BE-SAME: encoding: [0x34,0x49,0x38,0x00]
! LE-SAME: encoding: [0x49,0x34,0x00,0x38]
bset.b #4,@(2048,r4)
! ASM: bset.b #4,@(2048,r4)
! BE-SAME: encoding: [0x34,0x49,0x18,0x00]
! LE-SAME: encoding: [0x49,0x34,0x00,0x18]
bst.b #4,@(2048,r4)
! ASM: bst.b #4,@(2048,r4)
! BE-SAME: encoding: [0x34,0x49,0x28,0x00]
! LE-SAME: encoding: [0x49,0x34,0x00,0x28]
band.b #4,@(2048,r4)
! ASM: band.b #4,@(2048,r4)
! BE-SAME: encoding: [0x34,0x49,0x48,0x00]
! LE-SAME: encoding: [0x49,0x34,0x00,0x48]
bandnot.b #4,@(2048,r4)
! ASM: bandnot.b #4,@(2048,r4)
! BE-SAME: encoding: [0x34,0x49,0xc8,0x00]
! LE-SAME: encoding: [0x49,0x34,0x00,0xc8]
bldnot.b #4,@(2048,r4)
! ASM: bldnot.b #4,@(2048,r4)
! BE-SAME: encoding: [0x34,0x49,0xb8,0x00]
! LE-SAME: encoding: [0x49,0x34,0x00,0xb8]
bor.b #4,@(2048,r4)
! ASM: bor.b #4,@(2048,r4)
! BE-SAME: encoding: [0x34,0x49,0x58,0x00]
! LE-SAME: encoding: [0x49,0x34,0x00,0x58]
bornot.b #4,@(2048,r4)
! ASM: bornot.b #4,@(2048,r4)
! BE-SAME: encoding: [0x34,0x49,0xd8,0x00]
! LE-SAME: encoding: [0x49,0x34,0x00,0xd8]
bxor.b #4,@(2048,r4)
! ASM: bxor.b #4,@(2048,r4)
! BE-SAME: encoding: [0x34,0x49,0x68,0x00]
! LE-SAME: encoding: [0x49,0x34,0x00,0x68]

clips.b r4
! ASM: clips.b r4
clips.w r4
! ASM: clips.w r4
clipu.b r4
! ASM: clipu.b r4
clipu.w r4
! ASM: clipu.w r4
divs r0,r4
! ASM: divs r0,r4
divu r0,r4
! ASM: divu r0,r4
jsr/n @r5
! ASM: jsr/n @r5
jsr/n @@(8,tbr)
! ASM: jsr/n @@(8,tbr)
ldbank @r5,r0
! ASM: ldbank @r5,r0
movml.l r5,@-r15
! ASM: movml.l r5,@-r15
movml.l @r15+,r5
! ASM: movml.l @r15+,r5
movmu.l r5,@-r15
! ASM: movmu.l r5,@-r15
movmu.l @r15+,r5
! ASM: movmu.l @r15+,r5
movrt r4
! ASM: movrt r4
mulr r0,r4
! ASM: mulr r0,r4
nott
! ASM: nott
resbank
! ASM: resbank
rts/n
! ASM: rts/n
rtv/n r5
! ASM: rtv/n r5
stbank r0,@r4
! ASM: stbank r0,@r4

movi20 #4,r4
! ASM: movi20 #4,r4
! BE-SAME: encoding: [0x04,0x00,0x00,0x04]
! LE-SAME: encoding: [0x00,0x04,0x04,0x00]
movi20s #1024,r4
! ASM: movi20s #1024,r4
! BE-SAME: encoding: [0x04,0x01,0x00,0x04]
! LE-SAME: encoding: [0x01,0x04,0x04,0x00]
fmov.s fr2,@(2048,r4)
! ASM: fmov.s fr2,@(2048,r4)
! BE-SAME: encoding: [0x34,0x21,0x32,0x00]
! LE-SAME: encoding: [0x21,0x34,0x00,0x32]
fmov.s @(2048,r5),fr1
! ASM: fmov.s @(2048,r5),fr1
! BE-SAME: encoding: [0x31,0x51,0x72,0x00]
! LE-SAME: encoding: [0x51,0x31,0x00,0x72]
fmov.d dr2,@(4096,r4)
! ASM: fmov.d dr2,@(4096,r4)
! BE-SAME: encoding: [0x34,0x21,0x32,0x00]
! LE-SAME: encoding: [0x21,0x34,0x00,0x32]
fmov.d @(4096,r5),dr2
! ASM: fmov.d @(4096,r5),dr2
! BE-SAME: encoding: [0x32,0x51,0x72,0x00]
! LE-SAME: encoding: [0x51,0x32,0x00,0x72]

! DIS: ldc r5,tbr
! DIS: mov.b r5,@(2048,r4)
! DIS: band.b #4,@(2048,r4)
! DIS: jsr/n @@(8,tbr)
! DIS: movi20 #4,r4
! DIS: movi20s #1024,r4
! DIS: fmov.d dr2,@(4096,r4)
