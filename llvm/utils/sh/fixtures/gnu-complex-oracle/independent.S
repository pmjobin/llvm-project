// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
.text
.align 2
.global capture_odd
.type capture_odd,@function
capture_odd:
 mov.l .Lcap,r1
 fmov.s fr4,@r1
 add #4,r1
 fmov.s fr5,@r1
 add #4,r1
 fmov.s fr6,@r1
 add #4,r1
 fmov.s fr7,@r1
 add #4,r1
 fmov.s fr8,@r1
 add #4,r1
 fmov.s fr9,@r1
 add #4,r1
 fmov.s fr10,@r1
 add #4,r1
 fmov.s fr11,@r1
 add #4,r1
 mov.l r5,@r1
 rts
 nop
.align 2
.Lcap: .long captured
.global asm_call_odd
.type asm_call_odd,@function
asm_call_odd:
 sts.l pr,@-r15
 mov.l .Lvalues,r1
 fmov.s @r1+,fr5
 fmov.s @r1+,fr4
 fmov.s @r1+,fr7
 fmov.s @r1+,fr6
 mov.l @r1,r5
 mov #0,r4
 mov.l .Lcallee,r1
 jsr @r1
 nop
 lds.l @r15+,pr
 rts
 nop
.align 2
.Lvalues: .long .Ldata
.Lcallee: .long c1
.Ldata: .long 0x40e00000,0x3fa00000,0xc0600000,0x41100000,0x31415926
.section .note.GNU-stack,"",@progbits
