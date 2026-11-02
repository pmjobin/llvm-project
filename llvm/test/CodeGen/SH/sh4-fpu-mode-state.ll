; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -stop-after=sh-fpu-state -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4 -O0 -verify-machineinstrs < %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O0 -verify-machineinstrs < %s -o /dev/null

declare float @callee_f32(float)

; CHECK-LABEL: name: loop_f32
; CHECK: SET_FPSCR_MODE 0
; CHECK: SET_FPSCR_MODE 1
; CHECK: bb.1.loop:
; CHECK-NOT: SET_FPSCR_MODE
; CHECK: FADD_FR
; CHECK-NOT: SET_FPSCR_MODE
; CHECK: BF

define float @loop_f32(float %x, i32 %n) {
entry:
	br label %loop

loop:
	%i = phi i32 [ 0, %entry ], [ %next_i, %loop ]
	%value = phi float [ %x, %entry ], [ %next, %loop ]
	%next = fadd float %value, %x
	%next_i = add i32 %i, 1
	%again = icmp ult i32 %next_i, %n
	br i1 %again, label %loop, label %exit

exit:
	ret float %next
}

; CHECK-LABEL: name: loop_f64
; CHECK: SET_FPSCR_MODE 0
; CHECK: SET_FPSCR_MODE 3
; CHECK: bb.1.loop:
; CHECK-NOT: SET_FPSCR_MODE
; CHECK: FADD_DR
; CHECK-NOT: SET_FPSCR_MODE
; CHECK: BF

define double @loop_f64(double %x, i32 %n) {
entry:
	br label %loop

loop:
	%i = phi i32 [ 0, %entry ], [ %next_i, %loop ]
	%value = phi double [ %x, %entry ], [ %next, %loop ]
	%next = fadd double %value, %x
	%next_i = add i32 %i, 1
	%again = icmp ult i32 %next_i, %n
	br i1 %again, label %loop, label %exit

exit:
	ret double %next
}

; The call consumes the first hardware result, invalidates the assumed FPSCR
; state, and forces single mode to be re-established before resumed work.
;
; CHECK-LABEL: name: call_invalidation
; CHECK: SET_FPSCR_MODE 1
; CHECK: FADD_FR
; CHECK: FR_TO_GPR_PSEUDO
; CHECK: JSR
; CHECK: SET_FPSCR_MODE 1
; CHECK: GPR_TO_FR_PSEUDO
; CHECK: FMUL_FR

define float @call_invalidation(float %a, float %b) {
	%argument = fadd float %a, %b
	%returned = call float @callee_f32(float %argument)
	%result = fmul float %returned, %argument
	ret float %result
}

; CHECK-LABEL: name: mixed_precision
; CHECK: SET_FPSCR_MODE 1
; CHECK: FADD_FR
; CHECK: TRANSITION_FPSCR_MODE 3, 8
; CHECK: FADD_DR
; CHECK: TRANSITION_FPSCR_MODE 1, 8
; CHECK: FMUL_FR
; CHECK: TRANSITION_FPSCR_MODE 3, 8
; CHECK: F32_TO_F64_PSEUDO
; CHECK: FADD_DR

define double @mixed_precision(float %f, double %d, ptr %sink) {
	%fa = fadd float %f, %f
	store volatile float %fa, ptr %sink, align 4
	%da = fadd double %d, %d
	store volatile double %da, ptr %sink, align 4
	%fm = fmul float %fa, %f
	%extended = fpext float %fm to double
	%result = fadd double %da, %extended
	ret double %result
}

; CHECK-LABEL: name: diamond_join
; CHECK: bb.1.left:
; CHECK: SET_FPSCR_MODE 1
; CHECK: FADD_FR
; CHECK: bb.2.right:
; CHECK: SET_FPSCR_MODE 1
; CHECK: FMUL_FR
; CHECK: bb.3.join:
; CHECK: PHI

define float @diamond_join(float %a, float %b, i32 %condition) {
entry:
	%take_left = icmp ne i32 %condition, 0
	br i1 %take_left, label %left, label %right

left:
	%left_value = fadd float %a, %b
	br label %join

right:
	%right_value = fmul float %a, %b
	br label %join

join:
	%result = phi float [ %left_value, %left ], [ %right_value, %right ]
	ret float %result
}
