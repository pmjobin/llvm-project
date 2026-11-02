; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O1 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O1 -verify-machineinstrs < %s | FileCheck %s

; CHECK-LABEL: pass_float:
; CHECK: rts
; CHECK-NEXT: fmov fr4,fr0

define float @pass_float(float %value) #0 {
	ret float %value
}

; CHECK-LABEL: pass_double:
; CHECK: mov r5,r1
; CHECK-NEXT: rts
; CHECK-NEXT: mov r4,r0
; CHECK-NOT: {{[dx]}}r{{[0-9]+}}

define double @pass_double(double %value) #0 {
	ret double %value
}

; The ordinary E-class GPR cursor is sequential: a double after one integer
; starts in r5/r6 rather than skipping to an aligned pair.

; CHECK-LABEL: pass_double_after_integer:
; CHECK: mov r6,r1
; CHECK-NEXT: rts
; CHECK-NEXT: mov r5,r0

define double @pass_double_after_integer(i32 %integer, double %value) #0 {
	ret double %value
}

; The GPR and FPR cursors are independent.  E-class float arguments use
; FR4/FR5 in natural order in both endian modes; double uses sequential GPR
; words.

; CHECK-LABEL: mixed:
; CHECK-DAG: fmov fr4,fr13
; CHECK-DAG: fmov fr5,fr12
; CHECK: __adddf3
; CHECK-NOT: {{[dx]}}r{{[0-9]+}}

define double @mixed(i32 %first, float %single, double %wide, i32 %tail, float %last) #0 {
	%first.fp = sitofp i32 %first to double
	%single.fp = fpext float %single to double
	%sum0 = fadd double %first.fp, %single.fp
	%sum1 = fadd double %sum0, %wide
	%tail.fp = sitofp i32 %tail to double
	%sum2 = fadd double %sum1, %tail.fp
	%last.fp = fpext float %last to double
	%result = fadd double %sum2, %last.fp
	ret double %result
}

declare float @callee_float(float)
declare void @clobber()

; CHECK-LABEL: call_float:
; CHECK: jsr
; CHECK: fadd {{fr[0-9]+}},fr0
; CHECK-NOT: SET_FPSCR_MODE

define float @call_float(float %value) #0 {
	%called = call float @callee_float(float %value)
	%result = fadd float %called, %value
	ret float %result
}

; CHECK-LABEL: live_four:
; CHECK-DAG: .cfi_offset fr12,
; CHECK-DAG: .cfi_offset fr13,
; CHECK-DAG: .cfi_offset fr14,
; CHECK-DAG: .cfi_offset fr15,
; CHECK: jsr
; CHECK-DAG: .cfi_restore fr12
; CHECK-DAG: .cfi_restore fr13
; CHECK-DAG: .cfi_restore fr14
; CHECK-DAG: .cfi_restore fr15

define float @live_four(float %a, float %b, float %c, float %d) #0 {
	call void @clobber()
	%ab = fadd float %a, %b
	%cd = fadd float %c, %d
	%result = fadd float %ab, %cd
	ret float %result
}

; CHECK-LABEL: call_indirect:
; CHECK: jsr @r0

define float @call_indirect(ptr %callee, i32 %integer, float %single, double %wide) #0 {
	%result = call float %callee(i32 %integer, float %single, double %wide)
	ret float %result
}

declare void @variadic(i32, ...)

; Source double is f32 for this profile, so default promotion legitimately
; produces an unnamed f32.  It follows the E FPR cursor while an unnamed LLVM
; f64 follows the ordinary sequential GPR cursor.

; CHECK-LABEL: call_variadic:
; CHECK-DAG: mov {{r[0-9]+}},r5
; CHECK-DAG: mov {{r[0-9]+}},r6
; CHECK: jsr

define void @call_variadic(float %single, double %wide) #0 {
	call void (i32, ...) @variadic(i32 7, float %single, double %wide)
	ret void
}

declare float @callee_f12(float, float, float, float, float, float, float, float, float, float, float, float)

; CHECK-LABEL: forward_f12:
; CHECK: sts.l pr,@-r15
; CHECK: add #-4,r15
; CHECK: mov #8,r0
; CHECK-NEXT: fmov.s @(r0,r15),[[A8:fr[0-9]+]]
; CHECK-NEXT: mov #20,r0
; CHECK-NEXT: fmov.s @(r0,r15),[[A11:fr[0-9]+]]
; CHECK-NEXT: mov #16,r0
; CHECK-NEXT: fmov.s @(r0,r15),[[A10:fr[0-9]+]]
; CHECK-NEXT: mov #12,r0
; CHECK-NEXT: fmov.s @(r0,r15),[[A9:fr[0-9]+]]
; CHECK-NEXT: add #-16,r15
; CHECK: mov #4,r0
; CHECK-NEXT: fmov [[A9]],@(r0,r15)
; CHECK-NEXT: mov #8,r0
; CHECK-NEXT: fmov [[A10]],@(r0,r15)
; CHECK-NEXT: mov #12,r0
; CHECK-NEXT: fmov [[A11]],@(r0,r15)
; CHECK: jsr
; CHECK-NEXT: fmov [[A8]],@r15

define float @forward_f12(float %a0, float %a1, float %a2, float %a3, float %a4, float %a5, float %a6, float %a7, float %a8, float %a9, float %a10, float %a11) #0 {
	%result = call float @callee_f12(float %a0, float %a1, float %a2, float %a3, float %a4, float %a5, float %a6, float %a7, float %a8, float %a9, float %a10, float %a11)
	ret float %result
}

attributes #0 = { noinline "target-abi"="gnu-e-float" "target-cpu"="sh4" "target-features"="+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-e-float"}
