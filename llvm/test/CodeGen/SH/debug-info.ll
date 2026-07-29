; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -dwarf-version=5 %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -dwarf-version=5 %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -dwarf-version=4 -filetype=obj %s -o %t.be.v4.o
; RUN: llvm-dwarfdump --verify %t.be.v4.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --debug-info --debug-line %t.be.v4.o 2>&1 | FileCheck %s --check-prefixes=INFO,V4
; RUN: llvm-readobj --sections --relocations %t.be.v4.o | FileCheck %s --check-prefix=OBJECT
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -dwarf-version=4 -filetype=obj %s -o %t.le.v4.o
; RUN: llvm-dwarfdump --verify %t.le.v4.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --debug-info --debug-line %t.le.v4.o 2>&1 | FileCheck %s --check-prefixes=INFO,V4
; RUN: llvm-readobj --sections --relocations %t.le.v4.o | FileCheck %s --check-prefix=OBJECT
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -dwarf-version=5 -filetype=obj %s -o %t.be.v5.o
; RUN: llvm-dwarfdump --verify %t.be.v5.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --debug-info --debug-line %t.be.v5.o 2>&1 | FileCheck %s --check-prefixes=INFO,V5
; RUN: llvm-readobj --sections --relocations %t.be.v5.o | FileCheck %s --check-prefix=OBJECT
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -dwarf-version=5 -filetype=obj %s -o %t.le.v5.o
; RUN: llvm-dwarfdump --verify %t.le.v5.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --debug-info --debug-line %t.le.v5.o 2>&1 | FileCheck %s --check-prefixes=INFO,V5
; RUN: llvm-readobj --sections --relocations %t.le.v5.o | FileCheck %s --check-prefix=OBJECT

define i32 @add_one(i32 %value) nounwind !dbg !4 {
entry:
  %slot = alloca i32, align 4
  store i32 %value, ptr %slot, align 4, !dbg !12
  call void @llvm.dbg.declare(metadata ptr %slot, metadata !11, metadata !DIExpression()), !dbg !12
  %loaded = load i32, ptr %slot, align 4, !dbg !13
  %result = add i32 %loaded, 1, !dbg !13
  ret i32 %result, !dbg !14
}

define i32 @choose(i32 %value) nounwind !dbg !15 {
entry:
  %condition = icmp eq i32 %value, 0, !dbg !18
  br i1 %condition, label %zero, label %nonzero, !dbg !18

zero:
  ret i32 10, !dbg !19

nonzero:
  ret i32 20, !dbg !20
}

declare void @llvm.dbg.declare(metadata, metadata, metadata)

; ASM-LABEL: add_one:
; ASM: .loc 0 3 0
; ASM-NEXT: .cfi_sections .debug_frame
; ASM-NEXT: .cfi_startproc
; ASM: add #-4,r15
; ASM-NEXT: .cfi_def_cfa_offset 4
; ASM: .loc 0 4 2 prologue_end
; ASM-NEXT: mov.l r0,@r15
; ASM: .loc 0 5 9
; ASM: add #1,r0
; ASM: add #4,r15
; ASM-NEXT: .cfi_def_cfa_offset 0
; ASM: .loc 0 5 2
; ASM-NEXT: rts
; ASM-NEXT: nop
; ASM-LABEL: choose:
; ASM: .cfi_startproc
; ASM: .loc 1 9 2 prologue_end
; ASM: .loc 1 11 2
; ASM-NEXT: mov #10,r0
; ASM: .loc 1 13 2
; ASM-NEXT: mov #20,r0

; VERIFY-NOT: warning:
; VERIFY: No errors.

; INFO: Compile Unit:
; INFO-NOT: warning:
; V4-SAME: version = 0x0004
; V5-SAME: version = 0x0005
; INFO-SAME: addr_size = 0x04
; INFO: DW_TAG_compile_unit
; INFO: DW_AT_producer ("SH CG-16 test")
; INFO: DW_AT_name ("debug.c")
; INFO: DW_AT_comp_dir ("/source")
; INFO: DW_TAG_subprogram
; INFO: DW_AT_frame_base (DW_OP_reg15 R15)
; INFO: DW_AT_linkage_name ("add_one")
; INFO: DW_TAG_formal_parameter
; INFO: DW_AT_location (DW_OP_fbreg +0)
; INFO: DW_AT_name ("value")
; INFO: DW_TAG_subprogram
; INFO: DW_AT_linkage_name ("choose")
; INFO: .debug_line contents:
; V4: version: 4
; V5: version: 5
; V5: address_size: 4
; INFO: min_inst_length: 2
; V5: include_directories[  0] = "/source"
; INFO: include_directories[  1] = "/source/include"
; INFO: name: "debug.c"
; INFO: name: "branch.c"
; INFO: prologue_end
; INFO: end_sequence

; OBJECT: Sections [
; OBJECT-DAG: Name: .debug_abbrev
; OBJECT-DAG: Name: .debug_info
; OBJECT-DAG: Name: .rela.debug_info
; OBJECT-DAG: Name: .debug_line
; OBJECT-DAG: Name: .rela.debug_line
; OBJECT-DAG: Name: .debug_str
; OBJECT: Relocations [
; OBJECT: R_SH_DIR32

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!9}
!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "SH CG-16 test", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "debug.c", directory: "/source")
!2 = !{}
!3 = !DIFile(filename: "branch.c", directory: "/source/include")
!4 = distinct !DISubprogram(name: "add_one", linkageName: "add_one", scope: !1, file: !1, line: 3, type: !6, scopeLine: 3, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!6 = !DISubroutineType(types: !7)
!7 = !{!8, !8}
!8 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!9 = !{i32 2, !"Debug Info Version", i32 3}
!11 = !DILocalVariable(name: "value", arg: 1, scope: !4, file: !1, line: 3, type: !8)
!12 = !DILocation(line: 4, column: 2, scope: !4)
!13 = !DILocation(line: 5, column: 9, scope: !4)
!14 = !DILocation(line: 5, column: 2, scope: !4)
!15 = distinct !DISubprogram(name: "choose", linkageName: "choose", scope: !3, file: !3, line: 8, type: !6, scopeLine: 8, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!18 = !DILocation(line: 9, column: 2, scope: !15)
!19 = !DILocation(line: 11, column: 2, scope: !15)
!20 = !DILocation(line: 13, column: 2, scope: !15)
