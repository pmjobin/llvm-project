; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=sh-isel < %s | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=sh-delay-slot-filler < %s | FileCheck %s --check-prefix=FINAL

define i32 @stack_roundtrip(i32 %value) {
; CHECK-LABEL: stack_roundtrip:
; CHECK-NEXT: add	#-4,r15
; CHECK-NEXT: mov.l	r4,@r15
; CHECK-NEXT: mov.l	@r15,r0
; CHECK-NEXT: add	#4,r15
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%slot = alloca i32, align 4
	store volatile i32 %value, ptr %slot, align 4
	%result = load volatile i32, ptr %slot, align 4
	ret i32 %result
}

define i32 @two_stack_slots(i32 %a, i32 %b) {
; CHECK-LABEL: two_stack_slots:
; CHECK-NEXT: add	#-8,r15
; CHECK-NEXT: mov.l	r4,@(4,r15)
; CHECK-NEXT: mov.l	r5,@r15
; CHECK-NEXT: mov.l	@(4,r15),r{{[01]}}
; CHECK-NEXT: mov.l	@r15,r{{[01]}}
; CHECK-NEXT: add	r1,r0
; CHECK-NEXT: add	#8,r15
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%slot0 = alloca i32, align 4
	%slot1 = alloca i32, align 4
	store volatile i32 %a, ptr %slot0, align 4
	store volatile i32 %b, ptr %slot1, align 4
	%x = load volatile i32, ptr %slot0, align 4
	%y = load volatile i32, ptr %slot1, align 4
	%sum = add i32 %x, %y
	ret i32 %sum
}

define void @frame_60(i32 %value) {
; CHECK-LABEL: frame_60:
; CHECK-NEXT: add	#-60,r15
; CHECK-NEXT: mov.l	r4,@(56,r15)
; CHECK: mov.l	r4,@r15
; CHECK-NEXT: add	#60,r15
; CHECK-NEXT: rts
; CHECK-NEXT: nop
	%s0 = alloca i32, align 4
	%s1 = alloca i32, align 4
	%s2 = alloca i32, align 4
	%s3 = alloca i32, align 4
	%s4 = alloca i32, align 4
	%s5 = alloca i32, align 4
	%s6 = alloca i32, align 4
	%s7 = alloca i32, align 4
	%s8 = alloca i32, align 4
	%s9 = alloca i32, align 4
	%s10 = alloca i32, align 4
	%s11 = alloca i32, align 4
	%s12 = alloca i32, align 4
	%s13 = alloca i32, align 4
	%s14 = alloca i32, align 4
	store volatile i32 %value, ptr %s0, align 4
	store volatile i32 %value, ptr %s1, align 4
	store volatile i32 %value, ptr %s2, align 4
	store volatile i32 %value, ptr %s3, align 4
	store volatile i32 %value, ptr %s4, align 4
	store volatile i32 %value, ptr %s5, align 4
	store volatile i32 %value, ptr %s6, align 4
	store volatile i32 %value, ptr %s7, align 4
	store volatile i32 %value, ptr %s8, align 4
	store volatile i32 %value, ptr %s9, align 4
	store volatile i32 %value, ptr %s10, align 4
	store volatile i32 %value, ptr %s11, align 4
	store volatile i32 %value, ptr %s12, align 4
	store volatile i32 %value, ptr %s13, align 4
	store volatile i32 %value, ptr %s14, align 4
	ret void
}

; ISEL-LABEL: name:            stack_roundtrip
; ISEL: noVRegs:         false
; ISEL: stack:
; ISEL: name: slot
; ISEL-LABEL: body:
; ISEL: MOVL_store_disp %{{[0-9]+}}, %stack.0.slot, 0
; ISEL-NEXT: %{{[0-9]+}}:gpr = MOVL_load_disp %stack.0.slot, 0

; FINAL-LABEL: name:            stack_roundtrip
; FINAL: noVRegs:         true
; FINAL: stackSize:       4
; FINAL-LABEL: body:
; FINAL-NOT: %stack.
; FINAL-NOT: %{{[0-9]+}}
; FINAL: $r15 = frame-setup ADDri $r15, -4
; FINAL-NEXT: MOVL_store_reg killed $r4, $r15
; FINAL-NEXT: $r0 = MOVL_load_reg $r15
; FINAL-NEXT: $r15 = frame-destroy ADDri $r15, 4
; FINAL-NEXT: RTS implicit $pr, implicit killed $r0 {
; FINAL-NEXT: NOP
; FINAL-NEXT: }
