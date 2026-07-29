; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llvm-dwarfdump --verify %t.be.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --debug-line %t.be.o 2>&1 | FileCheck %s --check-prefix=LINE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llvm-dwarfdump --verify %t.le.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --debug-line %t.le.o 2>&1 | FileCheck %s --check-prefix=LINE

declare i32 @callee(i32)

define i32 @debug_data(i32 %value) !dbg !4 {
entry:
  %called = call i32 @callee(i32 %value), !dbg !11
  switch i32 %called, label %default [
    i32 0, label %case0
    i32 1, label %case1
    i32 2, label %case2
    i32 3, label %case3
    i32 4, label %case4
  ], !dbg !12

case0:
  ret i32 1000, !dbg !13

case1:
  ret i32 1001, !dbg !14

case2:
  ret i32 1002, !dbg !15

case3:
  ret i32 1003, !dbg !16

case4:
  ret i32 1004, !dbg !17

default:
  ret i32 1999, !dbg !18
}

; ASM-LABEL: debug_data:
; ASM: .loc 0 2 12 prologue_end
; ASM: jsr
; ASM-NEXT: nop
; ASM: .loc 0 3 2
; ASM: jmp
; ASM-NEXT: nop
; ASM: .loc 0 5 2
; ASM: .loc 0 7 2
; ASM: .loc 0 9 2
; ASM: .loc 0 11 2
; ASM: .loc 0 13 2
; ASM: .loc 0 15 2
; ASM: .loc 0 0 2
; ASM: .LCPI0_0_0:
; ASM-NEXT: .long callee
; ASM-NEXT: .LCPI0_1_0:
; ASM-NEXT: .long .LJTI0_0
; ASM-NEXT: .Lfunc_end0:
; ASM: .cfi_endproc
; ASM: .section .rodata
; ASM: .LJTI0_0:
; ASM-NEXT: .long .LBB0_2
; ASM-NEXT: .long .LBB0_3
; ASM-NEXT: .long .LBB0_4
; ASM-NEXT: .long .LBB0_5
; ASM-NEXT: .long .LBB0_6

; VERIFY-NOT: warning:
; VERIFY: No errors.

; LINE-NOT: warning:
; LINE: address_size: 4
; LINE: min_inst_length: 2
; LINE: 0x0000000000000004 2 12
; LINE: 0x000000000000000c 3 2
; LINE: 0x0000000000000028 5 2
; LINE: 0x000000000000003a 7 2
; LINE: 0x000000000000004c 9 2
; LINE: 0x000000000000005e 11 2
; LINE: 0x0000000000000070 13 2
; LINE: 0x0000000000000082 15 2
; LINE-NEXT: 0x0000000000000094 0 2
; LINE-NEXT: 0x000000000000009c 0 2
; LINE-SAME: end_sequence

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!9, !10}
!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "SH CG-16 test", isOptimized: false, runtimeVersion: 0, emissionKind: LineTablesOnly)
!1 = !DIFile(filename: "data.c", directory: "/source")
!2 = !{}
!4 = distinct !DISubprogram(name: "debug_data", linkageName: "debug_data", scope: !1, file: !1, line: 1, type: !6, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
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
