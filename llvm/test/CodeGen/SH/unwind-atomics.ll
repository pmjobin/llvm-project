; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llvm-dwarfdump --eh-frame %t.be.o 2>&1 | FileCheck %s --check-prefix=FRAME
; RUN: llvm-dwarfdump --eh-frame %t.be.o | %python %S/Inputs/sh-unwind-model.py --scenarios=tas-leaf,singlethread-fence,extended-atomic | FileCheck %s --check-prefix=MODEL
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llvm-dwarfdump --eh-frame %t.le.o 2>&1 | FileCheck %s --check-prefix=FRAME
; RUN: llvm-dwarfdump --eh-frame %t.le.o | %python %S/Inputs/sh-unwind-model.py --scenarios=tas-leaf,singlethread-fence,extended-atomic | FileCheck %s --check-prefix=MODEL

declare i32 @llvm.sh.tas.b(ptr)

; ASM-LABEL: tas_leaf:
; ASM-NEXT: .cfi_startproc
; ASM-NOT: sts.l
; ASM: tas.b @r4
; ASM-NEXT: movt r0
; ASM-NOT: .cfi_offset pr
; ASM: rts
define i32 @tas_leaf(ptr %p) nounwind uwtable {
  %result = call i32 @llvm.sh.tas.b(ptr %p)
  ret i32 %result
}

; ASM-LABEL: singlethread_fence:
; ASM-NEXT: .cfi_startproc
; ASM-NOT: sts.l
; ASM-NOT: jsr
; ASM: rts
; ASM-NOT: .cfi_offset pr
define void @singlethread_fence(ptr %p) nounwind uwtable {
  store i32 1, ptr %p, align 4
  fence syncscope("singlethread") seq_cst
  ret void
}

; ASM-LABEL: extended_atomic_frame:
; ASM-NEXT: .cfi_startproc
; ASM-NEXT: sts.l pr,@-r15
; ASM-NEXT: .cfi_def_cfa_offset 4
; ASM-NEXT: .cfi_offset pr, -4
; ASM-NEXT: add #-68,r15
; ASM-NEXT: .cfi_def_cfa_offset 72
; ASM: mov.l r1,@(60,r15)
; ASM-NEXT: add #4,r15
; ASM-NEXT: .cfi_adjust_cfa_offset -4
; ASM-NEXT: mov.l r0,@(60,r15)
; ASM-NEXT: add #-4,r15
; ASM-NEXT: .cfi_adjust_cfa_offset 4
; ASM-NEXT: add #-8,r15
; ASM-NEXT: .cfi_adjust_cfa_offset 8
; ASM: jsr
; ASM-NEXT: nop
; ASM-NEXT: add #8,r15
; ASM-NEXT: .cfi_adjust_cfa_offset -8
; ASM: add #68,r15
; ASM-NEXT: .cfi_def_cfa_offset 4
; ASM-NEXT: lds.l @r15+,pr
; ASM-NEXT: .cfi_restore pr
; ASM-NEXT: .cfi_def_cfa_offset 0
define i64 @extended_atomic_frame(ptr %p, i64 %limit) uwtable {
  %old = atomicrmw uinc_wrap ptr %p, i64 %limit seq_cst, align 8
  ret i64 %old
}

; FRAME: .eh_frame contents:
; FRAME: Return address column: 17
; FRAME-NOT: warning:
; FRAME: FDE cie=
; FRAME: 0x0: CFA=R15
; FRAME: FDE cie=
; FRAME: 0x0: CFA=R15
; FRAME: FDE cie=
; FRAME: CFA=R15+72: PR=[CFA-4]
; FRAME: CFA=R15+68: PR=[CFA-4]
; FRAME: CFA=R15+72: PR=[CFA-4]
; FRAME: CFA=R15+80: PR=[CFA-4]
; FRAME: CFA=R15

; MODEL: scenarios: 3
; MODEL: rows: {{[0-9]+}}
; MODEL: recoveries: {{[0-9]+}}
; MODEL: caller CFA, PR, r8-r14, and SP recovered
