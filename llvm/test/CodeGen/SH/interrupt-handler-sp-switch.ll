; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -relocation-model=static -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -relocation-model=static -O2 -verify-machineinstrs %s -o - | FileCheck %s

@value = external global i32
@count = external global i32

declare void @helper(i32, i32, i32, i32, i32, i32)
declare void @sink(ptr)

define void @empty() #0 {
	ret void
}

define void @used() #0 {
	call void asm sideeffect "", "~{r0},~{r8},~{mach},~{macl}"()
	ret void
}

define void @nonleaf() #0 {
	%loaded = load volatile i32, ptr @value
	call void @helper(i32 %loaded, i32 2, i32 3, i32 4, i32 5, i32 6)
	ret void
}

define void @frame() #0 {
	%storage = alloca [64 x i8], align 4
	call void @sink(ptr %storage)
	ret void
}

define void @large_frame() #0 {
	%storage = alloca [512 x i8], align 4
	call void @sink(ptr %storage)
	ret void
}

define void @dynamic_frame() #0 {
	%size = load volatile i32, ptr @count
	%storage = alloca i8, i32 %size, align 4
	call void @sink(ptr %storage)
	ret void
}

attributes #0 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-sp-switch"="alternate_stack" "target-abi"="gnu" }

; CHECK-LABEL: empty:
; CHECK-NEXT: ! %bb.0:
; CHECK-NEXT: mov.l r0,@-r15
; CHECK-NEXT: mov.l [[STACK_LITERAL:\.L[^,]+]],r0
; CHECK-NEXT: mov.l @r0,r0
; CHECK-NEXT: mov.l r15,@-r0
; CHECK-NEXT: mov r0,r15
; CHECK-NEXT: mov.l @r15,r15
; CHECK-NEXT: mov.l @r15+,r0
; CHECK-NEXT: rte
; CHECK-NEXT: nop
; CHECK: [[STACK_LITERAL]]:
; CHECK-NEXT: .long alternate_stack

; CHECK-LABEL: used:
; CHECK: mov.l r0,@-r15
; CHECK: mov r0,r15
; CHECK-NOT: mov.l r0,@-r15
; CHECK: sts.l mach,{{.*}}Folded Spill
; CHECK: sts.l macl,{{.*}}Folded Spill
; CHECK: mov.l r8,{{.*}}Folded Spill
; CHECK: mov.l @r15,r15
; CHECK-NEXT: mov.l @r15+,r0
; CHECK-NEXT: rte

; CHECK-LABEL: nonleaf:
; CHECK: mov.l r0,@-r15
; CHECK: mov r0,r15
; CHECK: add #-{{[0-9]+}},r15
; CHECK: mov.l r5,@({{[0-9]+}},r15)
; CHECK: jsr
; CHECK: mov.l @r15,r15
; CHECK-NEXT: mov.l @r15+,r0
; CHECK-NEXT: rte

; CHECK-LABEL: frame:
; CHECK: mov.l r0,@-r15
; CHECK: mov r0,r15
; CHECK: add #-{{[0-9]+}},r15
; CHECK: jsr
; CHECK: add #{{[0-9]+}},r15
; CHECK: mov.l @r15,r15
; CHECK-NEXT: mov.l @r15+,r0
; CHECK-NEXT: rte

; CHECK-LABEL: large_frame:
; CHECK: mov.l r0,@-r15
; CHECK: mov r0,r15
; CHECK: add #-128,r15
; CHECK: jsr
; CHECK: mov.l @r15,r15
; CHECK-NEXT: mov.l @r15+,r0
; CHECK-NEXT: rte

; CHECK-LABEL: dynamic_frame:
; CHECK: mov.l r0,@-r15
; CHECK: mov r0,r15
; CHECK: mov r15,r14
; CHECK: jsr
; CHECK: mov r14,r15
; CHECK: mov.l @r15,r15
; CHECK-NEXT: mov.l @r15+,r0
; CHECK-NEXT: rte
