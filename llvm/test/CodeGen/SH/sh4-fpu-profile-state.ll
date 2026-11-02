; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -mattr=+fpu-single-mode -target-abi=gnu-hard-float-single -O2 -stop-after=sh-fpu-state -verify-machineinstrs < %s | FileCheck %s --check-prefix=SINGLE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode -target-abi=gnu-e-float -O2 -stop-after=sh-fpu-state -verify-machineinstrs < %s | FileCheck %s --check-prefix=SINGLE-ONLY
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -target-abi=gnu -O2 -stop-after=sh-fpu-state -verify-machineinstrs < %s | FileCheck %s --check-prefix=NOFPU

; SINGLE-LABEL: name: loop_f32
; SINGLE-NOT: SET_FPSCR_MODE
; SINGLE: FADD_FR
; SINGLE-NOT: SET_FPSCR_MODE

define float @loop_f32(float %value, i32 %count) {
entry:
	br label %loop

loop:
	%index = phi i32 [ 0, %entry ], [ %next_index, %loop ]
	%result = phi float [ %value, %entry ], [ %next, %loop ]
	%next = fadd float %result, %value
	%next_index = add i32 %index, 1
	%again = icmp ult i32 %next_index, %count
	br i1 %again, label %loop, label %exit

exit:
	ret float %next
}

; SINGLE-LABEL: name: loop_f64
; SINGLE: TRANSITION_FPSCR_MODE 3, 8
; SINGLE: bb.1.loop:
; SINGLE-NOT: SET_FPSCR_MODE
; SINGLE: FADD_DR
; SINGLE-NOT: SET_FPSCR_MODE
; SINGLE: TRANSITION_FPSCR_MODE 1, 8

define double @loop_f64(double %value, i32 %count) {
entry:
	br label %loop

loop:
	%index = phi i32 [ 0, %entry ], [ %next_index, %loop ]
	%result = phi double [ %value, %entry ], [ %next, %loop ]
	%next = fadd double %result, %value
	%next_index = add i32 %index, 1
	%again = icmp ult i32 %next_index, %count
	br i1 %again, label %loop, label %exit

exit:
	ret double %next
}

declare double @callee_f64(double)

; SINGLE-LABEL: name: call_in_double_mode
; SINGLE: TRANSITION_FPSCR_MODE 3, 8
; SINGLE: FADD_DR
; SINGLE: TRANSITION_FPSCR_MODE 1, 8
; SINGLE: JSR
; SINGLE: TRANSITION_FPSCR_MODE 3, 8
; SINGLE: FADD_DR
; SINGLE: TRANSITION_FPSCR_MODE 1, 8

define double @call_in_double_mode(double %left, double %right) {
	%argument = fadd double %left, %right
	%returned = call double @callee_f64(double %argument)
	%result = fadd double %returned, %argument
	ret double %result
}

; SINGLE-ONLY-LABEL: name: f32_helper_f32
; SINGLE-ONLY-NOT: SET_FPSCR_MODE
; SINGLE-ONLY: FADD_FR
; SINGLE-ONLY-NOT: SET_FPSCR_MODE
; SINGLE-ONLY: JSR
; SINGLE-ONLY-NOT: SET_FPSCR_MODE
; SINGLE-ONLY: FMUL_FR
; SINGLE-ONLY-NOT: SET_FPSCR_MODE
; NOFPU-LABEL: name: f32_helper_f32
; NOFPU-NOT: SET_FPSCR_MODE
; NOFPU: JSR
; NOFPU-NOT: SET_FPSCR_MODE

define float @f32_helper_f32(float %single, double %wide) {
	%first = fadd float %single, %single
	%extended = fpext float %first to double
	%wide_result = fadd double %extended, %wide
	%rounded = fptrunc double %wide_result to float
	%result = fmul float %first, %rounded
	ret float %result
}
