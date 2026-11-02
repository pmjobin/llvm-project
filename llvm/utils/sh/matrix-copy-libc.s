! Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
! SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
!
! Independent integer-only GNU ABI test support. R4=dst,R5=src,R6=bytes;
! return original dst in R0. No stack, callee-save or FPU state is touched.
! memcpy requires disjoint ranges; memmove supports arbitrary overlap/length.
.text
.balign 2
.global memcpy
.type memcpy,@function
memcpy:
  mov r4,r0
.Lforward:
  tst r6,r6
  bt .Lreturn
.Lforward_loop:
  mov.b @r5+,r1
  mov.b r1,@r4
  add #1,r4
  dt r6
  bf .Lforward_loop
.Lreturn:
  rts
  nop
.size memcpy,.-memcpy

.balign 2
.global memmove
.type memmove,@function
memmove:
  mov r4,r0
  cmp/hs r5,r4
  bf .Lforward
  mov r4,r2
  sub r5,r2
  cmp/hs r6,r2
  bt .Lforward
  add r6,r4
  add r6,r5
  tst r6,r6
  bt .Lreturn
.Lbackward_loop:
  add #-1,r5
  mov.b @r5,r1
  mov.b r1,@-r4
  dt r6
  bf .Lbackward_loop
  rts
  nop
.size memmove,.-memmove
.section .note.GNU-stack,"",@progbits
