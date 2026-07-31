; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -dwarf-version=4 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=ASM,DWARF4
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -dwarf-version=5 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=ASM,DWARF5
; RUN: llc -mtriple=sh-unknown-linux-gnu -relocation-model=pic -dwarf-version=4 -O0 -verify-machineinstrs -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-linux-gnu -relocation-model=pic -dwarf-version=5 -O0 -verify-machineinstrs -filetype=obj < %s -o %t.le.o
; RUN: llvm-dwarfdump --verify %t.be.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --verify %t.le.o 2>&1 | FileCheck %s --check-prefix=VERIFY
; RUN: llvm-dwarfdump --debug-info --eh-frame %t.be.o | FileCheck %s --check-prefixes=DWARF,FRAME
; RUN: llvm-dwarfdump --debug-info --eh-frame %t.le.o | FileCheck %s --check-prefixes=DWARF,FRAME
; RUN: llvm-readobj --relocations %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj --relocations %t.le.o | FileCheck %s --check-prefix=OBJECT

@debug_tls = thread_local global i32 5, align 4, !dbg !0

define i32 @read_debug_tls() uwtable !dbg !9 {
entry:
	%value = load i32, ptr @debug_tls, align 4, !dbg !12
	ret i32 %value, !dbg !13
}

; ASM-LABEL: read_debug_tls:
; ASM: .cfi_offset pr, -4
; ASM: .cfi_offset r12,
; ASM: jsr	@r1
; ASM-NEXT: add	r12,r4
; ASM: .long	debug_tls@TLSGD
; ASM: .cfi_restore r12
; ASM: .cfi_restore pr
; ASM: .long	debug_tls@DTPOFF
; DWARF4: .byte	224
; DWARF5: .byte	224

; VERIFY-NOT: error:
; VERIFY-NOT: warning:
; VERIFY: No errors.

; DWARF: DW_TAG_variable
; DWARF: DW_AT_name	("debug_tls")
; DWARF: DW_AT_location	(DW_OP_const4u 0x0, DW_OP_GNU_push_tls_address)

; FRAME: DW_CFA_offset: PR
; FRAME: DW_CFA_offset: R12
; FRAME: DW_CFA_restore: R12
; FRAME: DW_CFA_restore: PR

; OBJECT-DAG: R_SH_TLS_GD_32 debug_tls
; OBJECT-DAG: R_SH_TLS_LDO_32 debug_tls

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!6, !7}
!llvm.ident = !{!8}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "debug_tls", scope: !2, file: !3, line: 1, type: !5, isLocal: false, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "SH TLS test", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !14, globals: !4)
!3 = !DIFile(filename: "tls-debug.c", directory: "/tmp")
!4 = !{!0}
!5 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!6 = !{i32 2, !"Dwarf Version", i32 4}
!7 = !{i32 2, !"Debug Info Version", i32 3}
!8 = !{!"SH TLS test"}
!9 = distinct !DISubprogram(name: "read_debug_tls", scope: !3, file: !3, line: 3, type: !10, scopeLine: 3, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !11)
!10 = !DISubroutineType(types: !11)
!11 = !{!5}
!12 = !DILocation(line: 4, column: 9, scope: !9)
!13 = !DILocation(line: 5, column: 2, scope: !9)
!14 = !{}
