; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -stop-after=sh-literal-islands < %s | FileCheck %s --check-prefix=ISLAND
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -stop-after=sh-literal-islands < %s | FileCheck %s --check-prefix=ISLAND

declare i32 @external_function(i32)

define internal i32 @local_function(i32 %x) {
	ret i32 %x
}

define i32 @pipeline(i32 %x) {
entry:
	%local = call i32 @local_function(i32 %x)
	%result = call i32 @external_function(i32 %local)
	ret i32 %result
}

; ISEL-LABEL: name:            pipeline
; ISEL: constants:
; ISEL: value:           '_GLOBAL_OFFSET_TABLE_@GOTPC'
; ISEL: value:           'local_function@GOTOFF'
; ISEL: value:           'external_function@PLT'
; ISEL: SH_PIC_SETUP %const.0, -1, implicit-def $r0, implicit-def $r12
; ISEL: %[[LOCAL:[0-9]+]]:gpr = SH_PIC_GOTOFF %const.1, -1
; ISEL: JSR %[[LOCAL]], csr_sh
; ISEL: %[[CALLEE:[0-9]+]]:gprnor0 = SH_PIC_ADDRESS %const.2, -1, implicit-def
; ISEL: JSR %[[CALLEE]], csr_sh

; ISLAND-LABEL: name:            pipeline
; ISLAND-NOT: SH_PIC_SETUP
; ISLAND-NOT: SH_PIC_ADDRESS
; ISLAND: $r0 = MOVA <mcsymbol [[GOTPC:.LCPI[0-9_]+]]>
; ISLAND-NEXT: $r12 = MOVL_load_pc <mcsymbol [[GOTPC]]>
; ISLAND-NEXT: $r12 = ADDrr $r12, killed $r0
; ISLAND: $r0 = MOVL_load_pc <mcsymbol [[GOTOFF:.LCPI[0-9_]+]]>
; ISLAND-NEXT: $r0 = ADDrr $r0, $r12
; ISLAND: $r0 = MOVA <mcsymbol [[PLT:.LCPI[0-9_]+]]>
; ISLAND-NEXT: ${{r[1-9][0-9]*}} = MOVL_load_pc <mcsymbol [[PLT]]>
; ISLAND-NEXT: ${{r[1-9][0-9]*}} = ADDrr ${{r[1-9][0-9]*}}, killed $r0
; ISLAND: SH_CONSTPOOL_ENTRY %const.0, 0, 4, 4
; ISLAND: SH_CONSTPOOL_ENTRY %const.1, 0, 4, 4
