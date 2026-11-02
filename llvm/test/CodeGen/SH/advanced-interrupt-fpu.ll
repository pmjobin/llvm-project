; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs %t/sh4.ll -o - | FileCheck %s --check-prefix=SH4
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs %t/sh4.ll -o - | FileCheck %s --check-prefix=SH4
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2e -O2 -verify-machineinstrs %t/e.ll -o - | FileCheck %s --check-prefix=E
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh3e -O2 -verify-machineinstrs %t/e.ll -o - | FileCheck %s --check-prefix=E
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=+single-fpu,-double-fpu,-fpu-size-mode,-fpu-register-bank,+fpu-single-mode -O2 -verify-machineinstrs %t/e.ll -o - | FileCheck %s --check-prefix=E
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=+single-fpu,+double-fpu,+fpu-size-mode,+fpu-register-bank,+fpu-single-mode -O2 -verify-machineinstrs %t/single.ll -o - | FileCheck %s --check-prefix=SINGLE
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs %t/soft.ll -o - | FileCheck %s --check-prefix=SOFT
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -relocation-model=pic -O2 -verify-machineinstrs %t/pic.ll -o - | FileCheck %s --check-prefix=PIC
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs -filetype=obj %t/cfi.ll -o %t.be.o
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs -filetype=obj %t/cfi.ll -o %t.le.o
; RUN: llvm-dwarfdump --verify %t.be.o %t.le.o

; SH4-LABEL: interrupt_empty:
; SH4-NOT: fpscr
; SH4-NOT: fpul
; SH4: rte
; SH4-NEXT: nop

; SH4-LABEL: interrupt_leaf:
; SH4: sts.l fpscr,@-r15
; SH4: .cfi_offset fpscr, -{{[0-9]+}}
; SH4: sts.l fpul,@-r15
; SH4: .cfi_offset fpul, -{{[0-9]+}}
; SH4: flds fr{{[0-9]+}},fpul
; SH4: sts.l fpul,@-r15
; SH4: lds.l @r0+,fpscr
; SH4: lds.l @r0+,fpul
; SH4-NEXT: fsts fpul,fr{{[0-9]+}}
; SH4: lds.l @r0+,fpul
; SH4: rte
; SH4-NEXT: nop

; SH4-LABEL: interrupt_high_fr:
; SH4-NOT: fpscr
; SH4: sts.l fpul,@-r15
; SH4: flds fr14,fpul
; SH4: sts.l fpul,@-r15
; SH4: .cfi_offset fr14, -{{[0-9]+}}
; SH4: lds.l @r0+,fpul
; SH4: fsts fpul,fr14
; SH4: lds.l @r0+,fpul
; SH4: rte
; SH4-NEXT: nop

; SH4-LABEL: interrupt_double_leaf:
; SH4: sts.l fpscr,@-r15
; SH4: sts.l fpul,@-r15
; SH4-COUNT-2: flds fr{{[0-9]+}},fpul
; SH4-COUNT-2: sts.l fpul,@-r15
; SH4: lds.l @r0+,fpscr
; SH4-COUNT-2: fsts fpul,fr{{[0-9]+}}
; SH4: lds.l @r0+,fpul
; SH4: rte
; SH4-NEXT: nop

; SH4-LABEL: interrupt_call:
; SH4: sts.l fpscr,@-r15
; SH4: sts.l fpul,@-r15
; SH4-COUNT-12: sts.l fpul,@-r15
; SH4: jsr
; SH4: lds.l @r0+,fpscr
; SH4-COUNT-12: fsts fpul,fr{{[0-9]+}}
; SH4: lds.l @r0+,fpul
; SH4: rte
; SH4-NEXT: nop

; SH4-LABEL: interrupt_large_fpu:
; SH4: sts.l fpscr,@-r15
; SH4: sts.l fpul,@-r15
; SH4: add #-128,r15
; SH4: jsr
; SH4: lds.l @r0+,fpscr
; SH4: rte
; SH4-NEXT: nop

; SH4-LABEL: trapa_float:
; SH4: sts.l fpscr,@-r15
; SH4-NOT: sts.l fpul,@-r15
; SH4-NOT: flds fr{{[0-9]+}},fpul
; SH4: lds.l @r0+,fpscr
; SH4: rte
; SH4-NEXT: nop

; SH4-LABEL: interrupt_resbank:
; SH4-NOT: sts.l pr,@-r15
; SH4-NOT: sts.l mach,@-r15
; SH4-NOT: sts.l macl,@-r15
; SH4: sts.l fpscr,@-r15
; SH4: sts.l fpul,@-r15
; SH4: resbank
; SH4-NEXT: rte
; SH4-NEXT: nop

; SH4-LABEL: interrupt_switched:
; SH4: mov.l r0,@-r15
; SH4: sts.l fpscr,@-r15
; SH4: sts.l fpul,@-r15
; SH4: jsr
; SH4: lds.l @r0+,fpscr
; SH4: mov.l @r15,r15
; SH4-NEXT: mov.l @r15+,r0
; SH4-NEXT: rte
; SH4-NEXT: nop

; SH4-LABEL: interrupt_trap_exit:
; SH4: sts.l fpscr,@-r15
; SH4: sts.l fpul,@-r15
; SH4: jsr
; SH4: lds.l @r0+,fpscr
; SH4-NOT: rte
; SH4: trapa #7

; SH4-LABEL: interrupt_nosave:
; SH4-NOT: mov.l r0,
; SH4-NOT: mov.l r1,
; SH4-NOT: mov.l r2,
; SH4-NOT: mov.l r3,
; SH4-NOT: mov.l r4,
; SH4-NOT: mov.l r5,
; SH4-NOT: mov.l r6,
; SH4-NOT: mov.l r7,
; SH4: sts.l fpscr,@-r15
; SH4: sts.l fpul,@-r15
; SH4: jsr
; SH4: lds.l @r0+,fpscr
; SH4: rte
; SH4-NEXT: nop

; SH4-LABEL: interrupt_resbank_switched:
; SH4: mov.l r0,@-r15
; SH4: mov r0,r15
; SH4: sts.l fpscr,@-r15
; SH4: sts.l fpul,@-r15
; SH4: jsr
; SH4: lds.l @r0+,fpscr
; SH4: mov.l @r15,r15
; SH4-NEXT: mov.l @r15+,r0
; SH4-NEXT: resbank
; SH4-NEXT: rte
; SH4-NEXT: nop

; SH4-LABEL: interrupt_nosave_trap_exit:
; SH4-NOT: mov.l r{{[0-7]}},{{.*}}Folded Spill
; SH4: sts.l fpscr,@-r15
; SH4: sts.l fpul,@-r15
; SH4: jsr
; SH4: lds.l @r0+,fpscr
; SH4-NOT: rte
; SH4: trapa #8

; E-LABEL: interrupt_profile_call:
; Calls may alter arithmetic cause/sticky flags even without a mode change.
; E: sts.l fpscr,@-r15
; E: sts.l fpul,@-r15
; E-COUNT-12: sts.l fpul,@-r15
; E: jsr
; E: lds.l @r0+,fpscr
; E-COUNT-12: fsts fpul,fr{{[0-9]+}}
; E: lds.l @r0+,fpul
; E: rte
; E-NEXT: nop

; SINGLE-LABEL: interrupt_profile_call:
; SINGLE: sts.l fpscr,@-r15
; SINGLE: sts.l fpul,@-r15
; SINGLE-COUNT-12: sts.l fpul,@-r15
; SINGLE: jsr
; SINGLE: lds.l @r0+,fpscr
; SINGLE-COUNT-12: fsts fpul,fr{{[0-9]+}}
; SINGLE: lds.l @r0+,fpul
; SINGLE: rte
; SINGLE-NEXT: nop

; SOFT-LABEL: interrupt_soft:
; SOFT-NOT: fpscr
; SOFT-NOT: fpul
; SOFT: jsr
; SOFT: rte
; SOFT-NEXT: nop

; PIC-LABEL: interrupt_call:
; PIC: sts.l fpscr,@-r15
; PIC: sts.l fpul,@-r15
; PIC: mov.l r12,
; PIC: jsr
; PIC: lds.l @r0+,fpscr
; PIC: mov.l {{.*}},r12
; PIC: rte
; PIC-NEXT: nop

;--- sh4.ll
@value = global float 1.000000e+00, align 4
@double_value = global double 1.000000e+00, align 8
declare float @float_helper(float)
declare void @sink(ptr)

define void @interrupt_empty() #0 {
	ret void
}

define void @interrupt_leaf() #0 {
	%value = load volatile float, ptr @value, align 4
	%result = fadd float %value, 1.000000e+00
	store volatile float %result, ptr @value, align 4
	ret void
}

define void @interrupt_high_fr() #0 {
	call void asm sideeffect "", "~{fr14}"()
	ret void
}

define void @interrupt_double_leaf() #0 {
	%value = load volatile double, ptr @double_value, align 8
	%result = fadd double %value, 1.000000e+00
	store volatile double %result, ptr @double_value, align 8
	ret void
}

define void @interrupt_call() #0 {
	%value = load volatile float, ptr @value, align 4
	%result = call float @float_helper(float %value)
	store volatile float %result, ptr @value, align 4
	ret void
}

define void @interrupt_large_fpu() #0 {
	%storage = alloca [512 x i8], align 4
	call void @sink(ptr %storage)
	%value = load volatile float, ptr @value, align 4
	%result = fadd float %value, 1.000000e+00
	store volatile float %result, ptr @value, align 4
	ret void
}

define void @trapa_float() #1 {
	%value = load volatile float, ptr @value, align 4
	%result = fadd float %value, 1.000000e+00
	store volatile float %result, ptr @value, align 4
	ret void
}

define void @interrupt_resbank() #2 {
	%value = load volatile float, ptr @value, align 4
	%result = call float @float_helper(float %value)
	store volatile float %result, ptr @value, align 4
	ret void
}

define void @interrupt_switched() #3 {
	%value = load volatile float, ptr @value, align 4
	%result = call float @float_helper(float %value)
	store volatile float %result, ptr @value, align 4
	ret void
}

define void @interrupt_trap_exit() #4 {
	%value = load volatile float, ptr @value, align 4
	%result = call float @float_helper(float %value)
	store volatile float %result, ptr @value, align 4
	ret void
}

define void @interrupt_nosave() #5 {
	%value = load volatile float, ptr @value, align 4
	%result = call float @float_helper(float %value)
	store volatile float %result, ptr @value, align 4
	ret void
}

define void @interrupt_resbank_switched() #6 {
	%value = load volatile float, ptr @value, align 4
	%result = call float @float_helper(float %value)
	store volatile float %result, ptr @value, align 4
	ret void
}

define void @interrupt_nosave_trap_exit() #7 {
	%value = load volatile float, ptr @value, align 4
	%result = call float @float_helper(float %value)
	store volatile float %result, ptr @value, align 4
	ret void
}

attributes #0 = { noinline nounwind uwtable "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "target-abi"="gnu-hard-float" }
attributes #1 = { noinline nounwind uwtable "disable-tail-calls"="true" "sh-interrupt-handler"="trapa" "target-abi"="gnu-hard-float" }
attributes #2 = { noinline nounwind uwtable "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-resbank" "target-abi"="gnu-hard-float" "target-cpu"="sh2a" }
attributes #3 = { noinline nounwind uwtable "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-sp-switch"="alternate_stack" "target-abi"="gnu-hard-float" }
attributes #4 = { noinline nounwind uwtable "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-trap-exit"="7" "target-abi"="gnu-hard-float" }
attributes #5 = { noinline nounwind uwtable "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-nosave-low-regs" "target-abi"="gnu-hard-float" }
attributes #6 = { noinline nounwind uwtable "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-resbank" "sh-sp-switch"="alternate_stack" "target-abi"="gnu-hard-float" "target-cpu"="sh2a" }
attributes #7 = { noinline nounwind uwtable "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-nosave-low-regs" "sh-trap-exit"="8" "target-abi"="gnu-hard-float" }

;--- pic.ll
@pic_value = external global float, align 4
declare float @pic_helper(float)
define void @interrupt_call() #0 {
	%value = load volatile float, ptr @pic_value, align 4
	%result = call float @pic_helper(float %value)
	store volatile float %result, ptr @pic_value, align 4
	ret void
}
attributes #0 = { noinline nounwind uwtable "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "target-abi"="gnu-hard-float" }

;--- cfi.ll
declare void @cfi_helper()
define void @cfi_interrupt_call() #0 {
	call void @cfi_helper()
	ret void
}
attributes #0 = { noinline nounwind uwtable "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "target-abi"="gnu-hard-float" }

;--- e.ll
declare void @profile_helper()
define void @interrupt_profile_call() #0 {
	call void @profile_helper()
	ret void
}
attributes #0 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "target-abi"="gnu-e-float" }

;--- single.ll
declare void @profile_helper()
define void @interrupt_profile_call() #0 {
	call void @profile_helper()
	ret void
}
attributes #0 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "target-abi"="gnu-hard-float-single" }

;--- soft.ll
@soft_value = global float 1.000000e+00, align 4
define void @interrupt_soft() #0 {
	%value = load volatile float, ptr @soft_value, align 4
	%result = fadd float %value, 1.000000e+00
	store volatile float %result, ptr @soft_value, align 4
	ret void
}
attributes #0 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "target-abi"="gnu" "use-soft-float"="true" }
