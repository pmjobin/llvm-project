; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -code-model=large -relocation-model=static -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -code-model=large -relocation-model=static -O0 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

declare i32 @external_fn(i32)
declare i64 @external_i64(i64, i32, i32, i32, i32)
declare i32 @external19(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)

define i32 @call_external(i32 %x) {
; CHECK-LABEL: call_external:
; CHECK-NEXT: sts.l	pr,@-r15
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,[[CALLEE:r[0-9]+]]
; CHECK: jsr	@[[CALLEE]]
; CHECK-NEXT: nop
; CHECK: lds.l	@r15+,pr
; CHECK: rts
; CHECK-NEXT: nop
; CHECK: .LCPI{{[0-9]+}}_0_0:
; CHECK-NEXT: .long	external_fn
	%result = call i32 @external_fn(i32 %x)
	ret i32 %result
}

define i64 @call_external_i64(i64 %value) {
; CHECK-LABEL: call_external_i64:
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,[[CALLEE:r[0-9]+]]
; CHECK: jsr	@[[CALLEE]]
; CHECK-NEXT: nop
; CHECK: .long	external_i64
	%result = call i64 @external_i64(i64 %value, i32 1, i32 2, i32 3, i32 4)
	ret i64 %result
}

define i32 @call_external_twice(i32 %x) {
; CHECK-LABEL: call_external_twice:
; CHECK-COUNT-2: jsr
; CHECK-COUNT-1: .LCPI{{[0-9]+}}_0_0:
; CHECK-NEXT: .long	external_fn
	%first = call i32 @external_fn(i32 %x)
	%second = call i32 @external_fn(i32 %first)
	ret i32 %second
}

define i32 @call_external19() {
; CHECK-LABEL: call_external19:
; CHECK: add	#-60,r15
; CHECK: mov.l	.LCPI{{[0-9]+}}_0_0,[[CALLEE:r[0-9]+]]
; CHECK: jsr	@[[CALLEE]]
; CHECK-NEXT: nop
; CHECK: add	#60,r15
; CHECK: .long	external19
	%result = call i32 @external19(i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
	ret i32 %result
}
