; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs -asm-verbose=false < %s | FileCheck %s

declare float @callee(float)

define float @tail_after_fpu(float %value) #0 {
; CHECK-LABEL: tail_after_fpu:
; CHECK-NOT: sts.l	pr
; CHECK: sts	fpscr,r0
; CHECK: fadd
; CHECK: lds	r0,fpscr
; CHECK: jmp	@
; CHECK-NEXT: nop
; CHECK-NOT: rts
	%sum = fadd float %value, %value
	%result = tail call float @callee(float %sum)
	ret float %result
}

attributes #0 = { noinline "target-cpu"="sh4" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}
