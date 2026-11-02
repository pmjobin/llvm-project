; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -O3 -verify-machineinstrs -stop-after=postmisched %s -o - | FileCheck %s --check-prefix=LATE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -O3 -verify-machineinstrs -stop-after=sh-fpu-state %s -o - | FileCheck %s --check-prefix=EARLY
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -O3 -verify-machineinstrs -stop-after=machine-scheduler %s -o - | FileCheck %s --check-prefix=EARLY
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -O3 -verify-machineinstrs -post-RA-scheduler=true -stop-after=post-RA-sched %s -o - | FileCheck %s --check-prefix=LATE
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM

; The return transition changes PR but must preserve current cause and flags.
define float @status_mul(float %a, float %b) {
	%r = fmul float %a, %b
	ret float %r
}
; EARLY-LABEL: name: status_mul
; EARLY: FMUL_FR {{.*}}implicit-def {{(dead )?}}$fpscr{{.*}}implicit $fpscr
; LATE-LABEL: name: status_mul
; LATE: FMUL_FR {{.*}}implicit-def {{(dead )?}}$fpscr{{.*}}implicit $fpscr
; LATE: STS_FPSCR implicit $fpscr
; LATE: LDS_FPSCR
; ASM-LABEL: status_mul:
; ASM: fmul
; ASM: sts fpscr,
; ASM: lds {{.*}},fpscr

declare float @status_callee(float)

define float @status_barriers(float %a, float %b) {
	%r = fmul float %a, %b
	%bits = bitcast float %r to i32
	call void asm sideeffect "! explicit state barrier", "r,~{fpscr}"(i32 %bits)
	%c = call float @status_callee(float %r)
	%s = fadd float %c, %b
	ret float %s
}
; EARLY-LABEL: name: status_barriers
; EARLY: FMUL_FR {{.*}}implicit-def {{(dead )?}}$fpscr{{.*}}implicit $fpscr
; EARLY: INLINEASM
; EARLY: JSR
; EARLY: FADD_FR {{.*}}implicit-def {{(dead )?}}$fpscr{{.*}}implicit $fpscr
; LATE-LABEL: name: status_barriers
; LATE: FMUL_FR {{.*}}implicit-def {{(dead )?}}$fpscr{{.*}}implicit $fpscr
; LATE: INLINEASM
; LATE: JSR
; LATE: FADD_FR {{.*}}implicit-def {{(dead )?}}$fpscr{{.*}}implicit $fpscr
; ASM-LABEL: status_barriers:
; ASM: fmul
; ASM: jsr
; ASM: fadd
; ASM: sts fpscr,

define float @status_add_size(float %a, float %b) optsize {
	%r = fadd float %a, %b
	ret float %r
}
; EARLY-LABEL: name: status_add_size
; EARLY: FADD_FR {{.*}}implicit-def {{(dead )?}}$fpscr{{.*}}implicit $fpscr
; LATE-LABEL: name: status_add_size
; LATE: FADD_FR {{.*}}implicit-def {{(dead )?}}$fpscr{{.*}}implicit $fpscr
; LATE: STS_FPSCR implicit $fpscr
; ASM-LABEL: status_add_size:
; ASM: fadd
; ASM: sts fpscr,
; ASM: lds {{.*}},fpscr

; A comparison has two observable outputs: T and FPSCR. Preserving status must
; retain the predicate consumed by MOVT and the integer return value.
define i32 @status_compare(float %a, float %b) {
	%p = fcmp ogt float %a, %b
	%r = zext i1 %p to i32
	ret i32 %r
}
; EARLY-LABEL: name: status_compare
; EARLY: FCMP_GT_FR {{.*}}implicit-def $tbit{{.*}}implicit-def {{(dead )?}}$fpscr{{.*}}implicit $fpscr
; LATE-LABEL: name: status_compare
; LATE: FCMP_GT_FR {{.*}}implicit-def $tbit{{.*}}implicit-def {{(dead )?}}$fpscr{{.*}}implicit $fpscr
; ASM-LABEL: status_compare:
; ASM: fcmp/gt
; ASM: movt

define float @status_convert(i32 %a) minsize optsize {
	%r = sitofp i32 %a to float
	ret float %r
}
; EARLY-LABEL: name: status_convert
; EARLY: I32_TO_F32_PSEUDO {{.*}}implicit-def {{(dead )?}}$fpul{{.*}}implicit-def {{(dead )?}}$fpscr{{.*}}implicit $fpscr
; LATE-LABEL: name: status_convert
; LATE: FLOAT_FR {{.*}}implicit-def {{(dead )?}}$fpscr{{.*}}implicit $fpul{{.*}}implicit $fpscr
; LATE: STS_FPSCR implicit $fpscr
; ASM-LABEL: status_convert:
; ASM: float fpul,
; ASM: sts fpscr,
; ASM: lds {{.*}},fpscr
