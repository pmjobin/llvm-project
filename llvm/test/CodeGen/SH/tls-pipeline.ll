; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs -stop-before=sh-literal-islands < %s | FileCheck %s --check-prefix=RA
; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs -stop-after=sh-literal-islands < %s | FileCheck %s --check-prefix=ISLAND
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -O0 -verify-machineinstrs -stop-after=sh-literal-pool-range-check < %s | FileCheck %s --check-prefix=FINAL

@gd = external thread_local global i32
@ld = internal thread_local(localdynamic) global i32 1
@ie = external thread_local(initialexec) global i32
@le = internal thread_local(localexec) global i32 2

define i32 @tls_pipeline() {
	%a = load i32, ptr @gd, align 4
	%b = load i32, ptr @ld, align 4
	%c = load i32, ptr @ie, align 4
	%d = load i32, ptr @le, align 4
	%ab = add i32 %a, %b
	%cd = add i32 %c, %d
	%result = add i32 %ab, %cd
	ret i32 %result
}

; ISEL-LABEL: name:            tls_pipeline
; ISEL-DAG: value:           'gd@TLSGD'
; ISEL-DAG: value:           '__tls_get_addr@PLT'
; ISEL-DAG: value:           'ld@TLSLDM'
; ISEL-DAG: value:           'ld@DTPOFF'
; ISEL-DAG: value:           'ie@GOTTPOFF'
; ISEL-DAG: value:           'le@TPOFF'
; ISEL: SH_PIC_SETUP
; ISEL-DAG: SH_TLS_CALL %const.{{[0-9]+}}, -1, %const.{{[0-9]+}}, -1, csr_sh
; ISEL-DAG: SH_TLS_CALL %const.{{[0-9]+}}, -1, %const.{{[0-9]+}}, -1, csr_sh
; ISEL-DAG: {{%[0-9]+}}:gprnor0 = SH_TLS_IE %const.{{[0-9]+}}, -1
; ISEL-DAG: {{%[0-9]+}}:gpr = STC_GBR

; RA-LABEL: name:            tls_pipeline
; RA: noVRegs:         true
; RA: registers:       []
; RA-NOT: %{{[0-9]+}}
; RA-DAG: SH_TLS_CALL %const.{{[0-9]+}}, -1, %const.{{[0-9]+}}, -1, csr_sh
; RA-DAG: SH_TLS_CALL %const.{{[0-9]+}}, -1, %const.{{[0-9]+}}, -1, csr_sh
; RA-DAG: ${{r[1-9][0-9]*}} = SH_TLS_IE %const.{{[0-9]+}}, -1
; RA-DAG: ${{r[0-9]+}} = STC_GBR

; ISLAND-LABEL: name:            tls_pipeline
; ISLAND-NOT: SH_TLS_CALL
; ISLAND-NOT: SH_TLS_IE
; ISLAND: ${{r[0-9]+}} = STC_GBR
; ISLAND: $r0 = MOVL_load_pc <mcsymbol .LCPI0_[[IE_CPI:[0-9]+]]_[[IE_INSTANCE:[0-9]+]]>
; ISLAND-NEXT: ${{r[1-9][0-9]*}} = STC_GBR
; ISLAND-NEXT: $r0 = MOVL_load_indexed $r12
; ISLAND: BRA
; ISLAND-NEXT: ${{r[1-9][0-9]*}} = ADDrr
; ISLAND: SH_CONSTPOOL_ENTRY %const.[[IE_CPI]], [[IE_INSTANCE]], 4, 4
; ISLAND: $r4 = MOVL_load_pc <mcsymbol .LCPI0_[[LD_CPI:[0-9]+]]_[[LD_INSTANCE:[0-9]+]]>
; ISLAND-NEXT: $r0 = MOVA <mcsymbol .LCPI0_[[LD_RESOLVER_CPI:[0-9]+]]_[[LD_RESOLVER_INSTANCE:[0-9]+]]>
; ISLAND-NEXT: $r1 = MOVL_load_pc
; ISLAND-NEXT: $r1 = ADDrr $r1, killed $r0
; ISLAND-NEXT: JSR killed $r1, csr_sh, implicit-def $pr, implicit killed $r4, implicit-def $r0 {
; ISLAND-NEXT: $r4 = ADDrr $r4, $r12
; ISLAND-NEXT: }
; ISLAND: SH_CONSTPOOL_ENTRY %const.[[LD_CPI]], [[LD_INSTANCE]], 4, 4
; ISLAND-NEXT: SH_CONSTPOOL_ENTRY %const.[[LD_RESOLVER_CPI]], [[LD_RESOLVER_INSTANCE]], 4, 4
; ISLAND: $r4 = MOVL_load_pc <mcsymbol .LCPI0_[[GD_CPI:[0-9]+]]_[[GD_INSTANCE:[0-9]+]]>
; ISLAND-NEXT: $r0 = MOVA <mcsymbol .LCPI0_[[GD_RESOLVER_CPI:[0-9]+]]_[[GD_RESOLVER_INSTANCE:[0-9]+]]>
; ISLAND: SH_CONSTPOOL_ENTRY %const.[[GD_CPI]], [[GD_INSTANCE]], 4, 4
; ISLAND-NEXT: SH_CONSTPOOL_ENTRY %const.[[GD_RESOLVER_CPI]], [[GD_RESOLVER_INSTANCE]], 4, 4
; ISLAND-NOT: SH_TLS_CALL
; ISLAND-NOT: SH_TLS_IE

; FINAL-LABEL: name:            tls_pipeline
; FINAL-NOT: SH_TLS_CALL
; FINAL-NOT: SH_TLS_IE
; FINAL: $r0 = MOVL_load_pc
; FINAL-NEXT: ${{r[1-9][0-9]*}} = STC_GBR
; FINAL-NEXT: $r0 = MOVL_load_indexed $r12
; FINAL: BRA {{.*}} {
; FINAL-NEXT: ${{r[1-9][0-9]*}} = ADDrr
; FINAL-NEXT: }
; FINAL: JSR killed $r1, csr_sh, implicit-def $pr, implicit killed $r4, implicit-def $r0 {
; FINAL-NEXT: $r4 = ADDrr $r4, $r12
; FINAL-NEXT: }
; FINAL-NOT: SH_TLS_CALL
; FINAL-NOT: SH_TLS_IE
