! Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
! See https://llvm.org/LICENSE.txt for license information.
! SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
!
! Test harness for QEMU's SH Linux user emulator. This is not board startup.
! The ELF loader supplies initialized data/BSS; the harness supplies its own
! stack. Only write and exit use the emulator's Linux syscall interface.
.section .text.start,"ax",@progbits
.globl _start
.type _start,@function
_start:
 mov.l .Lstack,r15
 mov.l .Lmain,r0
 jsr @r0
 nop
 mov r0,r4
 mov #1,r3
 trapa #0x11
.Lhalt:
 bra .Lhalt
 nop
.p2align 2
.Lstack:
 .long __stack_top
.Lmain:
 .long sh4_start

.section .text.platform_write,"ax",@progbits
.globl platform_write
.type platform_write,@function
platform_write:
 mov r5,r6
 mov r4,r5
 mov #1,r4
 mov #4,r3
 trapa #0x13
 rts
 nop

.section .text.platform_exit,"ax",@progbits
.globl platform_exit
.type platform_exit,@function
platform_exit:
 mov #1,r3
 trapa #0x11
 bra platform_exit
 nop

.section .bss.stack,"aw",@nobits
.p2align 3
.space 16384
__stack_top:
