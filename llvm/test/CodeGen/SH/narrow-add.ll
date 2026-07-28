; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s

define void @add_i8(ptr %p) {
; CHECK-LABEL: add_i8:
; CHECK: mov.b	@r4,[[VALUE:r[0-9]+]]
; CHECK-NEXT: add	#1,[[VALUE]]
; CHECK-NEXT: mov.b	[[VALUE]],@r4
; CHECK: rts
	%value = load i8, ptr %p, align 1
	%sum = add i8 %value, 1
	store i8 %sum, ptr %p, align 1
	ret void
}

define void @add_i16(ptr %p) {
; CHECK-LABEL: add_i16:
; CHECK: mov.w	@r4,[[VALUE:r[0-9]+]]
; CHECK-NEXT: add	#1,[[VALUE]]
; CHECK-NEXT: mov.w	[[VALUE]],@r4
; CHECK: rts
	%value = load i16, ptr %p, align 2
	%sum = add i16 %value, 1
	store i16 %sum, ptr %p, align 2
	ret void
}

define i32 @add_i8_signed_compare(ptr %p) {
; CHECK-LABEL: add_i8_signed_compare:
; CHECK: mov.b	@r4,[[VALUE:r[0-9]+]]
; CHECK: add	#1,[[VALUE]]
; CHECK: exts.b	[[VALUE]],[[SIGNED:r[0-9]+]]
; CHECK: cmp/ge
	%value = load i8, ptr %p, align 1
	%sum = add i8 %value, 1
	%compare = icmp sgt i8 %sum, 0
	br i1 %compare, label %positive, label %nonpositive
positive:
	ret i32 1
nonpositive:
	ret i32 0
}

define i32 @add_i16_unsigned_compare(ptr %p) {
; CHECK-LABEL: add_i16_unsigned_compare:
; CHECK: mov.w	@r4,[[VALUE:r[0-9]+]]
; CHECK: add	#1,[[VALUE]]
; CHECK: extu.w	[[VALUE]],[[UNSIGNED:r[0-9]+]]
; CHECK: cmp/hs
	%value = load i16, ptr %p, align 2
	%sum = add i16 %value, 1
	%compare = icmp ugt i16 %sum, 1
	br i1 %compare, label %greater, label %not_greater
greater:
	ret i32 1
not_greater:
	ret i32 0
}
