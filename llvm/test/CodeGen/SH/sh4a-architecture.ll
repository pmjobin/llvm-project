; RUN: llc -mtriple=shle-unknown-none-elf -O2 -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-none-elf -O2 -verify-machineinstrs -filetype=obj -o %t.be.o %s
; RUN: llc -mtriple=shle-unknown-none-elf -O2 -verify-machineinstrs -filetype=obj -o %t.le.o %s
; RUN: llvm-readobj --file-headers --relocations %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj --file-headers --relocations %t.le.o | FileCheck %s --check-prefix=OBJECT
; RUN: llc -mtriple=shle-unknown-none-elf -O2 -verify-machineinstrs -stop-after=sh-fpu-state -o - %s | FileCheck %s --check-prefix=STATE

define void @sh4a_fence() #0 {
	fence seq_cst
	ret void
}

define void @sh3_tuned_for_sh4a_fence() #1 {
	fence seq_cst
	ret void
}

define void @sh4a_singlethread_fence() #0 {
	fence syncscope("singlethread") seq_cst
	ret void
}

define i32 @sh4a_atomic(ptr %pointer) #0 {
	%result = atomicrmw add ptr %pointer, i32 1 seq_cst
	ret i32 %result
}

define float @sh4a_f32(float %left, float %right) #2 {
	%result = fadd float %left, %right
	ret float %result
}

define float @sh4a_single_only_f32(float %left, float %right) #3 {
	%result = fadd float %left, %right
	ret float %result
}

define i64 @sh4a_i64(i64 %left, i64 %right) #0 {
	%shifted = shl i64 %left, 9
	%result = add i64 %shifted, %right
	ret i64 %result
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4a" "target-features"="-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="sh4a" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu" "target-cpu"="sh3" "target-features"="-single-fpu" "tune-cpu"="sh4a" "use-soft-float"="true" }
attributes #2 = { "target-abi"="gnu-hard-float" "target-cpu"="sh4a" "tune-cpu"="sh4a" }
attributes #3 = { "target-abi"="gnu-e-float" "target-cpu"="sh4a" "target-features"="+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode" "tune-cpu"="sh4a" }

; ASM-LABEL: sh4a_fence:
; ASM: synco
; ASM-LABEL: sh3_tuned_for_sh4a_fence:
; ASM-NOT: synco
; ASM: !MEMBARRIER
; ASM-LABEL: sh4a_singlethread_fence:
; ASM-NOT: synco
; ASM: !MEMBARRIER
; ASM-LABEL: sh4a_atomic:
; ASM: .long	__atomic_fetch_add_4
; ASM-LABEL: sh4a_f32:
; ASM: fpchg
; ASM: fadd
; ASM: fpchg
; ASM-LABEL: sh4a_single_only_f32:
; ASM-NOT: fpchg
; ASM: fadd
; ASM-LABEL: sh4a_i64:
; ASM: shll8

; OBJECT: Format: elf32-sh
; OBJECT: Flags [ (0xC)
; OBJECT: R_SH_DIR32 __atomic_fetch_add_4

; STATE-LABEL: name: sh4a_f32
; STATE: SET_FPSCR_MODE_TOGGLE 1, implicit-def $fpscr, implicit $fpscr
; STATE: FADD_FR {{.*}}, implicit $fpscr
; STATE: SET_FPSCR_MODE_TOGGLE 3, implicit-def $fpscr, implicit $fpscr
; STATE-LABEL: name: sh4a_single_only_f32
; STATE-NOT: SET_FPSCR_MODE_TOGGLE
; STATE: FADD_FR {{.*}}, implicit $fpscr
