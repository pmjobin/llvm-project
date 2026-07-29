; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -filetype=null < %s
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -filetype=null < %s
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -stop-after=phi-node-elimination < %s | FileCheck %s --check-prefix=PHI
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -stop-after=virtregrewriter < %s | FileCheck %s --check-prefix=RA

declare ptr @external_identity(ptr)

define i32 @direct_blockaddress() {
entry:
	indirectbr ptr blockaddress(@direct_blockaddress, %left), [label %left, label %right]
left:
	ret i32 11
right:
	ret i32 22
}

define i32 @phi_blockaddress(i32 %condition) {
entry:
	%iszero = icmp eq i32 %condition, 0
	br i1 %iszero, label %left.path, label %right.path
left.path:
	br label %dispatch
right.path:
	br label %dispatch
dispatch:
	%address = phi ptr [ blockaddress(@phi_blockaddress, %left), %left.path ], [ blockaddress(@phi_blockaddress, %right), %right.path ]
	indirectbr ptr %address, [label %left, label %right]
left:
	ret i32 31
right:
	ret i32 47
}

define i32 @loaded_blockaddress(ptr %slot) {
entry:
	%address = load ptr, ptr %slot, align 4
	indirectbr ptr %address, [label %a, label %b, label %c]
a:
	ret i32 13
b:
	ret i32 29
c:
	ret i32 53
}

define i32 @argument_blockaddress(ptr %address) {
entry:
	indirectbr ptr %address, [label %a, label %b, label %c]
a:
	ret i32 17
b:
	ret i32 37
c:
	ret i32 59
}

define i32 @stored_blockaddress(ptr %slot, i32 %condition) {
entry:
	%iszero = icmp eq i32 %condition, 0
	br i1 %iszero, label %left.path, label %right.path
left.path:
	store ptr blockaddress(@stored_blockaddress, %left), ptr %slot, align 4
	br label %dispatch
right.path:
	store ptr blockaddress(@stored_blockaddress, %right), ptr %slot, align 4
	br label %dispatch
dispatch:
	%address = load ptr, ptr %slot, align 4
	indirectbr ptr %address, [label %left, label %right]
left:
	ret i32 61
right:
	ret i32 83
}

define i32 @returned_blockaddress(i32 %condition) {
entry:
	%iszero = icmp eq i32 %condition, 0
	br i1 %iszero, label %left.path, label %right.path
left.path:
	%left.address = call ptr @external_identity(ptr blockaddress(@returned_blockaddress, %left))
	br label %dispatch
right.path:
	%right.address = call ptr @external_identity(ptr blockaddress(@returned_blockaddress, %right))
	br label %dispatch
dispatch:
	%address = phi ptr [ %left.address, %left.path ], [ %right.address, %right.path ]
	indirectbr ptr %address, [label %left, label %right]
left:
	ret i32 71
right:
	ret i32 97
}

define i32 @roundtrip_blockaddress(i32 %condition) {
entry:
	%iszero = icmp eq i32 %condition, 0
	br i1 %iszero, label %left.path, label %right.path
left.path:
	br label %dispatch
right.path:
	br label %dispatch
dispatch:
	%address = phi ptr [ blockaddress(@roundtrip_blockaddress, %left), %left.path ], [ blockaddress(@roundtrip_blockaddress, %right), %right.path ]
	%integer = ptrtoint ptr %address to i32
	%restored = inttoptr i32 %integer to ptr
	indirectbr ptr %restored, [label %left, label %right]
left:
	ret i32 73
right:
	ret i32 101
}

; A direct block address is represented by a symbolic literal-pool word.
; ASM-LABEL: direct_blockaddress:
; ASM: mov.l	[[DIRECT_CPI:.LCPI[0-9_]+]],[[DIRECT_REG:r[0-9]+]]
; ASM-NEXT: jmp	@[[DIRECT_REG]]
; ASM-NEXT: nop
; ASM: [[DIRECT_LABEL:.Ltmp[0-9]+]]:
; ASM: [[DIRECT_CPI]]:
; ASM-NEXT: .long	[[DIRECT_LABEL]]

; The PHI may be distributed into its incoming paths, but both observable
; block addresses remain distinct and each computed transfer is delayed.
; ASM-LABEL: phi_blockaddress:
; ASM-COUNT-2: mov.l	.LCPI
; ASM: jmp	@
; ASM-NEXT: nop
; ASM: [[PHI_LEFT:.Ltmp[0-9]+]]:
; ASM: [[PHI_RIGHT:.Ltmp[0-9]+]]:
; ASM: .long	[[PHI_RIGHT]]
; ASM: .long	[[PHI_LEFT]]

; Addresses loaded from memory and passed as arguments are ordinary pointers.
; ASM-LABEL: loaded_blockaddress:
; ASM: mov.l	@[[SLOT:r[0-9]+]],[[LOADED:r[0-9]+]]
; ASM-NEXT: jmp	@[[LOADED]]
; ASM-NEXT: nop
; ASM-LABEL: argument_blockaddress:
; ASM: jmp	@r4
; ASM-NEXT: nop

; Stored, returned, and ptrtoint/inttoptr round-tripped addresses retain the
; same indirect-jump shape.
; ASM-LABEL: stored_blockaddress:
; ASM: mov.l
; ASM: jmp	@
; ASM-NEXT: nop
; ASM-LABEL: returned_blockaddress:
; ASM: jsr	@
; ASM-NEXT: nop
; ASM: jmp	@
; ASM-NEXT: nop
; ASM-LABEL: roundtrip_blockaddress:
; ASM: jmp	@
; ASM-NEXT: nop

; The IR destination list remains the authoritative machine CFG even though
; JMP has no encoded MBB operand.
; ISEL-LABEL: name:            loaded_blockaddress
; ISEL: successors: %bb.1(0x2aaaaaab), %bb.2(0x2aaaaaab), %bb.3(0x2aaaaaab)
; ISEL: %[[ADDRESS:[0-9]+]]:gpr = MOVL_load_reg
; ISEL: JMP {{(killed )?}}%[[ADDRESS]]

; ISEL-LABEL: name:            argument_blockaddress
; ISEL: bb.0.entry:
; ISEL-NEXT: successors: %bb.1(0x2aaaaaab), %bb.2(0x2aaaaaab), %bb.3(0x2aaaaaab)
; ISEL: %[[ARG:[0-9]+]]:gpr = COPY $r4
; ISEL-NEXT: JMP %[[ARG]]
; ISEL-NOT: JMP {{.*}}%bb.

; PHI-LABEL: name:            phi_blockaddress
; PHI: noPhis:          true
; PHI-NOT: PHI
; PHI: JMP
; PHI-LABEL: name:            roundtrip_blockaddress
; PHI: noPhis:          true
; PHI-NOT: PHI
; PHI: JMP

; RA-LABEL: name:            argument_blockaddress
; RA: noVRegs:         true
; RA: registers:       []
; RA-NOT: %{{[0-9]+}}
; RA: JMP {{(killed )?}}$r4
