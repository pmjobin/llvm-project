; C09: a hardware handler can interrupt either precision, irrespective of its
; ordinary calling convention. A homogeneous loop with an entry preheader must
; normalize unknown hardware mode, preserving the non-mode FPSCR bits.
; Check the first state-establishment pass, before allocation and scheduling.
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O0 -stop-after=sh-fpu-state -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,SH4
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4a -O0 -stop-after=sh-fpu-state -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,SH4A
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O0 -stop-after=sh-fpu-state -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,SH4
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4a -O0 -stop-after=sh-fpu-state -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,SH4A

@float_result = global float 0.0, align 4
@double_result = global double 0.0, align 4

; CHECK-LABEL: name: interrupt_float
; CHECK: bb.0.entry:
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: SET_FPSCR_MODE 1,
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: bb.1.loop:
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: I32_TO_F32_PSEUDO

define void @interrupt_float() #0 {
entry:
	br label %loop

loop:
	%i = phi i32 [ 1, %entry ], [ %next, %loop ]
	%f = sitofp i32 %i to float
	store volatile float %f, ptr @float_result, align 4
	%next = add nuw nsw i32 %i, 1
	%more = icmp ult i32 %next, 4
	br i1 %more, label %loop, label %exit

exit:
	ret void
}

; CHECK-LABEL: name: trapa_float
; CHECK: bb.0.entry:
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: SET_FPSCR_MODE 1,
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: bb.1.loop:
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: I32_TO_F32_PSEUDO

define void @trapa_float() #1 {
entry:
	br label %loop

loop:
	%i = phi i32 [ 1, %entry ], [ %next, %loop ]
	%f = sitofp i32 %i to float
	store volatile float %f, ptr @float_result, align 4
	%next = add nuw nsw i32 %i, 1
	%more = icmp ult i32 %next, 4
	br i1 %more, label %loop, label %exit

exit:
	ret void
}

; CHECK-LABEL: name: interrupt_double
; CHECK: bb.0.entry:
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: SET_FPSCR_MODE 3,
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: bb.1.loop:
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: I32_TO_F64_PSEUDO

define void @interrupt_double() #2 {
entry:
	br label %loop

loop:
	%i = phi i32 [ 1, %entry ], [ %next, %loop ]
	%f = sitofp i32 %i to double
	store volatile double %f, ptr @double_result, align 4
	%next = add nuw nsw i32 %i, 1
	%more = icmp ult i32 %next, 4
	br i1 %more, label %loop, label %exit

exit:
	ret void
}

; CHECK-LABEL: name: trapa_double
; CHECK: bb.0.entry:
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: SET_FPSCR_MODE 3,
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: bb.1.loop:
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: I32_TO_F64_PSEUDO

define void @trapa_double() #3 {
entry:
	br label %loop

loop:
	%i = phi i32 [ 1, %entry ], [ %next, %loop ]
	%f = sitofp i32 %i to double
	store volatile double %f, ptr @double_result, align 4
	%next = add nuw nsw i32 %i, 1
	%more = icmp ult i32 %next, 4
	br i1 %more, label %loop, label %exit

exit:
	ret void
}

; CHECK-LABEL: name: ordinary_float
; CHECK: bb.0.entry:
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; SH4: TRANSITION_FPSCR_MODE 1, 8,
; SH4A: SET_FPSCR_MODE_TOGGLE 1
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: bb.1.loop:
; SH4-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: I32_TO_F32_PSEUDO

; The ordinary ABI still supplies an exact source for the relative transition.
define void @ordinary_float() #4 {
entry:
	br label %loop

loop:
	%i = phi i32 [ 1, %entry ], [ %next, %loop ]
	%f = sitofp i32 %i to float
	store volatile float %f, ptr @float_result, align 4
	%next = add nuw nsw i32 %i, 1
	%more = icmp ult i32 %next, 4
	br i1 %more, label %loop, label %exit

exit:
	ret void
}

; CHECK-LABEL: name: ordinary_double
; CHECK: bb.0.entry:
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; SH4: TRANSITION_FPSCR_MODE 3, 8,
; SH4A: SET_FPSCR_MODE_TOGGLE 3
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: bb.1.loop:
; SH4-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: I32_TO_F64_PSEUDO

; The ordinary ABI still supplies an exact source for the relative transition.
define void @ordinary_double() #5 {
entry:
	br label %loop

loop:
	%i = phi i32 [ 1, %entry ], [ %next, %loop ]
	%f = sitofp i32 %i to double
	store volatile double %f, ptr @double_result, align 4
	%next = add nuw nsw i32 %i, 1
	%more = icmp ult i32 %next, 4
	br i1 %more, label %loop, label %exit

exit:
	ret void
}

; An integer-only handler does not impose any working FPSCR mode.
; CHECK-LABEL: name: integer_only
; CHECK: body:
; CHECK-NOT: {{SET_FPSCR_MODE|TRANSITION_FPSCR_MODE|FPCHG}}
; CHECK: RTE_RETURN
define void @integer_only() #0 {
	ret void
}

attributes #0 = { noinline nounwind "target-abi"="gnu-hard-float" "sh-interrupt-handler"="interrupt" }
attributes #1 = { noinline nounwind "target-abi"="gnu-hard-float" "sh-interrupt-handler"="trapa" }
attributes #2 = { noinline nounwind "target-abi"="gnu-hard-float-single" "target-features"="+fpu-single-mode" "sh-interrupt-handler"="interrupt" }
attributes #3 = { noinline nounwind "target-abi"="gnu-hard-float-single" "target-features"="+fpu-single-mode" "sh-interrupt-handler"="trapa" }
attributes #4 = { noinline nounwind "target-abi"="gnu-hard-float" }
attributes #5 = { noinline nounwind "target-abi"="gnu-hard-float-single" "target-features"="+fpu-single-mode" }
