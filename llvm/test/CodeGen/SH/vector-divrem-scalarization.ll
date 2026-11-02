; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2a -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4a -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O0 -verify-machineinstrs %s -o - | FileCheck %s

define void @sdiv_v4i32(ptr %output, ptr %left, ptr %right) optsize {
; CHECK-LABEL: sdiv_v4i32:
; CHECK-COUNT-4: jsr	@r0
; CHECK: .long	__divsi3
	%left.value = load <4 x i32>, ptr %left, align 4
	%right.value = load <4 x i32>, ptr %right, align 4
	%result = sdiv <4 x i32> %left.value, %right.value
	store <4 x i32> %result, ptr %output, align 4
	ret void
}

define void @udiv_v2i32(ptr %output, ptr %left, ptr %right) optsize {
; CHECK-LABEL: udiv_v2i32:
; CHECK-COUNT-2: jsr	@r0
; CHECK: .long	__udivsi3
	%left.value = load <2 x i32>, ptr %left, align 4
	%right.value = load <2 x i32>, ptr %right, align 4
	%result = udiv <2 x i32> %left.value, %right.value
	store <2 x i32> %result, ptr %output, align 4
	ret void
}

define void @srem_v2i32(ptr %output, ptr %left, ptr %right) optsize {
; CHECK-LABEL: srem_v2i32:
; CHECK-COUNT-2: jsr	@r0
; CHECK: .long	__modsi3
	%left.value = load <2 x i32>, ptr %left, align 4
	%right.value = load <2 x i32>, ptr %right, align 4
	%result = srem <2 x i32> %left.value, %right.value
	store <2 x i32> %result, ptr %output, align 4
	ret void
}

define void @urem_v2i8(ptr %output, ptr %left, ptr %right) optsize {
; CHECK-LABEL: urem_v2i8:
; CHECK-COUNT-2: jsr	@r0
; CHECK: .long	__umodsi3
	%left.value = load <2 x i8>, ptr %left, align 1
	%right.value = load <2 x i8>, ptr %right, align 1
	%result = urem <2 x i8> %left.value, %right.value
	store <2 x i8> %result, ptr %output, align 1
	ret void
}
