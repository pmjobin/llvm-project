/* Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

/* Compile-only counterexample: an ordinary external function writes XD0
 * without FTRV. Entry/exit: SH4 GNU hard-float PR=1, SZ=0, FR=0.
 * R4 points to an accessible, eight-byte-aligned pair of float words.
 * R0/R1 and XD0 are call-clobbered. No XD12/XD14 state is changed. */
        .text
        .align 2
        .global overwrite_xd0
        .type overwrite_xd0,@function
overwrite_xd0:
        sts fpscr,r0
        mov #24,r1
        shll16 r1
        xor r1,r0
        lds r0,fpscr
        fmov.d @r4,xd0
        sts fpscr,r0
        xor r1,r0
        lds r0,fpscr
        rts
        nop
        .size overwrite_xd0,.-overwrite_xd0
        .section .note.GNU-stack,"",@progbits
