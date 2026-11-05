; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -filetype=obj %s -o %t.le.o
; RUN: llvm-readobj --symbols %t.le.o | FileCheck %s --check-prefix=OBJ
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -filetype=obj %s -o %t.be.o
; RUN: llvm-readobj --symbols %t.be.o | FileCheck %s --check-prefix=OBJ

; libc++abi defines local references to operator new using symbol assignments.
; These emit no bytes and must not disrupt branch/literal-pool sizing.
declare void @alias()
define weak void @base() {
  ret void
}
define ptr @get_alias() {
  call void asm sideeffect "${0:c} = ${1:c}", "X,X"(ptr @alias, ptr @base)
  ret ptr @alias
}
; ASM: alias = base
; OBJ: Name: alias
; OBJ-NEXT: Value: 0x0
; OBJ: Name: base
; OBJ-NEXT: Value: 0x0
