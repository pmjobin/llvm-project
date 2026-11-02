; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O1 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O1 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=COMMON,BE

; The module ABI flag, rather than CPU identity or FPU target features, selects
; GNU SH-4 hard-float transport.

; COMMON-LABEL: pass_float:
; COMMON: rts
; LE-NEXT: fmov fr5,fr0
; BE-NEXT: fmov fr4,fr0

define float @pass_float(float %value) #0 {
	ret float %value
}

; COMMON-LABEL: pass_double:
; COMMON: fmov fr4,fr0
; COMMON-NEXT: rts
; COMMON-NEXT: fmov fr5,fr1

define double @pass_double(double %value) #0 {
	ret double %value
}

; Integer and FPU cursors are independent.  The integer values arrive in
; r4/r5, the float uses the first scalar slot, and aligning the later double
; skips the other half of the first FPU pair and uses dr6.

; COMMON-LABEL: mixed:
; COMMON: add r5,r4
; LE: fadd fr5,{{fr[0-9]+}}
; BE: fadd fr4,{{fr[0-9]+}}
; COMMON: fcnvds dr6,fpul
; COMMON: fsts fpul,fr0

define float @mixed(i32 %first, float %single, double %wide, i32 %tail) #0 {
	%integers = add i32 %first, %tail
	%integer.fp = sitofp i32 %integers to float
	%with.single = fadd float %single, %integer.fp
	%wide.single = fptrunc double %wide to float
	%result = fadd float %with.single, %wide.single
	ret float %result
}

declare float @callee_float(float)

; The call argument and result remain in FPU registers.  The ABI state is
; restored before the call and again before return after single-precision work.

; COMMON-LABEL: call_float:
; COMMON: .cfi_offset fr12,
; COMMON: jsr
; COMMON: fadd {{fr[0-9]+}},fr0
; COMMON: sts fpscr,r0
; COMMON: xor r1,r0
; COMMON: lds r0,fpscr

define float @call_float(float %value) #0 {
	%called = call float @callee_float(float %value)
	%result = fadd float %called, %value
	ret float %result
}

declare double @callee_double(double)

; COMMON-LABEL: call_double:
; COMMON: jsr
; COMMON: fadd {{dr[0-9]+}},dr0

define double @call_double(double %value) #0 {
	%called = call double @callee_double(double %value)
	%result = fadd double %called, %value
	ret double %result
}

declare void @clobber()

; FR12-FR15 are GNU callee-saved registers.  Keeping four scalar values live
; across a call exercises their prologue, epilogue, and established DWARF
; numbers (FR0 is 25, so these are 37 through 40).

; COMMON-LABEL: live_four:
; COMMON-DAG: .cfi_offset fr12,
; COMMON-DAG: .cfi_offset fr13,
; COMMON-DAG: .cfi_offset fr14,
; COMMON-DAG: .cfi_offset fr15,
; COMMON: jsr
; COMMON-DAG: .cfi_restore fr12
; COMMON-DAG: .cfi_restore fr13
; COMMON-DAG: .cfi_restore fr14
; COMMON-DAG: .cfi_restore fr15

define float @live_four(float %a, float %b, float %c, float %d) #0 {
	call void @clobber()
	%ab = fadd float %a, %b
	%cd = fadd float %c, %d
	%result = fadd float %ab, %cd
	ret float %result
}

; COMMON-LABEL: call_indirect:
; COMMON: jsr @r0

define float @call_indirect(ptr %callee, i32 %first, float %single, double %wide, i32 %tail) #0 {
	%result = call float %callee(i32 %first, float %single, double %wide, i32 %tail)
	ret float %result
}

declare float @callee_f12(float, float, float, float, float, float, float, float, float, float, float, float)

; The ninth through twelfth scalar FP arguments overflow to four consecutive
; stack words; they never fall back to r4-r7.

; COMMON-LABEL: forward_f12:
; COMMON: sts.l pr,@-r15
; COMMON: add #-4,r15
; COMMON: mov #8,r0
; COMMON-NEXT: fmov.s @(r0,r15),[[A8:fr[0-9]+]]
; COMMON-NEXT: mov #20,r0
; COMMON-NEXT: fmov.s @(r0,r15),[[A11:fr[0-9]+]]
; COMMON-NEXT: mov #16,r0
; COMMON-NEXT: fmov.s @(r0,r15),[[A10:fr[0-9]+]]
; COMMON-NEXT: mov #12,r0
; COMMON-NEXT: fmov.s @(r0,r15),[[A9:fr[0-9]+]]
; COMMON-NEXT: add #-16,r15
; COMMON: mov #4,r0
; COMMON-NEXT: fmov [[A9]],@(r0,r15)
; COMMON-NEXT: mov #8,r0
; COMMON-NEXT: fmov [[A10]],@(r0,r15)
; COMMON-NEXT: mov #12,r0
; COMMON-NEXT: fmov [[A11]],@(r0,r15)
; COMMON: jsr
; COMMON-NEXT: fmov [[A8]],@r15

define float @forward_f12(float %a0, float %a1, float %a2, float %a3, float %a4, float %a5, float %a6, float %a7, float %a8, float %a9, float %a10, float %a11) #0 {
	%result = call float @callee_f12(float %a0, float %a1, float %a2, float %a3, float %a4, float %a5, float %a6, float %a7, float %a8, float %a9, float %a10, float %a11)
	ret float %result
}

declare double @callee_d8(double, double, double, double, double, double, double, double)

; Four double arguments use dr4/dr6/dr8/dr10.  The remaining four occupy
; consecutive 8-byte stack slots.

; COMMON-LABEL: forward_d8:
; COMMON: add #-32,r15
; COMMON-COUNT-7: fmov.s {{fr[0-9]+}},@
; COMMON: jsr
; COMMON-NEXT: fmov.s {{fr[0-9]+}},@

define double @forward_d8(double %a0, double %a1, double %a2, double %a3, double %a4, double %a5, double %a6, double %a7) #0 {
	%result = call double @callee_d8(double %a0, double %a1, double %a2, double %a3, double %a4, double %a5, double %a6, double %a7)
	ret double %result
}

; GNU complex scalar modes retain FPU argument and result transport.  They are
; distinct from an ordinary C struct containing two floating members.

; COMMON-LABEL: pass_complex_float:
; COMMON-DAG: fmov fr4,fr0
; COMMON-DAG: fmov fr5,fr1

define { float, float } @pass_complex_float({ float, float } %value) #0 {
	ret { float, float } %value
}

; COMMON-LABEL: pass_complex_double:
; COMMON-DAG: fmov fr4,fr0
; COMMON-DAG: fmov fr5,fr1
; COMMON-DAG: fmov fr6,fr2
; COMMON-DAG: fmov fr7,fr3

define { double, double } @pass_complex_double({ double, double } %value) #0 {
	ret { double, double } %value
}

declare double @variadic(i32, ...)

; COMMON-LABEL: call_variadic:
; COMMON: jsr
; COMMON-NEXT: mov #2,r4

define double @call_variadic(double %first, double %second) #0 {
	%result = call double (i32, ...) @variadic(i32 2, double %first, double %second)
	ret double %result
}

declare void @variadic_integer_boundary(i32, i32, i32, ...)

; GNU does not split an unnamed two-word integer between r7 and the stack.
; The i64 is wholly on the stack, leaving r7 for the later i32.

; COMMON-LABEL: call_variadic_integer_boundary:
; COMMON-DAG: mov r4,[[WORD0:r[0-9]+]]
; COMMON-DAG: mov r5,[[WORD1:r[0-9]+]]
; COMMON: mov #9,r7
; COMMON: mov.l [[WORD0]],@r15
; COMMON: jsr
; COMMON-NEXT: mov.l [[WORD1]],@(4,r15)

define void @call_variadic_integer_boundary(i64 %wide) #0 {
	call void (i32, i32, i32, ...) @variadic_integer_boundary(i32 1, i32 2, i32 3, i64 %wide, i32 9)
	ret void
}

declare void @integer_boundary(i32, i32, i32, i64, i32)

; The same whole-value rule applies to an ordinary nonvariadic i64.

; COMMON-LABEL: call_integer_boundary:
; COMMON-DAG: mov r4,[[WORD0:r[0-9]+]]
; COMMON-DAG: mov r5,[[WORD1:r[0-9]+]]
; COMMON: mov #9,r7
; COMMON: mov.l [[WORD0]],@r15
; COMMON: jsr
; COMMON-NEXT: mov.l [[WORD1]],@(4,r15)

define void @call_integer_boundary(i64 %wide) #0 {
	call void @integer_boundary(i32 1, i32 2, i32 3, i64 %wide, i32 9)
	ret void
}

declare void @variadic_fpu_boundary(float, float, float, float, float, float, float, i32, ...)

; An aligned double that cannot fit the final logical FPR slot goes wholly to
; the stack without consuming that slot.  A later scalar value uses fr10.

; COMMON-LABEL: call_variadic_fpu_boundary:
; COMMON: sts.l pr,@-r15
; COMMON: add #-4,r15
; COMMON: mov r15,r0
; COMMON-NEXT: add #8,r0
; COMMON-NEXT: fmov.s @r0+,[[FPWORD0:fr[0-9]+]]
; COMMON-NEXT: fmov.s @r0+,[[FPWORD1:fr[0-9]+]]
; COMMON-NEXT: add #-8,r15
; LE: fmov @r0,fr10
; BE: fmov @r0,fr11
; COMMON: fmov.s [[FPWORD0]],@r15
; COMMON: mov #4,r0
; COMMON: jsr
; COMMON-NEXT: fmov.s [[FPWORD1]],@(r0,r15)

define void @call_variadic_fpu_boundary(float %a0, float %a1, float %a2, float %a3, float %a4, float %a5, float %a6, double %wide) #0 {
	call void (float, float, float, float, float, float, float, i32, ...) @variadic_fpu_boundary(float %a0, float %a1, float %a2, float %a3, float %a4, float %a5, float %a6, i32 0, double %wide, float 3.5)
	ret void
}

; Remaining scalar fallback helpers use the selected public ABI.  The integer
; argument is in r4 and __floatunsisf returns directly in fr0.

; COMMON-LABEL: unsigned_to_float:
; COMMON: jsr
; COMMON-NOT: lds r0,fpul
; COMMON: rts
; COMMON: .long __floatunsisf

define float @unsigned_to_float(i32 %value) #0 {
	%result = uitofp i32 %value to float
	ret float %result
}

attributes #0 = { noinline "target-cpu"="sh4" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}
