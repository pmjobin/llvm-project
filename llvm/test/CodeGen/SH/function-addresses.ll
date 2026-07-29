; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

declare void @external_function()
declare void @consume_function_pointer(ptr)

@function_pointer = global ptr @external_function, align 4

define internal void @internal_function() {
	ret void
}

define private void @private_function() {
	ret void
}

define void @global_function() {
	ret void
}

define ptr @defined_address() {
; CHECK-LABEL: defined_address:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0,r0
; CHECK: .long	internal_function
	ret ptr @internal_function
}

define ptr @private_address() {
; CHECK-LABEL: private_address:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0,r0
; CHECK: .long	{{.*}}private_function
	ret ptr @private_function
}

define ptr @global_function_address() {
; CHECK-LABEL: global_function_address:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0,r0
; CHECK: .long	global_function
	ret ptr @global_function
}

define ptr @undefined_address() {
; CHECK-LABEL: undefined_address:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0,r0
; CHECK: .long	external_function
	ret ptr @external_function
}

define void @store_function_address(ptr %slot) {
; CHECK-LABEL: store_function_address:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0,
; CHECK: mov.l	{{.*}},@r4
; CHECK: .long	internal_function
	store ptr @internal_function, ptr %slot, align 4
	ret void
}

define void @call_function_pointer() {
; CHECK-LABEL: call_function_pointer:
; CHECK: sts.l	pr,@-r15
; CHECK: mov.l	.LCPI{{[0-9]+}}_0,[[CALLEE:r[0-9]+]]
; CHECK: jsr	@[[CALLEE]]
; CHECK-NEXT: nop
; CHECK: .long	external_function
	%fn = inttoptr i32 ptrtoint (ptr @external_function to i32) to ptr
	call void %fn()
	ret void
}

define void @call_loaded_function_pointer() {
; CHECK-LABEL: call_loaded_function_pointer:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0,[[SLOT:r[0-9]+]]
; CHECK: mov.l	@[[SLOT]],[[CALLEE:r[0-9]+]]
; CHECK: jsr	@[[CALLEE]]
; CHECK-NEXT: nop
; CHECK: .long	function_pointer
	%fn = load ptr, ptr @function_pointer, align 4
	call void %fn()
	ret void
}

define void @pass_function_address() {
; CHECK-LABEL: pass_function_address:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0,
; CHECK: mov.l	.LCPI{{[0-9]+}}_1,
; CHECK: jsr
; CHECK: .long	external_function
; CHECK: .long	consume_function_pointer
	call void @consume_function_pointer(ptr @external_function)
	ret void
}
