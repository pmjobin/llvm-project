; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

@global = global i32 0, align 4
declare void @external_function()

define i32 @global_equal(ptr %value) {
; CHECK-LABEL: global_equal:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0,[[ADDR:r[0-9]+]]
; CHECK: cmp/eq	[[ADDR]],r4
; CHECK: {{b[tf]}}
; CHECK: .long	global
	%condition = icmp eq ptr %value, @global
	br i1 %condition, label %yes, label %no
yes:
	ret i32 1
no:
	ret i32 0
}

define i32 @function_not_null() {
; CHECK-LABEL: function_not_null:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0,[[ADDR:r[0-9]+]]
; CHECK: tst	[[ADDR]],[[ADDR]]
; CHECK: {{b[tf]}}
; CHECK: .long	external_function
	%condition = icmp ne ptr @external_function, null
	br i1 %condition, label %yes, label %no
yes:
	ret i32 1
no:
	ret i32 0
}

define i32 @function_equal(ptr %value) {
; CHECK-LABEL: function_equal:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0,[[ADDR:r[0-9]+]]
; CHECK: cmp/eq
; CHECK: .long	external_function
	%condition = icmp eq ptr %value, @external_function
	br i1 %condition, label %yes, label %no
yes:
	ret i32 1
no:
	ret i32 0
}
