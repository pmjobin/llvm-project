; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=EXPAND
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=EXPAND
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=phi-node-elimination < %s | FileCheck %s --check-prefix=PHI
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=virt-reg-rewriter < %s | FileCheck %s --check-prefix=RA
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=sh-literal-pool-range-check < %s | FileCheck %s --check-prefix=ISLAND
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=sh-literal-pool-range-check < %s | FileCheck %s --check-prefix=ISLAND
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM

define i32 @dense_profile(i32 %x) !prof !0 {
entry:
	switch i32 %x, label %default [
		i32 -3, label %a
		i32 -2, label %b
		i32 -1, label %same
		i32 0, label %same
		i32 2, label %c
		i32 3, label %d
	], !prof !1

a:
	br label %merge
b:
	br label %merge
same:
	br label %merge
c:
	br label %merge
d:
	br label %merge
default:
	br label %merge

merge:
	%value = phi i32 [ 10, %a ], [ 20, %b ], [ 30, %same ], [ 40, %c ], [ 50, %d ], [ -1, %default ]
	ret i32 %value
}

!0 = !{!"function_entry_count", i64 1000}
!1 = !{!"branch_weights", i32 100, i32 200, i32 150, i32 75, i32 75, i32 250, i32 150}

; ISEL-LABEL: name:            dense_profile
; ISEL: constants:       []
; ISEL: jumpTable:
; ISEL-NEXT: kind:            inline
; ISEL: blocks:          [ '%bb.1', '%bb.2', '%bb.3', '%bb.3', '%bb.6', '%bb.4',
; ISEL-NEXT: '%bb.5' ]
; ISEL: bb.{{[0-9]+}}.entry:
; ISEL: successors: %bb.1({{.*}}), %bb.2({{.*}}), %bb.3({{.*}}), %bb.6({{.*}}), %bb.4({{.*}}), %bb.5({{.*}})
; ISEL: SH_JT_DISPATCH %[[INDEX:[0-9]+]], %[[INDEX]], %jump-table.0 :: (dereferenceable invariant load (s32) from jump-table)

; EXPAND-LABEL: name:            dense_profile
; EXPAND: jumpTable:
; EXPAND-NEXT: kind:            inline
; EXPAND: bb.{{[0-9]+}}.entry:
; EXPAND: successors: %bb.1({{.*}}), %bb.2({{.*}}), %bb.3({{.*}}), %bb.6({{.*}}), %bb.4({{.*}}), %bb.5({{.*}})
; EXPAND: %[[INDEXCOPY:[0-9]+]]:gpr = COPY %[[INDEX:[0-9]+]]
; EXPAND-NEXT: $r0 = MOVA %jump-table.0
; EXPAND-NEXT: %[[SCALED:[0-9]+]]:gpr = SHLL2 %[[INDEXCOPY]]
; EXPAND-NEXT: %[[TARGET:[0-9]+]]:gpr = MOVL_load_indexed %[[SCALED]], implicit $r0 :: (dereferenceable invariant load (s32) from jump-table)
; EXPAND-NEXT: BRAF %[[TARGET]], %jump-table.0
; EXPAND-NOT: SH_JT_DISPATCH

; PHI-LABEL: name:            dense_profile
; PHI: noPhis:          true
; PHI-NOT: PHI
; PHI: BRAF {{.*}}, %jump-table.0

; RA-LABEL: name:            dense_profile
; RA: noVRegs:         true
; RA: registers:       []
; RA-NOT: %{{[0-9]+}}
; RA: $r0 = MOVA %jump-table.0
; RA-NEXT: ${{r[0-9]+}} = SHLL2
; RA-NEXT: ${{r[0-9]+}} = MOVL_load_indexed {{.*}}, implicit $r0 :: (dereferenceable invariant load (s32) from jump-table)
; RA-NEXT: BRAF {{.*}}, %jump-table.0

; ISLAND-LABEL: name:            dense_profile
; ISLAND: $r0 = MOVA <mcsymbol [[TABLE:\.Lsh_jumptable[0-9]+]]>
; ISLAND-NEXT: ${{r[0-9]+}} = SHLL {{.*}}, implicit-def dead $tbit
; ISLAND-NEXT: ${{r[0-9]+}} = MOVW_load_indexed {{.*}}, implicit killed $r0 :: (dereferenceable invariant load (s16) from jump-table, align 4)
; ISLAND-NEXT: BRAF {{.*}}, %jump-table.0, <mcsymbol [[BASE:\.Lsh_jumptable_base[0-9]+]]> {
; ISLAND-NEXT: NOP
; ISLAND-NEXT: }
; ISLAND: SH_JUMPTABLE %jump-table.0, <mcsymbol [[TABLE]]>, <mcsymbol [[BASE]]>, 2

; ASM-LABEL: dense_profile:
; ASM-DAG: add	#3,[[INDEX:r[0-9]+]]
; ASM-DAG: mov	#6,[[LIMIT:r[0-9]+]]
; ASM: cmp/hi	[[LIMIT]],[[INDEX]]
; ASM: bt	[[DEFAULT:.LBB[0-9_]+]]
; ASM: mova	[[TABLE:.Lsh_jumptable[0-9]+]],r0
; ASM-NEXT: shll	[[INDEX]]
; ASM-NEXT: mov.w	@(r0,[[INDEX]]),[[OFFSET:r[0-9]+]]
; ASM-NEXT: [[BASE:.Lsh_jumptable_base[0-9]+]]:
; ASM-NEXT: braf	[[OFFSET]]
; ASM-NEXT: nop
; ASM: [[TABLE]]:
; ASM-COUNT-7: .short	.LBB{{[0-9_]+}}-[[BASE]]-4
; ASM-NOT: .LJTI
