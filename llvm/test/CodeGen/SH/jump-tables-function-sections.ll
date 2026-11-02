; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -function-sections -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM --implicit-check-not=.rodata
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -function-sections -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM --implicit-check-not=.rodata
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -function-sections -O2 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -function-sections -O2 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --sections --symbols --relocations %t.be.o | FileCheck %s --check-prefix=OBJ --implicit-check-not=.rodata --implicit-check-not=.rela.text.
; RUN: llvm-readobj --sections --symbols --relocations %t.le.o | FileCheck %s --check-prefix=OBJ --implicit-check-not=.rodata --implicit-check-not=.rela.text.

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

; Each relative jump table stays inline in its executable function section, so
; function-section retention cannot separate a dispatch from its targets.
; ASM: .section	.text.live_switch
; ASM-LABEL: live_switch:
; ASM: mova	[[LIVE_TABLE:.Lsh_jumptable[0-9]+]],r0
; ASM: [[LIVE_BASE:.Lsh_jumptable_base[0-9]+]]:
; ASM-NEXT: braf	{{r[0-9]+}}
; ASM-NEXT: nop
; ASM: [[LIVE_TABLE]]:
; ASM-COUNT-6: .short	.LBB{{[0-9_]+}}-[[LIVE_BASE]]-4
; ASM: .size	live_switch,

; ASM: .section	.text.dead_switch
; ASM-LABEL: dead_switch:
; ASM: mova	[[DEAD_TABLE:.Lsh_jumptable[0-9]+]],r0
; ASM: [[DEAD_BASE:.Lsh_jumptable_base[0-9]+]]:
; ASM-NEXT: braf	{{r[0-9]+}}
; ASM-NEXT: nop
; ASM: [[DEAD_TABLE]]:
; ASM-COUNT-6: .short	.LBB{{[0-9_]+}}-[[DEAD_BASE]]-4
; ASM: .size	dead_switch,

; OBJ: Name: .text.live_switch
; OBJ: Flags [
; OBJ: SHF_ALLOC
; OBJ: SHF_EXECINSTR
; OBJ: Size: 56
; OBJ: AddressAlignment: 4

; OBJ: Name: .text.dead_switch
; OBJ: Flags [
; OBJ: SHF_ALLOC
; OBJ: SHF_EXECINSTR
; OBJ: Size: 60
; OBJ: AddressAlignment: 4

; OBJ: Relocations [
; OBJ: Section {{.*}} .rela.eh_frame {
; OBJ-COUNT-2: R_SH_REL32
; OBJ-NOT: R_SH_DIR32

; OBJ: Name: live_switch
; OBJ: Type: Function
; OBJ: Section: .text.live_switch
; OBJ: Name: dead_switch
; OBJ: Type: Function
; OBJ: Section: .text.dead_switch
