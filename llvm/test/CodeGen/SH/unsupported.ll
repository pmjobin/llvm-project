; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -verify-machineinstrs < %t/call.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CALL

;--- call.ll

declare i32 @callee(i32)

define i32 @caller(i32 %value) {
	%result = call i32 @callee(i32 %value)
	ret i32 %result
}

; CALL: LLVM ERROR: SH unresolved or interposable direct calls are not supported
