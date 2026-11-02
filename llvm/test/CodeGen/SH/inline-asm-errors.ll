; RUN: llc -mtriple=sh-unknown-elf -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -O0 -verify-machineinstrs %s -o - | FileCheck %s

@global = global i32 0

define i32 @memory_constraint() {
entry:
	%value = call i32 asm "mov.l $1,$0", "=r,*m"(ptr elementtype(i32) @global)
	ret i32 %value
}

; CHECK-LABEL: memory_constraint:
; CHECK: mov.l {{.*}},r0
; CHECK: mov.l @r0,r0
