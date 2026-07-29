; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=EXPAND
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=EXPAND
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=phi-node-elimination < %s | FileCheck %s --check-prefix=PHI
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=virtregrewriter < %s | FileCheck %s --check-prefix=RA
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
; ISEL: constants:
; ISEL: value:           jump-table.0
; ISEL-NEXT: alignment:       4
; ISEL-NEXT: isTargetSpecific: true
; ISEL: jumpTable:
; ISEL-NEXT: kind:            block-address
; ISEL: blocks:          [ '%bb.1', '%bb.2', '%bb.3', '%bb.3', '%bb.6', '%bb.4',
; ISEL-NEXT: '%bb.5' ]
; ISEL: bb.{{[0-9]+}}.entry:
; ISEL: successors: %bb.1({{.*}}), %bb.2({{.*}}), %bb.3({{.*}}), %bb.6({{.*}}), %bb.4({{.*}}), %bb.5({{.*}})
; ISEL: %[[BASE:[0-9]+]]:gpr = MOVL_load_pc_island %const.0, -1 :: (dereferenceable invariant load (s32) from constant-pool)
; ISEL-NEXT: SH_JT_DISPATCH %{{[0-9]+}}, killed %[[BASE]], %jump-table.0 :: (dereferenceable invariant load (s32) from jump-table)

; EXPAND-LABEL: name:            dense_profile
; EXPAND: jumpTable:
; EXPAND-NEXT: kind:            block-address
; EXPAND: bb.{{[0-9]+}}.entry:
; EXPAND: successors: %bb.1({{.*}}), %bb.2({{.*}}), %bb.3({{.*}}), %bb.6({{.*}}), %bb.4({{.*}}), %bb.5({{.*}})
; EXPAND: %[[BASE:[0-9]+]]:gpr = MOVL_load_pc_island %const.0, -1 :: (dereferenceable invariant load (s32) from constant-pool)
; EXPAND-NEXT: %[[INDEXCOPY:[0-9]+]]:gpr = MOVrr %[[INDEX:[0-9]+]]
; EXPAND-NEXT: %[[SCALED:[0-9]+]]:gpr = SHLL2 %[[INDEXCOPY]]
; EXPAND-NEXT: %[[BASECOPY:[0-9]+]]:gpr = MOVrr %[[BASE]]
; EXPAND-NEXT: %[[ENTRY:[0-9]+]]:gpr = ADDrr %[[BASECOPY]], %[[SCALED]]
; EXPAND-NEXT: %[[TARGET:[0-9]+]]:gpr = MOVL_load_reg %[[ENTRY]] :: (dereferenceable invariant load (s32) from jump-table)
; EXPAND-NEXT: JMP %[[TARGET]], %jump-table.0
; EXPAND-NOT: SH_JT_DISPATCH

; PHI-LABEL: name:            dense_profile
; PHI: noPhis:          true
; PHI-NOT: PHI
; PHI: JMP {{.*}}, %jump-table.0

; RA-LABEL: name:            dense_profile
; RA: noVRegs:         true
; RA: registers:       []
; RA-NOT: %{{[0-9]+}}
; RA: SHLL2
; RA: MOVL_load_reg {{.*}} :: (dereferenceable invariant load (s32) from jump-table)
; RA-NEXT: JMP {{.*}}, %jump-table.0

; ISLAND-LABEL: name:            dense_profile
; ISLAND: MOVL_load_pc_island %const.0, [[INSTANCE:[0-9]+]] :: (dereferenceable invariant load (s32) from constant-pool)
; ISLAND: SHLL2
; ISLAND: MOVL_load_reg {{.*}} :: (dereferenceable invariant load (s32) from jump-table)
; ISLAND-NEXT: JMP {{.*}}, %jump-table.0 {
; ISLAND-NEXT: NOP
; ISLAND-NEXT: }
; ISLAND: SH_CONSTPOOL_ENTRY %const.0, [[INSTANCE]], 4, 4

; ASM-LABEL: dense_profile:
; ASM: add	#3,[[INDEX:r[0-9]+]]
; ASM: mov	#6,[[LIMIT:r[0-9]+]]
; ASM: cmp/hi	[[LIMIT]],[[INDEX]]
; ASM: bt	[[DEFAULT:.LBB[0-9_]+]]
; ASM: mov.l	[[CPI:.LCPI[0-9_]+]],[[BASE:r[0-9]+]]
; ASM: shll2	[[SCALED:r[0-9]+]]
; ASM: add	[[SCALED]],[[BASE]]
; ASM: mov.l	@[[BASE]],[[BASE]]
; ASM: jmp	@[[BASE]]
; ASM-NEXT: nop
; ASM: [[CPI]]:
; ASM-NEXT: .long	[[JTI:.LJTI[0-9_]+]]
; ASM: .section	.rodata
; ASM: .p2align	2
; ASM-NEXT: [[JTI]]:
; ASM-COUNT-7: .long	.LBB
