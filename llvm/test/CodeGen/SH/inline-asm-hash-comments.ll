; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O3 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o %t
; RUN: llvm-objdump -d %t | FileCheck %s --check-prefix=OBJ

; The tied output must preserve the incoming pointer. The comment contributes
; no encoding, while the two immediate operands contribute real instructions.
; Use SH2 for the exact object sequence to exclude the independent SH4 FPSCR
; restoration emitted after inline asm by the existing FPU state pass.
define ptr @comment_tied(ptr %p) optsize {
  %q = call ptr asm sideeffect "# an arbitrary GNU comment", "=r,0"(ptr %p)
  ret ptr %q
}

; ASM-LABEL: comment_tied:
; ASM: !APP
; ASM-NEXT: ! an arbitrary GNU comment
; ASM-NEXT: !NO_APP
; OBJ-LABEL: <comment_tied>:
; OBJ-NEXT: {{.*}}mov r4,r0
; OBJ-NEXT: {{.*}}rts
; OBJ-NEXT: {{.*}}nop

define i32 @comment_and_immediates(i32 %x) {
  %v = call i32 asm sideeffect "1: # comment ; invalid_is_comment\0A\09add #1,$0\0A\09! SH line comment\0A\09mov #7,$0", "=r,0"(i32 %x)
  ret i32 %v
}

; ASM-LABEL: comment_and_immediates:
; ASM: add #1,r0
; ASM: mov #7,r0
; OBJ-LABEL: <comment_and_immediates>:
; OBJ: 01 70{{.*}}add #1,r0
; OBJ-NEXT: {{.*}}07 e0{{.*}}mov #7,r0
