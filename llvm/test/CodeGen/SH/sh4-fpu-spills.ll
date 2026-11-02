; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=COMMON,LE
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs -filetype=obj < %s -o %t.le.o

declare void @clobber()
declare void @clobber_f64(double, double, double, double, double, double, double, double, double, double)
declare void @consume(ptr)

; Ten f64 values are simultaneously live across a call.  Every DR value is
; caller-clobbered, the spill slots have four-byte ABI alignment, and the two
; scalar transfers retain the target's memory word order while SZ is clear.

; COMMON-LABEL: pressure_f64:
; BE: fmov.s fr1,@-r{{[0-9]+}}
; BE-NEXT: fmov.s fr0,@-r{{[0-9]+}}
; LE: fmov.s fr0,@-r{{[0-9]+}}
; LE-NEXT: fmov.s fr1,@-r{{[0-9]+}}
; COMMON: jsr
; COMMON: sts fpscr,r0
; COMMON: lds r0,fpscr
; COMMON: fmov.s @r{{[0-9]+}}+,fr{{[0-9]+}}
; COMMON: fadd dr{{[0-9]+}},dr{{[0-9]+}}

define double @pressure_f64(ptr %p) {
	%v0 = load double, ptr %p, align 4
	%p1 = getelementptr double, ptr %p, i32 1
	%v1 = load double, ptr %p1, align 4
	%p2 = getelementptr double, ptr %p, i32 2
	%v2 = load double, ptr %p2, align 4
	%p3 = getelementptr double, ptr %p, i32 3
	%v3 = load double, ptr %p3, align 4
	%p4 = getelementptr double, ptr %p, i32 4
	%v4 = load double, ptr %p4, align 4
	%p5 = getelementptr double, ptr %p, i32 5
	%v5 = load double, ptr %p5, align 4
	%p6 = getelementptr double, ptr %p, i32 6
	%v6 = load double, ptr %p6, align 4
	%p7 = getelementptr double, ptr %p, i32 7
	%v7 = load double, ptr %p7, align 4
	%p8 = getelementptr double, ptr %p, i32 8
	%v8 = load double, ptr %p8, align 4
	%p9 = getelementptr double, ptr %p, i32 9
	%v9 = load double, ptr %p9, align 4
	%a0 = fadd double %v0, 1.000000e+00
	%a1 = fadd double %v1, 1.000000e+00
	%a2 = fadd double %v2, 1.000000e+00
	%a3 = fadd double %v3, 1.000000e+00
	%a4 = fadd double %v4, 1.000000e+00
	%a5 = fadd double %v5, 1.000000e+00
	%a6 = fadd double %v6, 1.000000e+00
	%a7 = fadd double %v7, 1.000000e+00
	%a8 = fadd double %v8, 1.000000e+00
	%a9 = fadd double %v9, 1.000000e+00
	call void @clobber_f64(double %a0, double %a1, double %a2, double %a3, double %a4, double %a5, double %a6, double %a7, double %a8, double %a9)
	%s1 = fadd double %a0, %a1
	%s2 = fadd double %a2, %a3
	%s3 = fadd double %a4, %a5
	%s4 = fadd double %a6, %a7
	%s5 = fadd double %a8, %a9
	%s6 = fadd double %s1, %s2
	%s7 = fadd double %s3, %s4
	%s8 = fadd double %s6, %s7
	%result = fadd double %s8, %s5
	ret double %result
}

; COMMON-LABEL: pressure_f32:
; COMMON: fmov.s fr{{[0-9]+}},@(r0,r15)
; COMMON: jsr
; COMMON: sts fpscr,r0
; COMMON: lds r0,fpscr
; COMMON: fmov.s @(r0,r15),fr{{[0-9]+}}
; COMMON: fadd fr{{[0-9]+}},fr{{[0-9]+}}

define float @pressure_f32(ptr %p) {
	%v0 = load float, ptr %p, align 4
	%p1 = getelementptr float, ptr %p, i32 1
	%v1 = load float, ptr %p1, align 4
	%p2 = getelementptr float, ptr %p, i32 2
	%v2 = load float, ptr %p2, align 4
	%p3 = getelementptr float, ptr %p, i32 3
	%v3 = load float, ptr %p3, align 4
	%p4 = getelementptr float, ptr %p, i32 4
	%v4 = load float, ptr %p4, align 4
	%p5 = getelementptr float, ptr %p, i32 5
	%v5 = load float, ptr %p5, align 4
	%p6 = getelementptr float, ptr %p, i32 6
	%v6 = load float, ptr %p6, align 4
	%p7 = getelementptr float, ptr %p, i32 7
	%v7 = load float, ptr %p7, align 4
	%p8 = getelementptr float, ptr %p, i32 8
	%v8 = load float, ptr %p8, align 4
	%p9 = getelementptr float, ptr %p, i32 9
	%v9 = load float, ptr %p9, align 4
	call void @clobber()
	%s1 = fadd float %v0, %v1
	%s2 = fadd float %v2, %v3
	%s3 = fadd float %v4, %v5
	%s4 = fadd float %v6, %v7
	%s5 = fadd float %v8, %v9
	%s6 = fadd float %s1, %s2
	%s7 = fadd float %s3, %s4
	%s8 = fadd float %s6, %s7
	%result = fadd float %s8, %s5
	ret float %result
}

; COMMON-LABEL: large_realign_f64:
; COMMON: mov r15,r14
; COMMON: fadd dr{{[0-9]+}},dr{{[0-9]+}}
; COMMON-NOT: fmov.d
; COMMON: jsr
; COMMON-NOT: fmov.d
; COMMON: fmul dr{{[0-9]+}},dr{{[0-9]+}}

define double @large_realign_f64(double %left, double %right) uwtable {
	%frame = alloca [131072 x i8], align 32
	%slot = getelementptr [131072 x i8], ptr %frame, i32 0, i32 131064
	%sum = fadd double %left, %right
	store volatile double %sum, ptr %slot, align 4
	call void @consume(ptr %frame)
	%loaded = load volatile double, ptr %slot, align 4
	%result = fmul double %loaded, %right
	ret double %result
}
