; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

define i32 @call_pointer(ptr %fn, i32 %value) {
; CHECK-LABEL: call_pointer:
; CHECK: sts.l	pr,@-r15
; CHECK: mov	r4,[[CALLEE:r[0-9]+]]
; CHECK: {{mov(\.l)?}}	{{.*}},r4
; CHECK: jsr	@[[CALLEE]]
; CHECK-NEXT: nop
; CHECK: lds.l	@r15+,pr
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%result = call i32 %fn(i32 %value)
	ret i32 %result
}

define i32 @call_pointer_four(ptr %fn, i32 %a, i32 %b, i32 %c) {
; CHECK-LABEL: call_pointer_four:
; CHECK: sts.l	pr,@-r15
; CHECK: mov	r4,[[FOUR_CALLEE:r[0-9]+]]
; CHECK: mov	#-3,r7
; CHECK: jsr	@[[FOUR_CALLEE]]
; CHECK-NEXT: nop
	%result = call i32 %fn(i32 %a, i32 %b, i32 %c, i32 -3)
	ret i32 %result
}

define ptr @call_pointer_result(ptr %fn, ptr %value) {
; CHECK-LABEL: call_pointer_result:
; CHECK: sts.l	pr,@-r15
; CHECK: mov	r4,[[PTR_CALLEE:r[0-9]+]]
; CHECK: jsr	@[[PTR_CALLEE]]
; CHECK-NEXT: nop
; CHECK: lds.l	@r15+,pr
	%result = call ptr %fn(ptr %value)
	ret ptr %result
}

define void @call_pointer_void(ptr %fn) {
; CHECK-LABEL: call_pointer_void:
; CHECK: sts.l	pr,@-r15
; CHECK: jsr	@r4
; CHECK-NEXT: nop
; CHECK-NOT: mov	r0
; CHECK: lds.l	@r15+,pr
	call void %fn()
	ret void
}
