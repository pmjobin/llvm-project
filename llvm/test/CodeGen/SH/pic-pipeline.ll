; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -stop-after=sh-literal-islands < %s | FileCheck %s --check-prefix=ISLAND
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -stop-after=sh-literal-islands < %s | FileCheck %s --check-prefix=ISLAND

declare i32 @external_function(i32)

define i32 @pipeline(i32 %x) {
entry:
	%result = call i32 @external_function(i32 %x)
	ret i32 %result
}

; ISEL-LABEL: name:            pipeline
; ISEL: constants:
; ISEL: value:           '_GLOBAL_OFFSET_TABLE_@GOTPC'
; ISEL: value:           'external_function@PLT'
; ISEL: SH_PIC_SETUP %const.0, -1, implicit-def $r0, implicit-def $r12
; ISEL: %[[CALLEE:[0-9]+]]:gprnor0 = SH_PIC_ADDRESS %const.1, -1, implicit-def
; ISEL: JSR killed %[[CALLEE]], csr_sh

; ISLAND-LABEL: name:            pipeline
; ISLAND-NOT: SH_PIC_SETUP
; ISLAND-NOT: SH_PIC_ADDRESS
; ISLAND: $r0 = MOVA <mcsymbol [[GOTPC:.LCPI[0-9_]+]]>
; ISLAND-NEXT: $r12 = MOVL_load_pc <mcsymbol [[GOTPC]]>
; ISLAND-NEXT: $r12 = ADDrr $r12, killed $r0
; ISLAND: $r0 = MOVA <mcsymbol [[PLT:.LCPI[0-9_]+]]>
; ISLAND-NEXT: ${{r[1-9][0-9]*}} = MOVL_load_pc <mcsymbol [[PLT]]>
; ISLAND-NEXT: ${{r[1-9][0-9]*}} = ADDrr ${{r[1-9][0-9]*}}, killed $r0
; ISLAND: SH_CONSTPOOL_ENTRY %const.0, 0, 4, 4
; ISLAND: SH_CONSTPOOL_ENTRY %const.1, 0, 4, 4
