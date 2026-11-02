; RUN: llc -mtriple=shle-unknown-none-elf -O2 -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=LE
; RUN: llc -mtriple=sh-unknown-none-elf -O2 -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=BE
; RUN: llc -mtriple=shle-unknown-none-elf -O2 -verify-machineinstrs -stop-after=sh-fpu-state -o - %s | FileCheck %s --check-prefix=STATE

define void @st40_300_fence() #0 {
	fence seq_cst
	ret void
}

define void @st40_300_with_st40_100_tune() #1 {
	fence seq_cst
	ret void
}

define void @generic_sh4_with_st40_300_tune() #2 {
	fence seq_cst
	ret void
}

define void @st40_300_singlethread_fence() #0 {
	fence syncscope("singlethread") seq_cst
	ret void
}

define float @st40_300_f32(float %left, float %right) #3 {
	%result = fadd float %left, %right
	ret float %result
}

define float @st40_300_single_only_f32(float %left, float %right) #4 {
	%result = fadd float %left, %right
	ret float %result
}

attributes #0 = { "target-cpu"="sh4" "target-features"="+st40-300" "target-abi"="gnu-hard-float" "tune-cpu"="st40-300" }
attributes #1 = { "target-cpu"="sh4" "target-features"="+st40-300" "target-abi"="gnu-hard-float" "tune-cpu"="st40-100" }
attributes #2 = { "target-cpu"="sh4" "target-abi"="gnu-hard-float" "tune-cpu"="st40-300" }
attributes #3 = { "target-cpu"="sh4" "target-features"="+st40-300" "target-abi"="gnu-hard-float" "tune-cpu"="st40-300" }
attributes #4 = { "target-cpu"="sh4" "target-features"="+st40-300,+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode" "target-abi"="gnu-e-float" "tune-cpu"="st40-300" }

; LE-LABEL: st40_300_fence:
; LE: synco
; LE-LABEL: st40_300_with_st40_100_tune:
; LE: synco
; LE-LABEL: generic_sh4_with_st40_300_tune:
; LE-NOT: synco
; LE: !MEMBARRIER
; LE-LABEL: st40_300_singlethread_fence:
; LE-NOT: synco
; LE: !MEMBARRIER
; LE-LABEL: st40_300_f32:
; LE: fpchg
; LE: fadd
; LE: fpchg
; LE-LABEL: st40_300_single_only_f32:
; LE-NOT: fpchg
; LE: fadd

; BE-LABEL: st40_300_fence:
; BE: synco
; BE-LABEL: st40_300_with_st40_100_tune:
; BE: synco
; BE-LABEL: generic_sh4_with_st40_300_tune:
; BE-NOT: synco
; BE: !MEMBARRIER
; BE-LABEL: st40_300_f32:
; BE: fpchg
; BE: fadd

; STATE-LABEL: name: st40_300_f32
; STATE: SET_FPSCR_MODE_TOGGLE 1, implicit-def $fpscr, implicit $fpscr
; STATE: FADD_FR {{.*}}, implicit $fpscr
; STATE: SET_FPSCR_MODE_TOGGLE 3, implicit-def $fpscr, implicit $fpscr
; STATE-LABEL: name: st40_300_single_only_f32
; STATE-NOT: SET_FPSCR_MODE_TOGGLE
; STATE: FADD_FR {{.*}}, implicit $fpscr
