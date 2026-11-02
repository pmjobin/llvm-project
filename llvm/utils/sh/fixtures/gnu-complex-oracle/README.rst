Entirely GNU-linked odd-cursor diagnostic
========================================

Compile main.c, consumer.c and independent.S with the pinned GNU SH4 compiler
at O0, Os and O3, using ``-m4 -ml -mno-renesas -fno-pic -fno-pie -static
-no-pie`` and the matching explicit sysroot/binutils. Keep every assembly file
before assembling/linking. No LLVM object or supplemental compiler runtime is
needed. Run with the frozen scalar SH4 emulator, CPU sh7750r.

The expected complex components are real1.25 (3fa00000) and imaginary-3.5
(c0600000), followed by9 (41100000) and integer31415926. Free/even, odd,
exact-fit, overflow and exhausted FPR cursors are independent observations.
The assembly control captures the odd caller's actual FR4/FR5/FR6/FR7 contents
and supplies an independent caller for the GNU consumer.

GNU14.2 reverses real/imaginary for the fitting odd-cursor consumer, including
with its own caller. Save-area bytes and the independent caller show that
placement is correct: the consumer applies an unconditional pair reversal
which is valid only at even saved cursors. Keep this as a failed compatibility
case; a zero process exit or a successful control does not waive it. The test
prints observations for review, and the report retains per-component scoring.

The original GNU SDK remains the comparison reference. Any compiler-only
consumer correction must be built and pinned separately, then requalify all
directions before an owner-supported SDK change. An old GCC2.95 diagnostic
compiler corroborates the direct case but has different aggregate rules and
is unsuitable as a replacement.
