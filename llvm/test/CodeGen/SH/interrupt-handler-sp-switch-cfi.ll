; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -relocation-model=static -O2 -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -relocation-model=static -O2 -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llvm-dwarfdump --verify %t.be.o %t.le.o
; RUN: llvm-dwarfdump --eh-frame %t.be.o | FileCheck %s

declare void @helper()

define void @switched_cfi() #0 {
	call void @helper()
	ret void
}

attributes #0 = { noinline nounwind uwtable "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-sp-switch"="alternate_stack" "target-abi"="gnu" }

; CHECK: DW_CFA_def_cfa_offset
; CHECK: DW_CFA_offset: PR
; CHECK: DW_CFA_restore: PR
