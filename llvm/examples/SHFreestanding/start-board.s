! Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
! See https://llvm.org/LICENSE.txt for license information.
! SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
!
! Illustrative board startup. Adapt the memory map and boot/interrupt/MMU/cache
! initialization to the board before entering _start. This is not a ROM image
! for a named board. platform_exit and platform_write are application hooks.
.section .text.start,"ax",@progbits
.globl _start
.type _start,@function
_start:
 mov.l .Lstack,r15
 mov.l .Lload,r0
 mov.l .Ldata,r1
 mov.l .Ldata_end,r2
.Lcopy:
 cmp/eq r2,r1
 bt .Lclear_start
 mov.b @r0+,r3
 mov.b r3,@r1
 add #1,r1
 bra .Lcopy
 nop
.Lclear_start:
 mov.l .Lbss,r1
 mov.l .Lbss_end,r2
 mov #0,r0
.Lclear:
 cmp/eq r2,r1
 bt .Lrun
 mov.b r0,@r1
 add #1,r1
 bra .Lclear
 nop
.Lrun:
 mov.l .Lentry,r0
 jsr @r0
 nop
 mov r0,r4
 mov.l .Lexit,r0
 jsr @r0
 nop
.Lhalt:
 bra .Lhalt
 nop
.p2align 2
.Lstack:
 .long __stack_top
.Lload:
 .long __data_load_start
.Ldata:
 .long __data_start
.Ldata_end:
 .long __data_end
.Lbss:
 .long __bss_start
.Lbss_end:
 .long __bss_end
.Lentry:
 .long sh4_start
.Lexit:
 .long platform_exit
