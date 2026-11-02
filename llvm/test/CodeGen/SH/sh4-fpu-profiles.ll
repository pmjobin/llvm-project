; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -target-abi=gnu -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=NOFPU
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -target-abi=gnu -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=NOFPU
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=+fpu-single-mode -target-abi=gnu-hard-float-single -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SINGLE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -mattr=+fpu-single-mode -target-abi=gnu-hard-float-single -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SINGLE
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode -target-abi=gnu-e-float -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SINGLE-ONLY
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode -target-abi=gnu-e-float -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SINGLE-ONLY

; NOFPU-LABEL: add_f32:
; NOFPU-NOT: fadd
; NOFPU: __addsf3
; SINGLE-LABEL: add_f32:
; SINGLE-NOT: fpscr
; SINGLE: fadd fr{{[0-9]+}},fr0
; SINGLE-NOT: fpscr
; SINGLE-ONLY-LABEL: add_f32:
; SINGLE-ONLY-NOT: fpscr
; SINGLE-ONLY: fadd fr5,fr0
; SINGLE-ONLY-NOT: fpscr

define float @add_f32(float %left, float %right) {
	%result = fadd float %left, %right
	ret float %result
}

; NOFPU-LABEL: add_f64:
; NOFPU-NOT: fadd
; NOFPU: __adddf3
; SINGLE-LABEL: add_f64:
; SINGLE: sts fpscr
; SINGLE: fadd dr6,dr0
; SINGLE: lds {{r[0-9]+}},fpscr
; SINGLE-ONLY-LABEL: add_f64:
; SINGLE-ONLY-NOT: fadd
; SINGLE-ONLY-NOT: {{[dx]}}r{{[0-9]+}}
; SINGLE-ONLY: __adddf3

define double @add_f64(double %left, double %right) {
	%result = fadd double %left, %right
	ret double %result
}

; NOFPU-LABEL: mixed_precision:
; NOFPU: __extendsfdf2
; NOFPU: __adddf3
; NOFPU: __truncdfsf2
; SINGLE-LABEL: mixed_precision:
; SINGLE: fadd dr6,dr0
; SINGLE: lds {{r[0-9]+}},fpscr
; SINGLE-ONLY-LABEL: mixed_precision:
; SINGLE-ONLY-NOT: fadd
; SINGLE-ONLY-NOT: {{[dx]}}r{{[0-9]+}}
; SINGLE-ONLY: __extendsfdf2
; SINGLE-ONLY: __adddf3
; SINGLE-ONLY: __truncdfsf2

define float @mixed_precision(float %single, double %wide) {
	%extended = fpext float %single to double
	%sum = fadd double %extended, %wide
	%result = fptrunc double %sum to float
	ret float %result
}

; NOFPU-LABEL: memory_f64:
; NOFPU-NOT: fmov
; NOFPU: mov.l
; SINGLE-ONLY-LABEL: memory_f64:
; SINGLE-ONLY-NOT: fmov
; SINGLE-ONLY-NOT: {{[dx]}}r{{[0-9]+}}
; SINGLE-ONLY: mov.l

define void @memory_f64(ptr %source, ptr %destination) {
	%value = load double, ptr %source, align 4
	store double %value, ptr %destination, align 4
	ret void
}

declare float @llvm.sqrt.f32(float)
declare double @llvm.sqrt.f64(double)

; NOFPU-LABEL: sqrt_f32:
; NOFPU-NOT: fsqrt
; NOFPU: sqrtf
; SINGLE-LABEL: sqrt_f32:
; SINGLE: fsqrt fr0
; SINGLE-ONLY-LABEL: sqrt_f32:
; SINGLE-ONLY: fsqrt fr0

define float @sqrt_f32(float %value) {
	%result = call float @llvm.sqrt.f32(float %value)
	ret float %result
}

; NOFPU-LABEL: sqrt_f64:
; NOFPU-NOT: fsqrt
; NOFPU: sqrt
; SINGLE-LABEL: sqrt_f64:
; SINGLE: fsqrt dr0
; SINGLE-ONLY-LABEL: sqrt_f64:
; SINGLE-ONLY-NOT: fsqrt
; SINGLE-ONLY-NOT: {{[dx]}}r{{[0-9]+}}
; SINGLE-ONLY: sqrtl

define double @sqrt_f64(double %value) {
	%result = call double @llvm.sqrt.f64(double %value)
	ret double %result
}

; NOFPU-LABEL: remainder_f32:
; NOFPU-NOT: fdiv
; NOFPU: fmodf
; SINGLE-ONLY-LABEL: remainder_f32:
; SINGLE-ONLY: fmodf

define float @remainder_f32(float %left, float %right) {
	%result = frem float %left, %right
	ret float %result
}

; NOFPU-LABEL: remainder_f64:
; NOFPU-NOT: fdiv
; NOFPU: fmod
; SINGLE-ONLY-LABEL: remainder_f64:
; SINGLE-ONLY-NOT: {{[dx]}}r{{[0-9]+}}
; SINGLE-ONLY: fmodl

define double @remainder_f64(double %left, double %right) {
	%result = frem double %left, %right
	ret double %result
}
