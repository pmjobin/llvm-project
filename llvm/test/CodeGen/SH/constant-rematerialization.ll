; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=virt-reg-rewriter < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=virt-reg-rewriter < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O2 -verify-machineinstrs -stop-after=virt-reg-rewriter < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs -stop-after=virt-reg-rewriter < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs -filetype=obj < %s -o /dev/null
;
; A constant live across an asm that clobbers every allocatable GPR should be
; recreated at its next use. It needs no spill slot. Both literal loads refer
; to the same immutable pool entry; island placement still runs afterwards.

; CHECK-LABEL: name: immediate_constant
; CHECK: stack: []
; CHECK: MOVri 42
; CHECK-NEXT: MOVL_store_reg
; CHECK-NEXT: INLINEASM
; CHECK-NEXT: $r0 = MOVri 42
; CHECK-NEXT: RTS
define i32 @immediate_constant(ptr %p) optsize {
	store volatile i32 42, ptr %p, align 4
	call void asm sideeffect "", "~{r0},~{r1},~{r2},~{r3},~{r4},~{r5},~{r6},~{r7},~{r8},~{r9},~{r10},~{r11},~{r12},~{r13},~{r14}"()
	ret i32 42
}

; CHECK-LABEL: name: word_constant
; CHECK: stack: []
; CHECK: MOVW_load_pc_island %const.[[WORD:[0-9]+]], -1
; CHECK-NEXT: MOVL_store_reg
; CHECK-NEXT: INLINEASM
; CHECK-NEXT: $r0 = MOVW_load_pc_island %const.[[WORD]], -1
; CHECK-NEXT: RTS
define i32 @word_constant(ptr %p) optsize {
	store volatile i32 30000, ptr %p, align 4
	call void asm sideeffect "", "~{r0},~{r1},~{r2},~{r3},~{r4},~{r5},~{r6},~{r7},~{r8},~{r9},~{r10},~{r11},~{r12},~{r13},~{r14}"()
	ret i32 30000
}

; CHECK-LABEL: name: long_constant
; CHECK: stack: []
; CHECK: MOVL_load_pc_island %const.[[LONG:[0-9]+]], -1
; CHECK-NEXT: MOVL_store_reg
; CHECK-NEXT: INLINEASM
; CHECK-NEXT: $r0 = MOVL_load_pc_island %const.[[LONG]], -1
; CHECK-NEXT: RTS
define i32 @long_constant(ptr %p) optsize {
	store volatile i32 305419896, ptr %p, align 4
	call void asm sideeffect "", "~{r0},~{r1},~{r2},~{r3},~{r4},~{r5},~{r6},~{r7},~{r8},~{r9},~{r10},~{r11},~{r12},~{r13},~{r14}"()
	ret i32 305419896
}

; An ordinary volatile load must preserve the original value across the asm.
; CHECK-LABEL: name: varying_load
; CHECK: stack:
; CHECK: type: spill-slot
; CHECK: MOVL_load_reg
; CHECK: MOVL_store_disp {{.*}}%stack.0
; CHECK: INLINEASM
; CHECK: MOVL_load_disp %stack.0
define i32 @varying_load(ptr %p) optsize {
	%value = load volatile i32, ptr %p, align 4
	call void asm sideeffect "", "~{r0},~{r1},~{r2},~{r3},~{r4},~{r5},~{r6},~{r7},~{r8},~{r9},~{r10},~{r11},~{r12},~{r13},~{r14},~{memory}"()
	ret i32 %value
}

declare void @clobber()

; Immediate regeneration also replaces an uncoalescible return-register copy.
; This needs no callee-saved register even when allocation could provide one.
; CHECK-LABEL: name: immediate_across_call
; CHECK: stack: []
; CHECK: $r0 = MOVri 42
; CHECK: MOVL_store_reg killed $r0
; CHECK: JSR
; CHECK: $r0 = MOVri 42
; CHECK-NEXT: RTS
define i32 @immediate_across_call(ptr %p) optsize {
	store volatile i32 42, ptr %p, align 4
	call void @clobber()
	ret i32 42
}
