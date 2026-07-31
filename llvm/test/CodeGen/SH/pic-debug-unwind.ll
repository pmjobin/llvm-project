; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-dwarfdump --verify %t.be.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --verify %t.le.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --eh-frame %t.be.o | FileCheck %s --check-prefix=FRAME
; RUN: llvm-dwarfdump --eh-frame %t.le.o | FileCheck %s --check-prefix=FRAME
; RUN: llvm-readobj --sections --relocations %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj --sections --relocations %t.le.o | FileCheck %s --check-prefix=OBJECT
; RUN: %python %S/Inputs/check-sh-pic-elf.py llvm-readelf %t.be.o %t.le.o

declare i32 @external_function(i32)

define i32 @pic_debug(i32 %value) uwtable !dbg !4 {
entry:
	%called = call i32 @external_function(i32 %value), !dbg !11
	switch i32 %called, label %default [
		i32 0, label %case0
		i32 1, label %case1
		i32 2, label %case2
		i32 3, label %case3
		i32 4, label %case4
	], !dbg !12

case0:
	ret i32 10, !dbg !13

case1:
	ret i32 20, !dbg !14

case2:
	ret i32 30, !dbg !15

case3:
	ret i32 40, !dbg !16

case4:
	ret i32 50, !dbg !17

default:
	ret i32 -1, !dbg !18
}

; ASM-LABEL: pic_debug:
; ASM: .cfi_offset pr, -4
; ASM: .cfi_offset r12, -8
; ASM: mova	[[GOTPC:.LCPI[0-9_]+]],r0
; ASM-NEXT: mov.l	[[GOTPC]],r12
; ASM-NEXT: add	r0,r12
; ASM: .loc 0 2 12 prologue_end
; ASM: mova	[[PLT:.LCPI[0-9_]+]],r0
; ASM-NEXT: mov.l	[[PLT]],{{r[0-9]+}}
; ASM-NEXT: add	r0,{{r[0-9]+}}
; ASM-NEXT: jsr
; ASM-NEXT: nop
; ASM: .loc 0 0
; ASM: [[GOTPC]]:
; ASM-NEXT: .long	_GLOBAL_OFFSET_TABLE_
; ASM: [[PLT]]:
; ASM-NEXT: .long	external_function@PLT
; ASM: .section	.rodata
; ASM: .long	.LBB{{[0-9_]+}}-.LJTI

; VERIFY-NOT: warning:
; VERIFY: No errors.

; FRAME: DW_CFA_offset: PR
; FRAME: DW_CFA_offset: R12
; FRAME: DW_CFA_restore: R12
; FRAME: DW_CFA_restore: PR

; OBJECT-DAG: Name: .eh_frame
; OBJECT-DAG: Name: .rela.eh_frame
; OBJECT: Section {{.*}} .rela.text {
; OBJECT: R_SH_GOTPC _GLOBAL_OFFSET_TABLE_
; OBJECT: R_SH_PLT32 external_function
; OBJECT-NOT: R_SH_DIR32
; OBJECT: }
; OBJECT: Section {{.*}} .rela.rodata {
; OBJECT: R_SH_REL32
; OBJECT: Section {{.*}} .rela.eh_frame {
; OBJECT: R_SH_REL32

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!9, !10}
!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "SH CG-17 test", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "pic.c", directory: "/source")
!2 = !{}
!4 = distinct !DISubprogram(name: "pic_debug", linkageName: "pic_debug", scope: !1, file: !1, line: 1, type: !6, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!6 = !DISubroutineType(types: !7)
!7 = !{!8, !8}
!8 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!9 = !{i32 2, !"Dwarf Version", i32 5}
!10 = !{i32 2, !"Debug Info Version", i32 3}
!11 = !DILocation(line: 2, column: 12, scope: !4)
!12 = !DILocation(line: 3, column: 2, scope: !4)
!13 = !DILocation(line: 5, column: 2, scope: !4)
!14 = !DILocation(line: 7, column: 2, scope: !4)
!15 = !DILocation(line: 9, column: 2, scope: !4)
!16 = !DILocation(line: 11, column: 2, scope: !4)
!17 = !DILocation(line: 13, column: 2, scope: !4)
!18 = !DILocation(line: 15, column: 2, scope: !4)
