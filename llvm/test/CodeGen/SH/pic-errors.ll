; RUN: not --crash llc -mtriple=sh-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs %s -o /dev/null 2>&1 | FileCheck %s
; RUN: not --crash llc -mtriple=shle-unknown-elf -relocation-model=pic -O0 -verify-machineinstrs %s -o /dev/null 2>&1 | FileCheck %s

@external_data = external global i32

define i32 @unknown_inline_asm_got_state() {
entry:
	call void asm sideeffect "", ""()
	%value = load i32, ptr @external_data, align 4
	ret i32 %value
}

; CHECK: LLVM ERROR: SH inline assembly is not supported
