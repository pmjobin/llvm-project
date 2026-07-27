; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define internal i32 @callee2(i32 %a, i32 %b) {
	%result = add i32 %a, %b
	ret i32 %result
}

define internal i32 @callee3(i32 %a, i32 %b, i32 %c) {
	%first = add i32 %a, %b
	%result = add i32 %first, %c
	ret i32 %result
}

define internal i32 @callee4(i32 %a, i32 %b, i32 %c, i32 %d) {
	%first = add i32 %a, %b
	%second = add i32 %c, %d
	%result = add i32 %first, %second
	ret i32 %result
}

define internal ptr @identity_pointer(ptr %value) {
	ret ptr %value
}

define i32 @call_swapped(i32 %a, i32 %b) {
; CHECK-LABEL: call_swapped:
; CHECK: sts.l	pr,@-r15
; CHECK: mov	r4,{{r[01589]}}
; CHECK: {{mov(\.l)?}}	{{.*}},r4
; CHECK: bsr	callee2
; CHECK-NEXT: nop
	%result = call i32 @callee2(i32 %b, i32 %a)
	ret i32 %result
}

define i32 @call_three(i32 %a, i32 %b, i32 %c) {
; CHECK-LABEL: call_three:
; CHECK: sts.l	pr,@-r15
; CHECK: bsr	callee3
; CHECK-NEXT: nop
	%result = call i32 @callee3(i32 %a, i32 %b, i32 %c)
	ret i32 %result
}

define i32 @call_four_mixed(i32 %a, i32 %b, i32 %c, i32 %d) {
; CHECK-LABEL: call_four_mixed:
; CHECK: sts.l	pr,@-r15
; CHECK: mov	#-7,r5
; CHECK: mov	#12,r7
; CHECK: bsr	callee4
; CHECK-NEXT: nop
	%result = call i32 @callee4(i32 %d, i32 -7, i32 %b, i32 12)
	ret i32 %result
}

define i32 @call_four_repeated(i32 %value) {
; CHECK-LABEL: call_four_repeated:
; CHECK: sts.l	pr,@-r15
; CHECK: mov
; CHECK: mov
; CHECK: mov
; CHECK: bsr	callee4
; CHECK-NEXT: nop
	%result = call i32 @callee4(i32 %value, i32 %value, i32 %value, i32 %value)
	ret i32 %result
}

define ptr @call_pointer_argument_and_result(ptr %value) {
; CHECK-LABEL: call_pointer_argument_and_result:
; CHECK: sts.l	pr,@-r15
; CHECK: bsr	identity_pointer
; CHECK-NEXT: nop
; CHECK: lds.l	@r15+,pr
	%result = call ptr @identity_pointer(ptr %value)
	ret ptr %result
}
