; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -enable-tail-merge=false -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -enable-tail-merge=false -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -enable-tail-merge=false -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llvm-dwarfdump --eh-frame %t.be.o 2>&1 | FileCheck %s --check-prefix=FRAME
; RUN: llvm-dwarfdump --eh-frame %t.be.o | %python %S/Inputs/sh-unwind-model.py --scenarios=multiple-epilogues | FileCheck %s --check-prefix=MODEL
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -enable-tail-merge=false -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llvm-dwarfdump --eh-frame %t.le.o 2>&1 | FileCheck %s --check-prefix=FRAME
; RUN: llvm-dwarfdump --eh-frame %t.le.o | %python %S/Inputs/sh-unwind-model.py --scenarios=multiple-epilogues | FileCheck %s --check-prefix=MODEL

declare i32 @callee(i32)

; ASM-LABEL: multiple:
; ASM: .cfi_offset pr, -4
; ASM: .cfi_def_cfa_offset 8
; ASM-NEXT: .cfi_remember_state
; ASM: .LBB0_2:
; ASM: .cfi_def_cfa_offset 4
; ASM: .cfi_restore pr
; ASM-NEXT: .cfi_def_cfa_offset 0
; ASM: .LBB0_3:
; ASM-NEXT: .cfi_restore_state
; ASM-NEXT: .cfi_remember_state
; ASM: .cfi_restore pr
; ASM-NEXT: .cfi_def_cfa_offset 0
; ASM: .LBB0_4:
; ASM-NEXT: .cfi_restore_state
; ASM: .cfi_restore pr
; ASM-NEXT: .cfi_def_cfa_offset 0
; ASM: .LCPI0_0_0:
; ASM-NEXT: .long callee
; ASM-NEXT: .Lfunc_end0:
; ASM-NEXT: .size multiple, .Lfunc_end0-multiple
; ASM-NEXT: .cfi_endproc
define i32 @multiple(i32 %value) uwtable {
entry:
  %called = call i32 @callee(i32 %value)
  switch i32 %called, label %third [
    i32 0, label %first
    i32 1, label %second
  ]

first:
  ret i32 11

second:
  ret i32 22

third:
  ret i32 33
}

; FRAME: FDE cie=
; FRAME-NOT: warning:
; FRAME: DW_CFA_offset: PR -4
; FRAME: DW_CFA_def_cfa_offset: +8
; FRAME: DW_CFA_remember_state
; FRAME: DW_CFA_restore_state
; FRAME: DW_CFA_remember_state
; FRAME: DW_CFA_restore_state
; FRAME: 0x2: CFA=R15+4: PR=[CFA-4]
; FRAME: 0x4: CFA=R15+8: PR=[CFA-4]
; FRAME: CFA=R15

; MODEL: scenarios: 1
; MODEL: rows: {{[0-9]+}}
; MODEL: recoveries: {{[0-9]+}}
; MODEL: caller CFA, PR, r8-r14, and SP recovered
