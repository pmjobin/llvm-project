; RUN: split-file %s %t
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=obj %t/accepted.ll -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -filetype=obj %t/accepted.ll -o /dev/null
; RUN: not llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -filetype=asm %t/brackets.ll -o /dev/null 2>&1 | FileCheck %s
; RUN: not llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -filetype=obj %t/mixed.ll -o /dev/null 2>&1 | FileCheck %s
;
; ELF MC accepts brackets as expression grouping. The sizing guard must reject
; them before recursion, even though grouping is absent from the parsed AST.
; CHECK-COUNT-1: error: SH inline assembly size cannot be determined: .space has a malformed expression: constant expression nesting is too deep

;--- accepted.ll
define void @accepted() {
  call void asm sideeffect ".space [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[2]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]", ""()
  ret void
}

;--- brackets.ll
define void @brackets() {
  call void asm sideeffect ".space [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[2]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]", ""()
  ret void
}

;--- mixed.ll
define void @mixed() {
  call void asm sideeffect ".space (((((((((((((((((((((((((((((((([[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[2]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]))))))))))))))))))))))))))))))))", ""()
  ret void
}
