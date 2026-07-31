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
; ASM: mov.l	{{[^,]+}},r0
; ASM-NEXT: add	r12,r0
; ASM: shll2
; ASM: add	{{r[0-9]+}},[[ENTRY:r[0-9]+]]
; ASM: mov.l	@[[ENTRY]],[[OFFSET:r[0-9]+]]
; ASM: add	r0,[[OFFSET]]
; ASM: jmp	@[[OFFSET]]
; ASM-NEXT: nop
; ASM: .long	[[TABLE:.LJTI[0-9_]+]]@GOTOFF
; ASM: .section	.rodata
; ASM: [[TABLE]]:
; ASM-COUNT-5: .long	.LBB{{[0-9_]+}}-[[TABLE]]

; OBJECT: Section {{.*}} .rela.text.dense_switch {
; OBJECT: R_SH_GOTPC _GLOBAL_OFFSET_TABLE_
; OBJECT: R_SH_GOTOFF .LJTI0_0
; OBJECT: Section {{.*}} .rela.rodata{{.*}} {
; OBJECT-COUNT-5: R_SH_REL32
; OBJECT-NOT: R_SH_DIR32
