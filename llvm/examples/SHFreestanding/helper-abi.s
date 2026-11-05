! Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
! See https://llvm.org/LICENSE.txt for license information.
! SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
!
! Independently supply 64-bit arguments in SH4 little-endian ABI registers:
! r4/r5 and r6/r7, with low word first. Check the result in r0/r1.
.text
.globl helper_abi_test
.type helper_abi_test,@function
helper_abi_test:
 sts.l pr,@-r15
 mov #2,r4
 mov #1,r5
 mov #3,r6
 mov #0,r7
 mov.l .Ldivide,r0
 jsr @r0
 nop
 mov.l .Lquotient,r2
 cmp/eq r2,r0
 bf .Lfailed
 tst r1,r1
 bf .Lfailed
 mov #0,r0
 bra .Lreturn
 nop
.Lfailed:
 mov #1,r0
.Lreturn:
 lds.l @r15+,pr
 rts
 nop
.p2align 2
.Ldivide:
 .long __udivdi3
.Lquotient:
 .long 0x55555556
