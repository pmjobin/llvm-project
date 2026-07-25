; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -verify-machineinstrs < %t/call.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CALL
; RUN: not --crash llc -mtriple=sh-unknown-elf -mcpu=sh2 -verify-machineinstrs < %t/stack-arg.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=STACK

;--- call.ll

declare i32 @callee(i32)

define i32 @caller(i32 %value) {
	%result = call i32 @callee(i32 %value)
	ret i32 %result
}

; CALL: LLVM ERROR: SH function calls are not supported

;--- stack-arg.ll

define i32 @fifth_arg(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e) {
	ret i32 %e
}

; STACK: LLVM ERROR: SH stack-passed arguments are not supported
