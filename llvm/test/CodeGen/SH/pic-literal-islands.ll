; RUN: %python %S/Inputs/generate-pic-literal-islands.py > %t.ll
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs < %t.ll | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs < %t.ll | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -stop-after=sh-literal-islands < %t.ll | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %t.ll -o %t.le.o
; RUN: llvm-readobj -r %t.le.o | FileCheck %s --check-prefix=OBJECT

; ASM-LABEL: pic_literal_clones:
; ASM: mova	[[GOTPC:.LCPI[0-9_]+]],r0
; ASM-NEXT: mov.l	[[GOTPC]],r12
; ASM: mova	[[FIRST:.LCPI[0-9_]+]],r0
; ASM-NEXT: mov.l	[[FIRST]],{{r[0-9]+}}
; ASM: [[FIRST]]:
; ASM-NEXT: .long	external_function@PLT
; ASM: mova	[[SECOND:.LCPI[0-9_]+]],r0
; ASM-NEXT: mov.l	[[SECOND]],{{r[0-9]+}}
; ASM: [[SECOND]]:
; ASM-NEXT: .long	external_function@PLT
; ASM-NOT: .long	external_function

; MIR-LABEL: name:            pic_literal_clones
; MIR: $r0 = MOVA <mcsymbol [[GOTPC:.LCPI[0-9_]+]]>
; MIR-NEXT: $r12 = MOVL_load_pc <mcsymbol [[GOTPC]]>
; MIR: $r0 = MOVA <mcsymbol [[FIRST:.LCPI[0-9_]+]]>
; MIR-NEXT: ${{r[0-9]+}} = MOVL_load_pc <mcsymbol [[FIRST]]>
; MIR: SH_CONSTPOOL_ENTRY %const.1, 1, 4, 4
; MIR: $r0 = MOVA <mcsymbol [[SECOND:.LCPI[0-9_]+]]>
; MIR-NEXT: ${{r[0-9]+}} = MOVL_load_pc <mcsymbol [[SECOND]]>
; MIR: SH_CONSTPOOL_ENTRY %const.1, 0, 4, 4
; MIR-NOT: SH_PIC_SETUP
; MIR-NOT: SH_PIC_ADDRESS

; OBJECT-COUNT-2: R_SH_PLT32 external_function
; OBJECT-NOT: R_SH_DIR32
