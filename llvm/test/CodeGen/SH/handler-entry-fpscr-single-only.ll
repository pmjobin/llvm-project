; C09: single-only hardware handlers retain their stronger entry contract even
; without an ordinary floating ABI boundary. No mode normalization is needed.
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2e -target-abi=gnu -O0 -stop-after=sh-fpu-state -verify-machineinstrs %s -o - | FileCheck %s --implicit-check-not=SET_FPSCR_MODE --implicit-check-not=TRANSITION_FPSCR_MODE --implicit-check-not=FPCHG
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2e -target-abi=gnu-e-float -O0 -stop-after=sh-fpu-state -verify-machineinstrs %s -o - | FileCheck %s --implicit-check-not=SET_FPSCR_MODE --implicit-check-not=TRANSITION_FPSCR_MODE --implicit-check-not=FPCHG
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3e -target-abi=gnu -O0 -stop-after=sh-fpu-state -verify-machineinstrs %s -o - | FileCheck %s --implicit-check-not=SET_FPSCR_MODE --implicit-check-not=TRANSITION_FPSCR_MODE --implicit-check-not=FPCHG
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3e -target-abi=gnu-e-float -O0 -stop-after=sh-fpu-state -verify-machineinstrs %s -o - | FileCheck %s --implicit-check-not=SET_FPSCR_MODE --implicit-check-not=TRANSITION_FPSCR_MODE --implicit-check-not=FPCHG
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode -O0 -stop-after=sh-fpu-state -verify-machineinstrs %s -o - | FileCheck %s --implicit-check-not=SET_FPSCR_MODE --implicit-check-not=TRANSITION_FPSCR_MODE --implicit-check-not=FPCHG
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -target-abi=gnu-e-float -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode -O0 -stop-after=sh-fpu-state -verify-machineinstrs %s -o - | FileCheck %s --implicit-check-not=SET_FPSCR_MODE --implicit-check-not=TRANSITION_FPSCR_MODE --implicit-check-not=FPCHG

@result = global float 0.0, align 4

; CHECK-LABEL: name: interrupt_single_only
; CHECK: bb.0.entry:
; CHECK: bb.1.loop:
; CHECK: I32_TO_F32_PSEUDO
; CHECK: RTE_RETURN
define void @interrupt_single_only() #0 {
entry:
	br label %loop

loop:
	%i = phi i32 [ 1, %entry ], [ %next, %loop ]
	%f = sitofp i32 %i to float
	store volatile float %f, ptr @result, align 4
	%next = add nuw nsw i32 %i, 1
	%more = icmp ult i32 %next, 4
	br i1 %more, label %loop, label %exit

exit:
	ret void
}

; CHECK-LABEL: name: trapa_single_only
; CHECK: bb.0.entry:
; CHECK: bb.1.loop:
; CHECK: I32_TO_F32_PSEUDO
; CHECK: RTE_RETURN
define void @trapa_single_only() #1 {
entry:
	br label %loop

loop:
	%i = phi i32 [ 1, %entry ], [ %next, %loop ]
	%f = sitofp i32 %i to float
	store volatile float %f, ptr @result, align 4
	%next = add nuw nsw i32 %i, 1
	%more = icmp ult i32 %next, 4
	br i1 %more, label %loop, label %exit

exit:
	ret void
}

attributes #0 = { noinline nounwind "sh-interrupt-handler"="interrupt" }
attributes #1 = { noinline nounwind "sh-interrupt-handler"="trapa" }
