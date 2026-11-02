; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh4 -O2 -verify-machineinstrs < %s | FileCheck %s

; SH cannot encode an indexed constant-pool address.  Keep selects of FPU
; constants in the ordinary select lowering instead of forming an indexed
; two-element constant-pool array.

; CHECK-LABEL: select_fp_constants:
; CHECK-DAG: fldi0
; CHECK-DAG: fldi1
; CHECK-NOT: .long 1065353216
; CHECK: rts

define float @select_fp_constants(i32 %value) #0 {
	%condition = icmp eq i32 %value, 0
	%result = select i1 %condition, float 0.000000e+00, float 1.000000e+00
	ret float %result
}

attributes #0 = { "target-cpu"="sh4" }
