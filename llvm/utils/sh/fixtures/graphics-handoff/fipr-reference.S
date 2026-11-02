/* Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

/* Hand-authored optimization reference, NOT current compiler output and NOT
 * runtime-qualified. R4=out, R5=a[4], R6=b; all pointers are 8-byte aligned.
 * Read every input before writing output, matching dot4_sequence snapshots.
 * FR0..3 pack results, FV4 holds b, FV8 is the reusable input/result group.
 * FLDS/FSTS copy one lane through FPUL without changing SZ. A compiler pattern
 * must model the FPUL def/use and all FR/FV aliases before allocation.
 * These functions clobber only caller-saved registers, including FPUL. */
        .text
        .macro dots_body
        fmov.d @r6+,dr4
        fmov.d @r6+,dr6
        fmov.d @r5+,dr8
        fmov.d @r5+,dr10
        fipr fv4,fv8
        flds fr11,fpul
        fsts fpul,fr0
        fmov.d @r5+,dr8
        fmov.d @r5+,dr10
        fipr fv4,fv8
        flds fr11,fpul
        fsts fpul,fr1
        fmov.d @r5+,dr8
        fmov.d @r5+,dr10
        fipr fv4,fv8
        flds fr11,fpul
        fsts fpul,fr2
        fmov.d @r5+,dr8
        fmov.d @r5+,dr10
        fipr fv4,fv8
        flds fr11,fpul
        fsts fpul,fr3
        add #16,r4
        fmov.d dr2,@-r4
        fmov.d dr0,@-r4
        .endm

/* Standard campaign SH4 boundary: PR=1, SZ=0, FR=0. */
        .align 2
        .global reference_dot4_sequence_m4
        .type reference_dot4_sequence_m4,@function
reference_dot4_sequence_m4:
        sts fpscr,r0
        mov #24,r1
        shll16 r1
        xor r1,r0
        lds r0,fpscr
        dots_body
        sts fpscr,r0
        xor r1,r0
        lds r0,fpscr
        rts
        nop
        .size reference_dot4_sequence_m4,.-reference_dot4_sequence_m4

/* Different, explicitly single-mode boundary: PR=0, SZ=0, FR=0.
 * Do not call this entry with the campaign's ordinary -m4 runtime contract. */
        .align 2
        .global reference_dot4_sequence_single_mode
        .type reference_dot4_sequence_single_mode,@function
reference_dot4_sequence_single_mode:
        fschg
        dots_body
        fschg
        rts
        nop
        .size reference_dot4_sequence_single_mode,.-reference_dot4_sequence_single_mode
        .section .note.GNU-stack,"",@progbits
