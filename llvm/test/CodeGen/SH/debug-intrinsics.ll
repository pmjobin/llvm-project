; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llvm-dwarfdump --verify %t.be.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --debug-info %t.be.o 2>&1 | FileCheck %s --check-prefix=INFO
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llvm-dwarfdump --verify %t.le.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --debug-info %t.le.o 2>&1 | FileCheck %s --check-prefix=INFO

define i32 @debug_intrinsics(i32 %value) nounwind !dbg !4 {
entry:
	%slot = alloca i32, align 4, !DIAssignID !16
	call void @llvm.dbg.declare(metadata ptr %slot, metadata !11, metadata !DIExpression()), !dbg !13
	call void @llvm.lifetime.start.p0(ptr %slot), !dbg !13
	store i32 %value, ptr %slot, align 4, !dbg !13, !DIAssignID !17
	call void @llvm.dbg.assign(metadata i32 %value, metadata !11, metadata !DIExpression(), metadata !17, metadata ptr %slot, metadata !DIExpression()), !dbg !13
	call void @llvm.dbg.value(metadata i32 %value, metadata !11, metadata !DIExpression()), !dbg !13
	call void @llvm.dbg.label(metadata !12), !dbg !14
	%result = load i32, ptr %slot, align 4, !dbg !14
	call void @llvm.lifetime.end.p0(ptr %slot), !dbg !15
	ret i32 %result, !dbg !15
}

declare void @llvm.dbg.declare(metadata, metadata, metadata)
declare void @llvm.dbg.value(metadata, metadata, metadata)
declare void @llvm.dbg.assign(metadata, metadata, metadata, metadata, metadata, metadata)
declare void @llvm.dbg.label(metadata)
declare void @llvm.lifetime.start.p0(ptr)
declare void @llvm.lifetime.end.p0(ptr)

; VERIFY-NOT: warning:
; VERIFY: No errors.

; INFO-NOT: warning:
; INFO: DW_TAG_subprogram
; INFO: DW_AT_linkage_name ("debug_intrinsics")
; INFO: DW_TAG_variable
; INFO: DW_AT_location
; INFO: DW_AT_name ("local")
; INFO: DW_TAG_label
; INFO: DW_AT_name ("ready")

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!9, !10, !18}
!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "SH CG-16 test", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "intrinsics.c", directory: "/source")
!2 = !{}
!3 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!4 = distinct !DISubprogram(name: "debug_intrinsics", linkageName: "debug_intrinsics", scope: !1, file: !1, line: 1, type: !6, scopeLine: 1, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !5)
!5 = !{!11, !12}
!6 = !DISubroutineType(types: !7)
!7 = !{!3, !3}
!9 = !{i32 2, !"Dwarf Version", i32 5}
!10 = !{i32 2, !"Debug Info Version", i32 3}
!11 = !DILocalVariable(name: "local", scope: !4, file: !1, line: 2, type: !3)
!12 = !DILabel(scope: !4, name: "ready", file: !1, line: 4)
!13 = !DILocation(line: 2, column: 2, scope: !4)
!14 = !DILocation(line: 4, column: 2, scope: !4)
!15 = !DILocation(line: 5, column: 2, scope: !4)
!16 = distinct !DIAssignID()
!17 = distinct !DIAssignID()
!18 = !{i32 7, !"debug-info-assignment-tracking", i1 true}
