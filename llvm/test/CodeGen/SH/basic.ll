; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --implicit-check-not=r15 --implicit-check-not=mov.b --implicit-check-not=mov.w --implicit-check-not=mov.l
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --implicit-check-not=r15 --implicit-check-not=mov.b --implicit-check-not=mov.w --implicit-check-not=mov.l
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --implicit-check-not=r15 --implicit-check-not=mov.b --implicit-check-not=mov.w --implicit-check-not=mov.l
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --implicit-check-not=r15 --implicit-check-not=mov.b --implicit-check-not=mov.w --implicit-check-not=mov.l

define i32 @add_i32(i32 %a, i32 %b) {
; CHECK-LABEL: add_i32:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: add	r5,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%sum = add i32 %a, %b
	ret i32 %sum
}

define i32 @identity_i32(i32 %value) {
; CHECK-LABEL: identity_i32:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	ret i32 %value
}

define i32 @minus_one() {
; CHECK-LABEL: minus_one:
; CHECK-NEXT: mov	#-1,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	ret i32 -1
}

define i32 @add_seven(i32 %value) {
; CHECK-LABEL: add_seven:
; CHECK-NEXT: mov	r4,r0
; CHECK-NEXT: add	#7,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%result = add i32 %value, 7
	ret i32 %result
}

define i32 @fourth_arg(i32 %a, i32 %b, i32 %c, i32 %d) {
; CHECK-LABEL: fourth_arg:
; CHECK-NEXT: mov	r7,r0
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	ret i32 %d
}

define void @return_void() {
; CHECK-LABEL: return_void:
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	ret void
}
