; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-hard-float -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s

define i32 @preserve_i32_result(ptr %lhs, ptr %rhs) {
; CHECK-LABEL: preserve_i32_result:
; CHECK-NOT: r15
; CHECK: fcmp/eq
; CHECK-NEXT: movt	[[RESULT:r[0-9]+]]
; CHECK-NEXT: mov	[[RESULT]],[[SAVED:r[0-9]+]]
; CHECK: sts	fpscr,r0
; CHECK: rts
; CHECK-NEXT: mov	[[SAVED]],r0
	%a = load float, ptr %lhs, align 4
	%b = load float, ptr %rhs, align 4
	%condition = fcmp oeq float %a, %b
	%result = zext i1 %condition to i32
	ret i32 %result
}

define i64 @preserve_i64_result(ptr %lhs, ptr %rhs) {
; CHECK-LABEL: preserve_i64_result:
; CHECK-NOT: r15
; CHECK: mov	#0,[[ZERO:r[0-9]+]]
; CHECK: mov	[[ZERO]],[[SAVED_HIGH:r[0-9]+]]
; CHECK: fcmp/eq
; CHECK: movt	[[RESULT:r[0-9]+]]
; CHECK: mov	[[RESULT]],[[SAVED_LOW:r[0-9]+]]
; CHECK: sts	fpscr,r0
; CHECK: mov	[[SAVED_HIGH]],r1
; CHECK: rts
; CHECK-NEXT: mov	[[SAVED_LOW]],r0
	%a = load float, ptr %lhs, align 4
	%b = load float, ptr %rhs, align 4
	%condition = fcmp oeq float %a, %b
	%result = zext i1 %condition to i64
	ret i64 %result
}
