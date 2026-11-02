; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2e -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=COMMON,SH2E,BE --implicit-check-not='fadd dr' --implicit-check-not='fmov.d' --implicit-check-not=frchg --implicit-check-not=fschg
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2e -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=COMMON,SH2E,LE --implicit-check-not='fadd dr' --implicit-check-not='fmov.d' --implicit-check-not=frchg --implicit-check-not=fschg
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh3e -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=COMMON,SH3E,BE --implicit-check-not='fadd dr' --implicit-check-not='fmov.d' --implicit-check-not=frchg --implicit-check-not=fschg
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3e -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=COMMON,SH3E,LE --implicit-check-not='fadd dr' --implicit-check-not='fmov.d' --implicit-check-not=frchg --implicit-check-not=fschg
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2e -O2 -filetype=obj < %s -o %t.sh2e.o
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3e -O2 -filetype=obj < %s -o %t.sh3e.o
; RUN: llvm-readobj --file-headers %t.sh2e.o | FileCheck %s --check-prefix=SH2E-FLAGS
; RUN: llvm-readobj --file-headers %t.sh3e.o | FileCheck %s --check-prefix=SH3E-FLAGS

; SH2E-FLAGS: Flags [ (0xB)
; SH3E-FLAGS: Flags [ (0x8)

declare float @llvm.sqrt.f32(float)
declare float @external_f32(float)

define float @f32_arithmetic(float %left, float %right) {
; COMMON-LABEL: f32_arithmetic:
; COMMON-DAG: fadd
; COMMON-DAG: fsub
; COMMON-DAG: fmul
; COMMON-DAG: fdiv
	%add = fadd float %left, %right
	%sub = fsub float %left, %right
	%mul = fmul float %add, %sub
	%result = fdiv float %mul, %right
	ret float %result
}

define i1 @f32_compare(float %left, float %right) {
; COMMON-LABEL: f32_compare:
; COMMON: fcmp/gt
	%result = fcmp ogt float %left, %right
	ret i1 %result
}

define float @f32_sqrt(float %value) {
; COMMON-LABEL: f32_sqrt:
; SH2E: .long sqrtf
; SH2E-NOT: fsqrt
; SH3E: fsqrt
	%result = call float @llvm.sqrt.f32(float %value)
	ret float %result
}

define double @f64_arithmetic(double %left, double %right) {
; COMMON-LABEL: f64_arithmetic:
; COMMON: .long __adddf3
	%result = fadd double %left, %right
	ret double %result
}

define float @signed_to_f32(i32 %value) {
; COMMON-LABEL: signed_to_f32:
; COMMON: lds {{r[0-9]+}},fpul
; COMMON: float fpul,fr0
	%result = sitofp i32 %value to float
	ret float %result
}

define i32 @f32_to_signed(float %value) {
; COMMON-LABEL: f32_to_signed:
; COMMON: ftrc {{fr[0-9]+}},fpul
; COMMON: sts fpul,r0
	%result = fptosi float %value to i32
	ret i32 %result
}

define float @indirect_f32_call(ptr %function, float %value) {
; COMMON-LABEL: indirect_f32_call:
; COMMON: jsr @r{{[0-9]+}}
	%result = call float %function(float %value)
	ret float %result
}

define float @direct_f32_call(float %value) {
; COMMON-LABEL: direct_f32_call:
; COMMON: .long external_f32
	%result = call float @external_f32(float %value)
	ret float %result
}

declare void @f64_boundary_callee(i32, i32, i32, double, i32)

define void @call_f64_boundary() {
; COMMON-LABEL: call_f64_boundary:
; COMMON: mov #9,[[TAIL:r[0-9]+]]
; COMMON: mov.l [[TAIL]],@(8,r15)
; COMMON-NOT: mov #9,r7
; COMMON: jsr
	call void @f64_boundary_callee(i32 1, i32 2, i32 3, double 2.25, i32 9)
	ret void
}

define double @take_f64_boundary(i32 %first, i32 %second, i32 %third, double %wide, i32 %tail) {
; COMMON-LABEL: take_f64_boundary:
; COMMON-NOT: mov r7
; COMMON: mov.l @(4,r15),r1
; COMMON: rts
; COMMON-NEXT: mov.l @r15,r0
	ret double %wide
}

declare void @i64_boundary_callee(i32, i32, i32, i64, i32)

define void @call_i64_boundary() {
; COMMON-LABEL: call_i64_boundary:
; COMMON: add #-12,r15
; COMMON-DAG: mov.l [[W1POOL:.LCPI[0-9_]+]],[[W1:r[0-9]+]]
; COMMON-DAG: mov.l [[W0POOL:.LCPI[0-9_]+]],[[W0:r[0-9]+]]
; COMMON: mov.l [[W1]],@(4,r15)
; COMMON: mov.l [[W0]],@r15
; COMMON: mov #9,[[I64TAIL:r[0-9]+]]
; COMMON-NOT: mov #9,r7
; COMMON: jsr
; COMMON-NEXT: mov.l [[I64TAIL]],@(8,r15)
; COMMON: [[W0POOL]]:
; BE-NEXT: .long 287454020
; LE-NEXT: .long 1432778632
; COMMON: [[W1POOL]]:
; BE-NEXT: .long 1432778632
; LE-NEXT: .long 287454020
	call void @i64_boundary_callee(i32 1, i32 2, i32 3, i64 1234605616436508552, i32 9)
	ret void
}

define i64 @take_i64_boundary(i32 %first, i32 %second, i32 %third, i64 %wide, i32 %tail) {
; COMMON-LABEL: take_i64_boundary:
; COMMON-NOT: mov r7
; COMMON: mov.l @(4,r15),r1
; COMMON: rts
; COMMON-NEXT: mov.l @r15,r0
	ret i64 %wide
}
