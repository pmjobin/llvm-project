; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s

define void @sub_i8(ptr %left, ptr %right, ptr %out) {
; CHECK-LABEL: sub_i8:
; CHECK: mov.b	@
; CHECK: mov.b	@
; CHECK: sub
; CHECK: mov.b	{{r[0-9]+}},@
	%a = load i8, ptr %left, align 1
	%b = load i8, ptr %right, align 1
	%result = sub i8 %a, %b
	store i8 %result, ptr %out, align 1
	ret void
}

define void @sub_i16(ptr %left, ptr %right, ptr %out) {
; CHECK-LABEL: sub_i16:
; CHECK: mov.w	@
; CHECK: mov.w	@
; CHECK: sub
; CHECK: mov.w	{{r[0-9]+}},@
	%a = load i16, ptr %left, align 2
	%b = load i16, ptr %right, align 2
	%result = sub i16 %a, %b
	store i16 %result, ptr %out, align 2
	ret void
}

define void @and_i8(ptr %left, ptr %right, ptr %out) {
; CHECK-LABEL: and_i8:
; CHECK: and
; CHECK: mov.b	{{r[0-9]+}},@
	%a = load i8, ptr %left, align 1
	%b = load i8, ptr %right, align 1
	%result = and i8 %a, %b
	store i8 %result, ptr %out, align 1
	ret void
}

define void @and_i16(ptr %left, ptr %right, ptr %out) {
; CHECK-LABEL: and_i16:
; CHECK: and
; CHECK: mov.w	{{r[0-9]+}},@
	%a = load i16, ptr %left, align 2
	%b = load i16, ptr %right, align 2
	%result = and i16 %a, %b
	store i16 %result, ptr %out, align 2
	ret void
}

define void @or_i8(ptr %left, ptr %right, ptr %out) {
; CHECK-LABEL: or_i8:
; CHECK: or
; CHECK: mov.b	{{r[0-9]+}},@
	%a = load i8, ptr %left, align 1
	%b = load i8, ptr %right, align 1
	%result = or i8 %a, %b
	store i8 %result, ptr %out, align 1
	ret void
}

define void @or_i16(ptr %left, ptr %right, ptr %out) {
; CHECK-LABEL: or_i16:
; CHECK: or
; CHECK: mov.w	{{r[0-9]+}},@
	%a = load i16, ptr %left, align 2
	%b = load i16, ptr %right, align 2
	%result = or i16 %a, %b
	store i16 %result, ptr %out, align 2
	ret void
}

define void @xor_i8(ptr %left, ptr %right, ptr %out) {
; CHECK-LABEL: xor_i8:
; CHECK: xor
; CHECK: mov.b	{{r[0-9]+}},@
	%a = load i8, ptr %left, align 1
	%b = load i8, ptr %right, align 1
	%result = xor i8 %a, %b
	store i8 %result, ptr %out, align 1
	ret void
}

define void @xor_i16(ptr %left, ptr %right, ptr %out) {
; CHECK-LABEL: xor_i16:
; CHECK: xor
; CHECK: mov.w	{{r[0-9]+}},@
	%a = load i16, ptr %left, align 2
	%b = load i16, ptr %right, align 2
	%result = xor i16 %a, %b
	store i16 %result, ptr %out, align 2
	ret void
}

define void @shl_i8(ptr %in, ptr %out) {
; CHECK-LABEL: shl_i8:
; CHECK: mov.b	@
; CHECK: shll2
; CHECK: shll
; CHECK: mov.b	{{r[0-9]+}},@
	%value = load i8, ptr %in, align 1
	%result = shl i8 %value, 3
	store i8 %result, ptr %out, align 1
	ret void
}

define void @shl_i16(ptr %in, ptr %out) {
; CHECK-LABEL: shl_i16:
; CHECK: mov.w	@
; CHECK: shll8
; CHECK: mov.w	{{r[0-9]+}},@
	%value = load i16, ptr %in, align 2
	%result = shl i16 %value, 8
	store i16 %result, ptr %out, align 2
	ret void
}

define void @lshr_i8(ptr %in, ptr %out) {
; CHECK-LABEL: lshr_i8:
; CHECK: mov.b	@
; CHECK: extu.b
; CHECK: shlr2
; CHECK: shlr
; CHECK: mov.b	{{r[0-9]+}},@
	%value = load i8, ptr %in, align 1
	%result = lshr i8 %value, 3
	store i8 %result, ptr %out, align 1
	ret void
}

define void @lshr_i16(ptr %in, ptr %out) {
; CHECK-LABEL: lshr_i16:
; CHECK: mov.b	@
; CHECK: extu.b
; CHECK: mov.w	{{r[0-9]+}},@
	%value = load i16, ptr %in, align 2
	%result = lshr i16 %value, 8
	store i16 %result, ptr %out, align 2
	ret void
}

define void @ashr_i8(ptr %in, ptr %out) {
; CHECK-LABEL: ashr_i8:
; CHECK: mov.b	@
; CHECK: shlr2
; CHECK: shlr
; CHECK: mov.b	{{r[0-9]+}},@
	%value = load i8, ptr %in, align 1
	%result = ashr i8 %value, 3
	store i8 %result, ptr %out, align 1
	ret void
}

define void @ashr_i16(ptr %in, ptr %out) {
; CHECK-LABEL: ashr_i16:
; CHECK: mov.b	@
; CHECK: mov.w	{{r[0-9]+}},@
	%value = load i16, ptr %in, align 2
	%result = ashr i16 %value, 8
	store i16 %result, ptr %out, align 2
	ret void
}
