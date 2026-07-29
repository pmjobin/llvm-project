; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llvm-dwarfdump --verify %t.be.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --debug-info %t.be.o 2>&1 | FileCheck %s --check-prefixes=COMMON,BE
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llvm-dwarfdump --verify %t.le.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --debug-info %t.le.o 2>&1 | FileCheck %s --check-prefixes=COMMON,LE

%Pair = type { i32, i32 }

define i64 @identity(i64 %value) nounwind !dbg !4 {
entry:
  call void @llvm.dbg.value(metadata i64 %value, metadata !11, metadata !DIExpression()), !dbg !12
  ret i64 %value, !dbg !13
}

define i32 @pair_sum(i32 %first, i32 %second) nounwind !dbg !14 {
entry:
  call void @llvm.dbg.value(metadata i32 %first, metadata !19, metadata !DIExpression(DW_OP_LLVM_fragment, 0, 32)), !dbg !20
  call void @llvm.dbg.value(metadata i32 %second, metadata !19, metadata !DIExpression(DW_OP_LLVM_fragment, 32, 32)), !dbg !20
  %sum = add i32 %first, %second, !dbg !21
  ret i32 %sum, !dbg !22
}

define i32 @stack_arg(i32 %a, i32 %b, i32 %c, i32 %d, i32 %fifth) nounwind !dbg !23 {
entry:
  call void @llvm.dbg.value(metadata i32 %fifth, metadata !26, metadata !DIExpression()), !dbg !27
  ret i32 %fifth, !dbg !28
}

declare void @llvm.dbg.value(metadata, metadata, metadata)

; VERIFY-NOT: warning:
; VERIFY: No errors.

; COMMON-NOT: warning:
; BE: DW_AT_location
; BE: DW_OP_reg1 R1, DW_OP_piece 0x4
; BE-NEXT: {{.*}}DW_OP_reg1 R1, DW_OP_piece 0x4, DW_OP_reg0 R0, DW_OP_piece 0x4
; LE: DW_AT_location
; LE: DW_OP_piece 0x4, DW_OP_reg1 R1, DW_OP_piece 0x4
; LE-NEXT: {{.*}}DW_OP_reg0 R0, DW_OP_piece 0x4, DW_OP_reg1 R1, DW_OP_piece 0x4
; COMMON: DW_AT_name ("value")
; COMMON: DW_AT_linkage_name ("pair_sum")
; COMMON: DW_AT_location
; COMMON: DW_OP_reg4 R4, DW_OP_piece 0x4, DW_OP_reg5 R5, DW_OP_piece 0x4
; COMMON: DW_AT_name ("pair")
; COMMON: DW_AT_linkage_name ("stack_arg")
; COMMON: DW_AT_location (DW_OP_reg0 R0)
; COMMON: DW_AT_name ("fifth")
; COMMON: DW_TAG_structure_type
; COMMON: DW_AT_name ("Pair")
; COMMON: DW_AT_byte_size (0x08)
; COMMON: DW_TAG_member
; COMMON: DW_AT_name ("first")
; COMMON: DW_AT_data_member_location (0x00)
; COMMON: DW_TAG_member
; COMMON: DW_AT_name ("second")
; COMMON: DW_AT_data_member_location (0x04)

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!9, !10}
!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "SH CG-16 test", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "locations.c", directory: "/source")
!2 = !{}
!3 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!4 = distinct !DISubprogram(name: "identity", linkageName: "identity", scope: !1, file: !1, line: 1, type: !6, scopeLine: 1, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !2)
!5 = !DIBasicType(name: "long long", size: 64, encoding: DW_ATE_signed)
!6 = !DISubroutineType(types: !7)
!7 = !{!5, !5}
!8 = !DISubroutineType(types: !15)
!9 = !{i32 2, !"Dwarf Version", i32 5}
!10 = !{i32 2, !"Debug Info Version", i32 3}
!11 = !DILocalVariable(name: "value", arg: 1, scope: !4, file: !1, line: 1, type: !5)
!12 = !DILocation(line: 1, column: 24, scope: !4)
!13 = !DILocation(line: 1, column: 35, scope: !4)
!14 = distinct !DISubprogram(name: "pair_sum", linkageName: "pair_sum", scope: !1, file: !1, line: 3, type: !8, scopeLine: 3, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !2)
!15 = !{!3, !3, !3}
!16 = !DICompositeType(tag: DW_TAG_structure_type, name: "Pair", file: !1, line: 2, size: 64, elements: !17)
!17 = !{!18, !24}
!18 = !DIDerivedType(tag: DW_TAG_member, name: "first", scope: !16, file: !1, line: 2, baseType: !3, size: 32)
!19 = !DILocalVariable(name: "pair", scope: !14, file: !1, line: 4, type: !16)
!20 = !DILocation(line: 4, column: 2, scope: !14)
!21 = !DILocation(line: 5, column: 15, scope: !14)
!22 = !DILocation(line: 5, column: 2, scope: !14)
!23 = distinct !DISubprogram(name: "stack_arg", linkageName: "stack_arg", scope: !1, file: !1, line: 7, type: !25, scopeLine: 7, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !2)
!24 = !DIDerivedType(tag: DW_TAG_member, name: "second", scope: !16, file: !1, line: 2, baseType: !3, size: 32, offset: 32)
!25 = !DISubroutineType(types: !29)
!26 = !DILocalVariable(name: "fifth", arg: 5, scope: !23, file: !1, line: 7, type: !3)
!27 = !DILocation(line: 7, column: 30, scope: !23)
!28 = !DILocation(line: 8, column: 2, scope: !23)
!29 = !{!3, !3, !3, !3, !3, !3}
