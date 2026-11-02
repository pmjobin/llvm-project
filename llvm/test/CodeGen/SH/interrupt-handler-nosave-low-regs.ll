; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh3 -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-double-fpu -O2 -verify-machineinstrs %s -o - | FileCheck %s

declare void @helper(i32, i32, i32, i32, i32, i32, i32, i32)

define void @leaf() #0 {
	call void asm sideeffect "", "~{r0},~{r7},~{r8},~{mach},~{macl}"()
	ret void
}

define void @nonleaf() #0 {
	call void @helper(i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8)
	ret void
}

attributes #0 = { noinline nounwind "disable-tail-calls"="true" "sh-interrupt-handler"="interrupt" "sh-nosave-low-regs" "target-abi"="gnu" }

; CHECK-LABEL: leaf:
; CHECK: sts.l mach,@-r15
; CHECK: sts.l macl,@-r15
; CHECK-NOT: mov.l r{{[0-7]}},{{.*}}Folded Spill
; CHECK: mov.l r8,
; CHECK: rte
; CHECK-NEXT: nop

; CHECK-LABEL: nonleaf:
; CHECK: sts.l pr,@-r15
; CHECK: sts.l mach,@-r15
; CHECK: sts.l macl,@-r15
; CHECK-NOT: mov.l r{{[0-7]}},{{.*}}Folded Spill
; CHECK: jsr
; CHECK: lds.l @r15+,pr
; CHECK: rte
; CHECK-NEXT: nop
