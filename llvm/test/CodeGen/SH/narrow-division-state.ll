; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh4 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM

; A comparison value crosses an inline-assembly T clobber and the division.
; Every DIV1 must retain its implicit state dependencies through scheduling.
define i32 @condition_across_division(i32 %n, i16 %input, i32 %x, i32 %y) {
; MIR-LABEL: name: condition_across_division
; MIR: INLINEASM
; MIR: DIV0U implicit-def $mbit, implicit-def $qbit, implicit-def $tbit
; MIR-COUNT-16: DIV1 {{.*}}implicit-def $qbit, implicit-def $tbit, implicit $mbit, implicit $qbit, implicit $tbit
; MIR: XTRCT
; MIR: SWAP_W
; MIR-COUNT-16: DIV1 {{.*}}implicit-def $qbit, implicit-def $tbit, implicit $mbit, implicit $qbit, implicit $tbit
; MIR: ROTCL {{.*}}implicit-def $tbit, implicit $tbit
; ASM-LABEL: condition_across_division:
; ASM: div0u
; ASM: xtrct
; ASM: rotcl
	%c = icmp ult i32 %x, %y
	%ready = call i32 asm sideeffect "", "=r,0,~{t}"(i32 %n)
	%extended = zext i16 %input to i32
	%d = or i32 %extended, 1
	%q = udiv i32 %ready, %d
	%adjustment = select i1 %c, i32 %x, i32 %y
	%result = add i32 %q, %adjustment
	ret i32 %result
}

@numerator = external global i32
@denominator = external global i16
@quotient = external global i32

; The ordinary hardware handler still restores the interrupted SR with RTE.
define void @interrupt_division() #0 {
; MIR-LABEL: name: interrupt_division
; MIR: DIV0U implicit-def $mbit, implicit-def $qbit, implicit-def $tbit
; MIR: RTE_RETURN
; ASM-LABEL: interrupt_division:
; ASM: div0u
; ASM: xtrct
; ASM: rotcl
; ASM: rte
	%n = load volatile i32, ptr @numerator
	%input = load volatile i16, ptr @denominator
	%extended = zext i16 %input to i32
	%d = or i32 %extended, 1
	%q = udiv i32 %n, %d
	store volatile i32 %q, ptr @quotient
	ret void
}

attributes #0 = { noinline nounwind "sh-interrupt-handler"="interrupt" "target-abi"="gnu" }
