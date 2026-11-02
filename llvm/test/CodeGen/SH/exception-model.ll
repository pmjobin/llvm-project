; RUN: split-file %s %t
; RUN: llc -mtriple=shle-unknown-linux-gnu -verify-machineinstrs %t/dwarf.ll -o /dev/null
; RUN: not llc -mtriple=shle-unknown-linux-gnu %t/sjlj.ll -o /dev/null 2>&1 | FileCheck %s
; RUN: not llc -mtriple=sh-unknown-linux-gnu %t/sjlj.ll -o /dev/null 2>&1 | FileCheck %s
; CHECK: SH only supports DWARF CFI exception handling

;--- dwarf.ll
define void @f() {
  ret void
}
!llvm.module.flags = !{!0}
!0 = !{i32 1, !"exception-model", !"dwarf"}

;--- sjlj.ll
define void @f() {
  ret void
}
!llvm.module.flags = !{!0}
!0 = !{i32 1, !"exception-model", !"sjlj"}
