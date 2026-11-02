; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O3 -verify-machineinstrs < %s | FileCheck %s --check-prefix=DYNAMIC
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=O0

declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i32 @llvm.fshr.i32(i32, i32, i32)

; The wide-product experiment is omitted. The count and source stay live
; across both conventional shift helpers, with freeze semantics retained.
; CHECK-LABEL: left_live:
; CHECK-COUNT-2: jsr
; CHECK: or
; CHECK-NOT: dmulu.l
; CHECK: rts
; DYNAMIC-LABEL: left_live:
; DYNAMIC-NOT: jsr
; DYNAMIC-NOT: dmulu.l
; DYNAMIC: rts
; O0-LABEL: left_live:
; O0-NOT: dmulu.l
; O0: rts
define i32 @left_live(i32 %value, i32 %count) {
	%v = freeze i32 %value
	%c = freeze i32 %count
	%r = call i32 @llvm.fshl.i32(i32 %v, i32 %v, i32 %c)
	%sum = add i32 %r, %c
	%result = xor i32 %sum, %v
	ret i32 %result
}

; CHECK-LABEL: right_live:
; CHECK: jsr
; CHECK: neg
; CHECK: jsr
; CHECK: or
; CHECK: rts
define i32 @right_live(i32 %value, i32 %count) {
	%c = freeze i32 %count
	%r = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 %c)
	%result = add i32 %r, %c
	ret i32 %result
}

; CHECK-LABEL: sized:
; CHECK-NOT: dmulu.l
; CHECK: rts
define i32 @sized(i32 %value, i32 %count) optsize {
	%r = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 %count)
	%result = add i32 %r, %count
	ret i32 %result
}

; CHECK-LABEL: constant:
; CHECK-NOT: jsr
; CHECK-NOT: dmulu.l
; CHECK: rts
define i32 @constant(i32 %value) {
	%r = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 31)
	ret i32 %r
}

; CHECK-LABEL: short:
; CHECK-NOT: jsr
; CHECK-NOT: dmulu.l
; CHECK: rts
define i32 @short(i32 %value, i32 %count) {
	%c = and i32 %count, 15
	%r = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 %c)
	ret i32 %r
}

; Distinct funnel inputs must retain the ordinary funnel/shift semantics.
; CHECK-LABEL: distinct:
; CHECK-NOT: dmulu.l
; CHECK: rts
define i32 @distinct(i32 %left, i32 %right, i32 %count) {
	%r = call i32 @llvm.fshl.i32(i32 %left, i32 %right, i32 %count)
	ret i32 %r
}
