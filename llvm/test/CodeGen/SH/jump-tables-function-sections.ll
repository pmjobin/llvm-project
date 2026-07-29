; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -function-sections -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -function-sections -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -function-sections -O2 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -function-sections -O2 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --sections --symbols --relocations %t.be.o | FileCheck %s --check-prefix=OBJ
; RUN: llvm-readobj --sections --symbols --relocations %t.le.o | FileCheck %s --check-prefix=OBJ

define i32 @live_switch(i32 %x) {
entry:
	switch i32 %x, label %default [
		i32 0, label %c0
		i32 1, label %c1
		i32 2, label %c2
		i32 3, label %c3
		i32 4, label %c4
		i32 5, label %c5
	]
c0:
	ret i32 11
c1:
	ret i32 23
c2:
	ret i32 37
c3:
	ret i32 53
c4:
	ret i32 71
c5:
	ret i32 89
default:
	ret i32 -1
}

define i32 @dead_switch(i32 %x) {
entry:
	switch i32 %x, label %default [
		i32 20, label %c0
		i32 21, label %c1
		i32 22, label %c2
		i32 23, label %c3
		i32 24, label %c4
		i32 25, label %c5
	]
c0:
	ret i32 13
c1:
	ret i32 29
c2:
	ret i32 43
c3:
	ret i32 61
c4:
	ret i32 79
c5:
	ret i32 101
default:
	ret i32 -1
}

; Literal islands stay inline in each executable function section. The
; corresponding jump table follows the function in a separate, independently
; retainable read-only section.
; ASM: .section	.text.live_switch
; ASM-LABEL: live_switch:
; ASM: jmp	@
; ASM-NEXT: nop
; ASM: [[LIVE_CPI:.LCPI[0-9_]+]]:
; ASM-NEXT: .long	[[LIVE_JTI:.LJTI[0-9_]+]]
; ASM: .size	live_switch,
; ASM: .section	.rodata.live_switch
; ASM: [[LIVE_JTI]]:
; ASM-COUNT-6: .long	.LBB

; ASM: .section	.text.dead_switch
; ASM-LABEL: dead_switch:
; ASM: jmp	@
; ASM-NEXT: nop
; ASM: [[DEAD_CPI:.LCPI[0-9_]+]]:
; ASM-NEXT: .long	[[DEAD_JTI:.LJTI[0-9_]+]]
; ASM: .size	dead_switch,
; ASM: .section	.rodata.dead_switch
; ASM: [[DEAD_JTI]]:
; ASM-COUNT-6: .long	.LBB

; OBJ: Name: .text.live_switch
; OBJ: Flags [
; OBJ: SHF_ALLOC
; OBJ: SHF_EXECINSTR
; OBJ: AddressAlignment: 4
; OBJ: Name: .rela.text.live_switch
; OBJ: Name: .rodata.live_switch
; OBJ: Flags [
; OBJ: SHF_ALLOC
; OBJ-NOT: SHF_EXECINSTR
; OBJ: Size: 24
; OBJ: AddressAlignment: 4
; OBJ: Name: .rela.rodata.live_switch

; OBJ: Name: .text.dead_switch
; OBJ: Flags [
; OBJ: SHF_ALLOC
; OBJ: SHF_EXECINSTR
; OBJ: Name: .rela.text.dead_switch
; OBJ: Name: .rodata.dead_switch
; OBJ: Flags [
; OBJ: SHF_ALLOC
; OBJ-NOT: SHF_EXECINSTR
; OBJ: Size: 24
; OBJ: AddressAlignment: 4
; OBJ: Name: .rela.rodata.dead_switch

; OBJ: Section {{.*}} .rela.text.live_switch {
; OBJ-NEXT: 0x{{[0-9A-F]+}} R_SH_DIR32 .LJTI0_0 0x0
; OBJ-NEXT: }
; OBJ: Section {{.*}} .rela.rodata.live_switch {
; OBJ-COUNT-6: R_SH_DIR32
; OBJ-NEXT: }
; OBJ: Section {{.*}} .rela.text.dead_switch {
; OBJ-NEXT: 0x{{[0-9A-F]+}} R_SH_DIR32 .LJTI1_0 0x0
; OBJ-NEXT: }
; OBJ: Section {{.*}} .rela.rodata.dead_switch {
; OBJ-COUNT-6: R_SH_DIR32
; OBJ-NEXT: }

; OBJ: Name: live_switch
; OBJ: Type: Function
; OBJ: Section: .text.live_switch
; OBJ: Name: dead_switch
; OBJ: Type: Function
; OBJ: Section: .text.dead_switch
