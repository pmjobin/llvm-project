; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=virt-reg-rewriter < %s | FileCheck %s --check-prefix=RA
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=sh-delay-slot-filler < %s | FileCheck %s --check-prefix=FINAL
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=asm -asm-verbose=false < %s | FileCheck %s --check-prefix=ASM

define i32 @force_gpr_spill(ptr %p) nounwind {
	%p0 = getelementptr i8, ptr %p, i32 0
	%p1 = getelementptr i8, ptr %p, i32 4
	%p2 = getelementptr i8, ptr %p, i32 8
	%p3 = getelementptr i8, ptr %p, i32 12
	%p4 = getelementptr i8, ptr %p, i32 16
	%p5 = getelementptr i8, ptr %p, i32 20
	%p6 = getelementptr i8, ptr %p, i32 24
	%p7 = getelementptr i8, ptr %p, i32 28
	%p8 = getelementptr i8, ptr %p, i32 32
	%p9 = getelementptr i8, ptr %p, i32 36
	%p10 = getelementptr i8, ptr %p, i32 40
	%p11 = getelementptr i8, ptr %p, i32 44
	%p12 = getelementptr i8, ptr %p, i32 48
	%p13 = getelementptr i8, ptr %p, i32 52
	%p14 = getelementptr i8, ptr %p, i32 56
	%p15 = getelementptr i8, ptr %p, i32 60
	%v0 = load volatile i32, ptr %p0, align 4
	%v1 = load volatile i32, ptr %p1, align 4
	%v2 = load volatile i32, ptr %p2, align 4
	%v3 = load volatile i32, ptr %p3, align 4
	%v4 = load volatile i32, ptr %p4, align 4
	%v5 = load volatile i32, ptr %p5, align 4
	%v6 = load volatile i32, ptr %p6, align 4
	%v7 = load volatile i32, ptr %p7, align 4
	%v8 = load volatile i32, ptr %p8, align 4
	%v9 = load volatile i32, ptr %p9, align 4
	%v10 = load volatile i32, ptr %p10, align 4
	%v11 = load volatile i32, ptr %p11, align 4
	%v12 = load volatile i32, ptr %p12, align 4
	%v13 = load volatile i32, ptr %p13, align 4
	%v14 = load volatile i32, ptr %p14, align 4
	%v15 = load volatile i32, ptr %p15, align 4
	store volatile i32 %v15, ptr %p0, align 4
	store volatile i32 %v14, ptr %p1, align 4
	store volatile i32 %v13, ptr %p2, align 4
	store volatile i32 %v12, ptr %p3, align 4
	store volatile i32 %v11, ptr %p4, align 4
	store volatile i32 %v10, ptr %p5, align 4
	store volatile i32 %v9, ptr %p6, align 4
	store volatile i32 %v8, ptr %p7, align 4
	store volatile i32 %v7, ptr %p8, align 4
	store volatile i32 %v6, ptr %p9, align 4
	store volatile i32 %v5, ptr %p10, align 4
	store volatile i32 %v4, ptr %p11, align 4
	store volatile i32 %v3, ptr %p12, align 4
	store volatile i32 %v2, ptr %p13, align 4
	store volatile i32 %v1, ptr %p14, align 4
	store volatile i32 %v0, ptr %p15, align 4
	ret i32 %v0
}

; RA-LABEL: name:            force_gpr_spill
; RA: noVRegs:         true
; RA: stack:
; RA: id: 0
; RA-SAME: type: spill-slot
; RA: id: 1
; RA-SAME: type: spill-slot
; RA: MOVL_store_disp killed ${{r[0-9]+}}, %stack.{{[0-9]+}}, 0
; RA: ${{r[0-9]+}} = MOVL_load_disp %stack.{{[0-9]+}}, 0

; FINAL-LABEL: name:            force_gpr_spill
; FINAL: noVRegs:         true
; FINAL: frameInfo:
; FINAL: stackSize:       36
; FINAL-DAG: callee-saved-register: '$r8'
; FINAL-DAG: callee-saved-register: '$r9'
; FINAL-DAG: callee-saved-register: '$r10'
; FINAL-DAG: callee-saved-register: '$r11'
; FINAL-DAG: callee-saved-register: '$r12'
; FINAL-DAG: callee-saved-register: '$r13'
; FINAL-DAG: callee-saved-register: '$r14'
; FINAL-LABEL: body:
; FINAL-NOT: %{{[0-9]+}}
; FINAL: early-clobber $r15 = frame-setup MOVL_store_predec killed $r8, $r15
; FINAL-NEXT: early-clobber $r15 = frame-setup MOVL_store_predec killed $r9, $r15
; FINAL-NEXT: early-clobber $r15 = frame-setup MOVL_store_predec killed $r10, $r15
; FINAL-NEXT: early-clobber $r15 = frame-setup MOVL_store_predec killed $r11, $r15
; FINAL-NEXT: early-clobber $r15 = frame-setup MOVL_store_predec killed $r12, $r15
; FINAL-NEXT: early-clobber $r15 = frame-setup MOVL_store_predec killed $r13, $r15
; FINAL-NEXT: early-clobber $r15 = frame-setup MOVL_store_predec killed $r14, $r15
; FINAL-NEXT: $r15 = frame-setup ADDri $r15, -8
; FINAL: [[RELOAD:\$r[0-9]+]] = MOVL_load_reg $r15
; FINAL: [[RELOAD]] = MOVL_load_disp $r15, 4
; FINAL: $r15 = frame-destroy ADDri $r15, 8
; FINAL-NEXT: early-clobber $r14, $r15 = frame-destroy MOVL_load_postinc $r15
; FINAL-NEXT: early-clobber $r13, $r15 = frame-destroy MOVL_load_postinc $r15
; FINAL-NEXT: early-clobber $r12, $r15 = frame-destroy MOVL_load_postinc $r15
; FINAL-NEXT: early-clobber $r11, $r15 = frame-destroy MOVL_load_postinc $r15
; FINAL-NEXT: early-clobber $r10, $r15 = frame-destroy MOVL_load_postinc $r15
; FINAL-NEXT: early-clobber $r9, $r15 = frame-destroy MOVL_load_postinc $r15
; FINAL-NEXT: RTS implicit $pr, implicit killed $r0 {
; FINAL-NEXT: early-clobber $r8, $r15 = frame-destroy MOVL_load_postinc $r15
; FINAL-NEXT: }

; ASM-LABEL: force_gpr_spill:
; ASM: mov.l	r8,@-r15
; ASM: add	#-8,r15
; ASM: mov.l	@r15,{{r[0-9]+}}
; ASM: add	#8,r15
; ASM-NEXT: mov.l	@r15+,r14
; ASM: mov.l	@r15+,r9
; ASM-NEXT: rts
; ASM-NEXT: mov.l	@r15+,r8
