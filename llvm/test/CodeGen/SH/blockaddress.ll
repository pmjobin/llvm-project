; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -filetype=null < %s
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -filetype=null < %s
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL

declare ptr @external_identity(ptr)

define ptr @return_blockaddress(i32 %condition) {
entry:
	%iszero = icmp eq i32 %condition, 0
	br i1 %iszero, label %return.address, label %target
return.address:
	ret ptr blockaddress(@return_blockaddress, %target)
target:
	ret ptr null
}

define i32 @compare_blockaddress_with_null(i32 %condition) {
entry:
	%iszero = icmp eq i32 %condition, 0
	br i1 %iszero, label %left.path, label %right.path
left.path:
	%left = call ptr @external_identity(ptr blockaddress(@compare_blockaddress_with_null, %left.target))
	br label %compare
right.path:
	%right = call ptr @external_identity(ptr null)
	br label %compare
compare:
	%address = phi ptr [ %left, %left.path ], [ %right, %right.path ]
	%isnull = icmp eq ptr %address, null
	br i1 %isnull, label %null, label %left.target
left.target:
	ret i32 11
null:
	ret i32 23
}

define i32 @compare_two_blockaddresses(i32 %condition) {
entry:
	%iszero = icmp eq i32 %condition, 0
	br i1 %iszero, label %left.path, label %right.path
left.path:
	%left = call ptr @external_identity(ptr blockaddress(@compare_two_blockaddresses, %left.target))
	br label %compare
right.path:
	%right = call ptr @external_identity(ptr blockaddress(@compare_two_blockaddresses, %right.target))
	br label %compare
compare:
	%address = phi ptr [ %left, %left.path ], [ %right, %right.path ]
	%isleft = icmp eq ptr %address, blockaddress(@compare_two_blockaddresses, %left.target)
	br i1 %isleft, label %left.target, label %right.target
left.target:
	ret i32 41
right.target:
	ret i32 59
}

define i32 @deduplicate_blockaddress_literals(ptr %slot) {
entry:
	store ptr blockaddress(@deduplicate_blockaddress_literals, %target), ptr %slot, align 4
	%address = call ptr @external_identity(ptr blockaddress(@deduplicate_blockaddress_literals, %target))
	indirectbr ptr %address, [label %target, label %other]
target:
	ret i32 71
other:
	ret i32 97
}

; Returning a block address uses the ordinary pointer return register and keeps
; the addressed block's distinct emitted label.
; ASM-LABEL: return_blockaddress:
; ASM: mov.l	[[RETURN_CPI:.LCPI[0-9_]+]],r0
; ASM: rts
; ASM-NEXT: nop
; ASM: [[RETURN_LABEL:.Ltmp[0-9]+]]:
; ASM: [[RETURN_CPI]]:
; ASM-NEXT: .long	[[RETURN_LABEL]]

; Block addresses participate in ordinary pointer calls, PHIs, equality, and
; inequality/null control flow.
; ASM-LABEL: compare_blockaddress_with_null:
; ASM: jsr	@
; ASM-NEXT: nop
; ASM: tst
; ASM: {{bt|bf}}
; ASM: .long	.Ltmp
; ASM-LABEL: compare_two_blockaddresses:
; ASM: jsr	@
; ASM-NEXT: nop
; ASM: cmp/eq
; ASM: {{bt|bf}}
; ASM: .long	.Ltmp

; Two SelectionDAG references to one BlockAddress share one original target
; constant-pool entry. Literal-island placement may still clone its instance.
; ISEL-LABEL: name:            deduplicate_blockaddress_literals
; ISEL: constants:
; ISEL-COUNT-1: value:           'blockaddress(@deduplicate_blockaddress_literals, %target)'
; ISEL-COUNT-1: value:           external_identity
; ISEL: body:
; ISEL-COUNT-1: MOVL_load_pc_island %const.0, -1
; ISEL: MOVL_load_pc_island %const.1, -1
; ISEL: JMP
