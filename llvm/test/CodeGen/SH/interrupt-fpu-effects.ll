; Hardware effects are independent of floating argument transport.
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2e -target-abi=gnu -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,SOFT
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2e -target-abi=gnu-e-float -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,HARD
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2e -target-abi=gnu -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,SOFT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2e -target-abi=gnu-e-float -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,HARD
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh3e -target-abi=gnu -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,SOFT
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh3e -target-abi=gnu-e-float -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,HARD
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3e -target-abi=gnu -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,SOFT
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3e -target-abi=gnu-e-float -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,HARD
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -target-abi=gnu -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,SOFT,DOUBLE
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,HARD,DOUBLE
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float-single -mattr=+fpu-single-mode -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,HARD,DOUBLE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,SOFT,DOUBLE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,HARD,DOUBLE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float-single -mattr=+fpu-single-mode -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,HARD,DOUBLE
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4a -target-abi=gnu -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,SOFT,DOUBLE
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4a -target-abi=gnu-hard-float -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,HARD,DOUBLE
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4a -target-abi=gnu-hard-float-single -mattr=+fpu-single-mode -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,HARD,DOUBLE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4a -target-abi=gnu -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,SOFT,DOUBLE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4a -target-abi=gnu-hard-float -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,HARD,DOUBLE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4a -target-abi=gnu-hard-float-single -mattr=+fpu-single-mode -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,HARD,DOUBLE

@value = global float 1.0, align 4
@integer = global i32 1, align 4
@shift = global i32 2, align 4

define void @empty() #0 {
	ret void
}
; CHECK-LABEL: empty:
; CHECK-NOT: fpscr
; CHECK-NOT: fpul
; CHECK-NOT: flds
; CHECK: rte

define void @integer_leaf() #0 {
	%v = load volatile i32, ptr @integer
	%r = add i32 %v, 1
	store volatile i32 %r, ptr @integer
	ret void
}
; CHECK-LABEL: integer_leaf:
; CHECK-NOT: fpscr
; CHECK-NOT: fpul
; CHECK-NOT: flds
; CHECK: rte

define void @leaf_divide() #0 {
	%v = load volatile float, ptr @value
	%r = fdiv float %v, 0.0
	store volatile float %r, ptr @value
	ret void
}
; CHECK-LABEL: leaf_divide:
; CHECK: sts.l fpscr,@-r15
; CHECK: fdiv
; CHECK: lds.l @r0+,fpscr
; CHECK: rte

define void @leaf_add() #0 {
	%v = load volatile float, ptr @value
	%r = fadd float %v, 1.0
	store volatile float %r, ptr @value
	ret void
}
; CHECK-LABEL: leaf_add:
; CHECK: sts.l fpscr,@-r15
; CHECK: fadd
; CHECK: lds.l @r0+,fpscr
; CHECK: rte

declare float @llvm.fabs.f32(float)
define void @leaf_abs() #0 {
	%v = load volatile float, ptr @value
	%r = call float @llvm.fabs.f32(float %v)
	store volatile float %r, ptr @value
	ret void
}
; CHECK-LABEL: leaf_abs:
; CHECK: sts.l fpscr,@-r15
; CHECK: fabs
; CHECK: lds.l @r0+,fpscr
; CHECK: rte

define void @leaf_neg() #0 {
	%v = load volatile float, ptr @value
	%r = fneg float %v
	store volatile float %r, ptr @value
	ret void
}
; CHECK-LABEL: leaf_neg:
; CHECK: sts.l fpscr,@-r15
; CHECK: fneg
; CHECK: lds.l @r0+,fpscr
; CHECK: rte

define void @leaf_fpul() #0 {
	call void asm sideeffect "", "~{fpul}"()
	ret void
}
; CHECK-LABEL: leaf_fpul:
; CHECK-NOT: fpscr
; CHECK: sts.l fpul,@-r15
; CHECK-NOT: fpscr
; CHECK: lds.l @r0+,fpul
; CHECK-NOT: fpscr
; CHECK: rte

define void @leaf_fr() #0 {
	call void asm sideeffect "", "~{fr14}"()
	ret void
}
; CHECK-LABEL: leaf_fr:
; CHECK-NOT: fpscr
; CHECK: sts.l fpul,@-r15
; CHECK: flds fr14,fpul
; CHECK: sts.l fpul,@-r15
; CHECK-NOT: fpscr
; CHECK: fsts fpul,fr14
; CHECK: lds.l @r0+,fpul
; CHECK-NOT: fpscr
; CHECK: rte

; DR aliases require a double-FPU profile; E-class has only scalar FR.
define void @leaf_dr_alias() #0 {
	call void asm sideeffect "", "~{dr2}"()
	ret void
}
; CHECK-LABEL: leaf_dr_alias:
; DOUBLE: flds fr2,fpul
; DOUBLE: flds fr3,fpul
; DOUBLE: fsts fpul,fr3
; DOUBLE: fsts fpul,fr2
; CHECK: rte

define void @leaf_fpscr() #0 {
	call void asm sideeffect "", "~{fpscr}"()
	ret void
}
; CHECK-LABEL: leaf_fpscr:
; CHECK: sts.l fpscr,@-r15
; CHECK: lds.l @r0+,fpscr
; CHECK: rte

; The known shift libcall mask preserves FR, FPUL and FPSCR. On CPUs with
; dynamic shifts this remains a leaf. Neither case needs FPU preservation.
define void @shift_handler() #0 {
	%v = load volatile i32, ptr @integer
	%n = load volatile i32, ptr @shift
	%r = shl i32 %v, %n
	store volatile i32 %r, ptr @integer
	ret void
}
; CHECK-LABEL: shift_handler:
; CHECK-NOT: fpscr
; CHECK-NOT: fpul
; CHECK-NOT: flds
; CHECK: rte

; This same-module noinline helper modifies FR, FPUL and FPSCR. The handler
; must use the call mask even if it has no local FP instruction.
define void @fp_helper() #1 {
	%v = load volatile float, ptr @value
	%r = fdiv float %v, 0.0
	store volatile float %r, ptr @value
	call void asm sideeffect "lds $0,fpul", "r,~{fpul}"(i32 42)
	ret void
}

define void @handler_call() #0 {
	call void @fp_helper()
	ret void
}
; CHECK-LABEL: handler_call:
; CHECK: sts.l fpscr,@-r15
; CHECK: sts.l fpul,@-r15
; SOFT-COUNT-16: flds fr{{[0-9]+}},fpul
; HARD-COUNT-12: flds fr{{[0-9]+}},fpul
; CHECK: jsr
; CHECK: lds.l @r0+,fpscr
; SOFT-COUNT-16: fsts fpul,fr{{[0-9]+}}
; HARD-COUNT-12: fsts fpul,fr{{[0-9]+}}
; CHECK: lds.l @r0+,fpul
; CHECK: rte

define void @helper_wrapper() #1 {
	call void @fp_helper()
	ret void
}

define void @handler_transitive_call() #0 {
	call void @helper_wrapper()
	ret void
}
; CHECK-LABEL: handler_transitive_call:
; CHECK: sts.l fpscr,@-r15
; CHECK: sts.l fpul,@-r15
; SOFT-COUNT-16: flds fr{{[0-9]+}},fpul
; HARD-COUNT-12: flds fr{{[0-9]+}},fpul
; CHECK: jsr
; CHECK: lds.l @r0+,fpscr
; SOFT-COUNT-16: fsts fpul,fr{{[0-9]+}}
; HARD-COUNT-12: fsts fpul,fr{{[0-9]+}}
; CHECK: lds.l @r0+,fpul
; CHECK: rte

attributes #0 = { noinline nounwind optsize "sh-interrupt-handler"="interrupt" "disable-tail-calls"="true" }
attributes #1 = { noinline nounwind }
