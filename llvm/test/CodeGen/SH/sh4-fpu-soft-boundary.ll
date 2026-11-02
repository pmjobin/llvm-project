; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=COMMON,BE --implicit-check-not='xf{{[0-9]+}}' --implicit-check-not='xd{{[0-9]+}}' --implicit-check-not=frchg
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=COMMON,LE --implicit-check-not='xf{{[0-9]+}}' --implicit-check-not='xd{{[0-9]+}}' --implicit-check-not=frchg

; The public ABI remains a raw GPR/stack transport.  Only computation after
; the boundary uses the primary FR bank and its even DR views.

; COMMON-LABEL: add_f32:
; COMMON: lds r5,fpul
; COMMON-NEXT: fsts fpul,fr1
; COMMON: lds r4,fpul
; COMMON-NEXT: fsts fpul,fr0
; COMMON: fadd fr1,fr0
; COMMON: flds fr0,fpul
; COMMON-NEXT: rts
; COMMON-NEXT: sts fpul,r0

define float @add_f32(float %a, float %b) {
	%result = fadd float %a, %b
	ret float %result
}

; BE-LABEL: add_f64:
; BE: lds r6,fpul
; BE-NEXT: fsts fpul,fr2
; BE-NEXT: lds r7,fpul
; BE-NEXT: fsts fpul,fr3
; BE: lds r4,fpul
; BE-NEXT: fsts fpul,fr0
; BE-NEXT: lds r5,fpul
; BE-NEXT: fsts fpul,fr1
; BE: fadd dr2,dr0
; BE: flds fr0,fpul
; BE-NEXT: sts fpul,r0
; BE-NEXT: flds fr1,fpul
; BE-NEXT: rts
; BE-NEXT: sts fpul,r1

; LE-LABEL: add_f64:
; LE: lds r7,fpul
; LE-NEXT: fsts fpul,fr2
; LE-NEXT: lds r6,fpul
; LE-NEXT: fsts fpul,fr3
; LE: lds r5,fpul
; LE-NEXT: fsts fpul,fr0
; LE-NEXT: lds r4,fpul
; LE-NEXT: fsts fpul,fr1
; LE: fadd dr2,dr0
; LE: flds fr0,fpul
; LE-NEXT: sts fpul,r1
; LE-NEXT: flds fr1,fpul
; LE-NEXT: rts
; LE-NEXT: sts fpul,r0

define double @add_f64(double %a, double %b) {
	%result = fadd double %a, %b
	ret double %result
}

; COMMON-LABEL: memory_f32:
; COMMON: fmov @r4,fr0
; COMMON: fadd fr0,fr0
; COMMON: fmov fr0,@r5

define void @memory_f32(ptr %source, ptr %destination) {
	%value = load float, ptr %source, align 4
	%result = fadd float %value, %value
	store float %result, ptr %destination, align 4
	ret void
}

; COMMON-LABEL: memory_f64:
; BE: fmov.s @r4,fr0
; LE: fmov.s @r4,fr1
; COMMON: mov #4,r0
; BE: fmov.s @(r0,r4),fr1
; LE: fmov.s @(r0,r4),fr0
; COMMON-NOT: fmov.d
; COMMON: fadd dr0,dr0
; COMMON-NOT: fmov.d
; BE: fmov.s fr0,@r5
; BE: fmov.s fr1,@(r0,r5)
; LE: fmov.s fr1,@r5
; LE: fmov.s fr0,@(r0,r5)

define void @memory_f64(ptr %source, ptr %destination) {
	%value = load double, ptr %source, align 4
	%result = fadd double %value, %value
	store double %result, ptr %destination, align 4
	ret void
}

declare float @callee_f32(float)
declare double @callee_f64(double)

; COMMON-LABEL: call_f32:
; COMMON: fadd
; COMMON: flds {{fr[0-9]+}},fpul
; COMMON: jsr
; COMMON-NEXT: sts fpul,r4
; COMMON: sts fpscr,r0
; COMMON: lds r0,fpscr
; COMMON: lds r0,fpul
; COMMON: fsts fpul,{{fr[0-9]+}}
; COMMON: fmul

define float @call_f32(float %a, float %b) {
	%argument = fadd float %a, %b
	%returned = call float @callee_f32(float %argument)
	%result = fmul float %returned, %argument
	ret float %result
}

; COMMON-LABEL: call_f64:
; COMMON: fadd dr{{[0-9]+}},dr{{[0-9]+}}
; COMMON: jsr
; COMMON: sts fpscr,r0
; COMMON: lds r0,fpscr
; COMMON: fmul dr{{[0-9]+}},dr{{[0-9]+}}

define double @call_f64(double %a, double %b) {
	%argument = fadd double %a, %b
	%returned = call double @callee_f64(double %argument)
	%result = fmul double %returned, %argument
	ret double %result
}

declare float @llvm.sqrt.f32(float)
declare double @llvm.sqrt.f64(double)

; COMMON-LABEL: sqrt_f32:
; COMMON: fsqrt fr{{[0-9]+}}

define float @sqrt_f32(float %value) {
	%result = call float @llvm.sqrt.f32(float %value)
	ret float %result
}

; COMMON-LABEL: sqrt_f64:
; COMMON: fsqrt dr{{[0-9]+}}

define double @sqrt_f64(double %value) {
	%result = call double @llvm.sqrt.f64(double %value)
	ret double %result
}
