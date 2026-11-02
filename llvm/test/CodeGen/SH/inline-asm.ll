; RUN: llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -O0 -verify-machineinstrs %s -o - | FileCheck %s

@external_data = external global i32

define i32 @inline_asm_preserves_declared_got_state() {
entry:
	call void asm sideeffect "", ""()
	%value = load i32, ptr @external_data, align 4
	ret i32 %value
}

define i32 @inline_asm_size_is_modeled() {
entry:
	call void asm sideeffect "nop\0A\09.space 8", ""()
	%value = load i32, ptr @external_data, align 4
	ret i32 %value
}

define i32 @inline_add(i32 %lhs, i32 %rhs) {
entry:
	%result = call i32 asm "add $2,$0", "=r,0,r"(i32 %lhs, i32 %rhs)
	ret i32 %result
}

define i32 @inline_immediate() {
entry:
	%result = call i32 asm "mov #$1,$0", "=r,i"(i32 7)
	ret i32 %result
}

; CHECK-LABEL: inline_add:
; CHECK: !APP
; CHECK-NEXT: add r{{[0-9]+}},r{{[0-9]+}}
; CHECK-NEXT: !NO_APP

; CHECK-LABEL: inline_immediate:
; CHECK: !APP
; CHECK-NEXT: mov #7,r0
; CHECK-NEXT: !NO_APP
