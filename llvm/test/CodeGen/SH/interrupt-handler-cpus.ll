; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh1 -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2e -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2a -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh3 -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh3e -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4a -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4al-dsp -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=+st40-300 -O2 -verify-machineinstrs %s -o - | FileCheck %s

define void @interrupt_integer_leaf() #0 {
	call void asm sideeffect "", "~{r0}"()
	ret void
}

define void @trapa_integer_leaf() #1 {
	call void asm sideeffect "", "~{r8}"()
	ret void
}

attributes #0 = { noinline nounwind "sh-interrupt-handler"="interrupt" "target-abi"="gnu" }
attributes #1 = { noinline nounwind "sh-interrupt-handler"="trapa" "target-abi"="gnu" }

; CHECK-LABEL: interrupt_integer_leaf:
; CHECK: mov.l r0,
; CHECK: mov.l {{.*}},r0
; CHECK: rte
; CHECK-NEXT: nop

; CHECK-LABEL: trapa_integer_leaf:
; CHECK: mov.l r8,
; CHECK: mov.l {{.*}},r8
; CHECK: rte
; CHECK-NEXT: nop
