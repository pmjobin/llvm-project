; RUN: opt -passes='loop-unroll' -S < %s | llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs | FileCheck %s --check-prefix=ASM
; RUN: opt -passes='loop-unroll' -S < %s | llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs | FileCheck %s --check-prefix=ASM
; RUN: opt -passes='loop-unroll' -S < %s | llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=sh-literal-pool-range-check | FileCheck %s --check-prefix=RANGE
; RUN: opt -passes='loop-unroll' -S < %s | llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -stop-after=sh-literal-pool-range-check | FileCheck %s --check-prefix=RANGE

declare void @external()

; Full unrolling creates enough delayed external calls to put a trailing
; literal island out of range of the dispatch. SH must create an earlier island
; and relax the ordinary bounds-check branch while preserving the table jump.
define i32 @large_switch(i32 %x) {
entry:
	switch i32 %x, label %default [
		i32 0, label %large
		i32 1, label %c1
		i32 2, label %c2
		i32 3, label %c3
		i32 4, label %c4
		i32 5, label %c5
	]
large:
	br label %loop
loop:
	%index = phi i32 [ 0, %large ], [ %next, %loop ]
	call void @external()
	%next = add nuw nsw i32 %index, 1
	%more = icmp ult i32 %next, 300
	br i1 %more, label %loop, label %done, !llvm.loop !0
done:
	ret i32 0
c1:
	ret i32 17
c2:
	ret i32 31
c3:
	ret i32 47
c4:
	ret i32 67
c5:
	ret i32 89
default:
	ret i32 -1
}

!0 = distinct !{!0, !1}
!1 = !{!"llvm.loop.unroll.full"}

; The out-of-range conditional is inverted around a delayed BRA. The dispatch
; remains an indirect delayed JMP with no relocation or direct target.
; ASM-LABEL: large_switch:
; ASM: cmp/hi
; ASM-NEXT: bf	[[DISPATCH:.LBB[0-9_]+]]
; ASM-NEXT: bra	[[DEFAULT:.LBB[0-9_]+]]
; ASM-NEXT: nop
; ASM-NEXT: [[DISPATCH]]:
; ASM: mov.l	[[TABLE_CPI:.LCPI[0-9_]+]],[[BASE:r[0-9]+]]
; ASM: shll2
; ASM: mov.l	@
; ASM: jmp	@
; ASM-NEXT: nop

; A delayed branch prevents fallthrough into the early island containing only
; the table-address word and the nearby external-call word.
; ASM: mov.l	[[CALL_CPI:.LCPI[0-9_]+]],r8
; ASM-NEXT: bra	[[AFTER_ISLAND:.LBB[0-9_]+]]
; ASM-NEXT: nop
; ASM: .p2align	2
; ASM: [[TABLE_CPI]]:
; ASM-NEXT: .long	[[JTI:.LJTI[0-9_]+]]
; ASM: [[CALL_CPI]]:
; ASM-NEXT: .long	external
; ASM-NEXT: [[AFTER_ISLAND]]:
; ASM: jsr	@r8
; ASM-NEXT: nop

; The jump table remains separate read-only data and is emitted exactly once.
; ASM: .section	.rodata
; ASM: [[JTI]]:
; ASM-COUNT-6: .long	.LBB

; RANGE-LABEL: name:            large_switch
; RANGE: value:           jump-table.0
; RANGE: MOVL_load_pc_island %const.0, [[TABLE_INSTANCE:[0-9]+]]
; RANGE: JMP {{.*}}, %jump-table.0 {
; RANGE-NEXT: NOP
; RANGE-NEXT: }
; RANGE: SH_CONSTPOOL_ENTRY %const.0, [[TABLE_INSTANCE]], 4, 4
; RANGE-NOT: MOVL_load_pc_island %const.0, -1
