; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -function-sections -O2 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -function-sections -O2 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj -r %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj -r %t.le.o | FileCheck %s --check-prefix=OBJECT
; RUN: %python %S/Inputs/check-sh-pic-elf.py llvm-readelf %t.be.o %t.le.o

define i32 @dense_switch(i32 %x) {
entry:
	switch i32 %x, label %default [
		i32 0, label %a
		i32 1, label %b
		i32 2, label %same
		i32 3, label %same
		i32 4, label %c
	]

a:
	ret i32 10
b:
	ret i32 20
same:
	ret i32 30
c:
	ret i32 40
default:
	ret i32 -1
}

; ASM-LABEL: dense_switch:
; ASM: mova	[[TABLE:.Lsh_jumptable[0-9]+]],r0
; ASM-NEXT: shll	[[INDEX:r[0-9]+]]
; ASM-NEXT: mov.w	@(r0,[[INDEX]]),[[OFFSET:r[0-9]+]]
; ASM-NEXT: [[BASE:.Lsh_jumptable_base[0-9]+]]:
; ASM-NEXT: braf	[[OFFSET]]
; ASM-NEXT: nop
; ASM: [[TABLE]]:
; ASM-COUNT-5: .short	.LBB{{[0-9_]+}}-[[BASE]]-4
; ASM-NOT: _GLOBAL_OFFSET_TABLE_
; ASM-NOT: @GOTOFF
; ASM-NOT: .LJTI

; OBJECT: Section {{.*}} .rela.eh_frame {
; OBJECT-NEXT: 0x{{[0-9A-F]+}} R_SH_REL32 .L0 0x0
; OBJECT-NEXT: }
; OBJECT-NOT: R_SH_GOTPC
; OBJECT-NOT: R_SH_GOTOFF
; OBJECT-NOT: R_SH_DIR32
