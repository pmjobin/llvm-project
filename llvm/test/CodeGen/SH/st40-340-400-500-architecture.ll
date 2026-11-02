; RUN: llc -mtriple=shle-unknown-none-elf -O2 -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=sh-unknown-none-elf -O2 -verify-machineinstrs -o /dev/null %s

define i32 @st40_340_integer(ptr %pointer, i32 %left, i32 %right) #0 {
	%loaded = load i32, ptr %pointer, align 4
	%product = mul i32 %left, %right
	%result = add i32 %loaded, %product
	ret i32 %result
}

define void @st40_340_fence() #0 {
	fence seq_cst
	ret void
}

define float @st40_340_f32(float %left, float %right) #0 {
	%result = fadd float %left, %right
	ret float %result
}

define double @st40_340_f64(double %left, double %right) #0 {
	%result = fadd double %left, %right
	ret double %result
}

define i32 @st40_340_atomic(ptr %pointer) #0 {
	%result = atomicrmw add ptr %pointer, i32 1 seq_cst
	ret i32 %result
}

define i32 @st40_400_integer(ptr %pointer, i32 %left, i32 %right) #1 {
	%loaded = load i32, ptr %pointer, align 4
	%product = mul i32 %left, %right
	%result = add i32 %loaded, %product
	ret i32 %result
}

define void @st40_400_fence() #1 {
	fence seq_cst
	ret void
}

define float @st40_400_f32(float %left, float %right) #1 {
	%result = fadd float %left, %right
	ret float %result
}

define double @st40_400_f64(double %left, double %right) #1 {
	%result = fadd double %left, %right
	ret double %result
}

define i32 @st40_400_atomic(ptr %pointer) #1 {
	%result = atomicrmw add ptr %pointer, i32 1 seq_cst
	ret i32 %result
}

define i32 @st40_500_integer(ptr %pointer, i32 %left, i32 %right) #2 {
	%loaded = load i32, ptr %pointer, align 4
	%product = mul i32 %left, %right
	%result = add i32 %loaded, %product
	ret i32 %result
}

define void @st40_500_fence() #2 {
	fence seq_cst
	ret void
}

define float @st40_500_f32(float %left, float %right) #2 {
	%result = fadd float %left, %right
	ret float %result
}

define double @st40_500_f64(double %left, double %right) #2 {
	%result = fadd double %left, %right
	ret double %result
}

define i32 @st40_500_atomic(ptr %pointer) #2 {
	%result = atomicrmw add ptr %pointer, i32 1 seq_cst
	ret i32 %result
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-340" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-400" "use-soft-float"="true" }
attributes #2 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-500" "use-soft-float"="true" }

; ASM-LABEL: st40_340_integer:
; ASM: mul.l
; ASM-LABEL: st40_340_fence:
; ASM-NOT: synco
; ASM: !MEMBARRIER
; ASM-LABEL: st40_340_f32:
; ASM-NOT: fadd
; ASM: __addsf3
; ASM-LABEL: st40_340_f64:
; ASM-NOT: fadd
; ASM: __adddf3
; ASM-LABEL: st40_340_atomic:
; ASM: __atomic_fetch_add_4

; ASM-LABEL: st40_400_integer:
; ASM: mul.l
; ASM-LABEL: st40_400_fence:
; ASM-NOT: synco
; ASM: !MEMBARRIER
; ASM-LABEL: st40_400_f32:
; ASM-NOT: fadd
; ASM: __addsf3
; ASM-LABEL: st40_400_f64:
; ASM-NOT: fadd
; ASM: __adddf3
; ASM-LABEL: st40_400_atomic:
; ASM: __atomic_fetch_add_4

; ASM-LABEL: st40_500_integer:
; ASM: mul.l
; ASM-LABEL: st40_500_fence:
; ASM-NOT: synco
; ASM: !MEMBARRIER
; ASM-LABEL: st40_500_f32:
; ASM-NOT: fadd
; ASM: __addsf3
; ASM-LABEL: st40_500_f64:
; ASM-NOT: fadd
; ASM: __adddf3
; ASM-LABEL: st40_500_atomic:
; ASM: __atomic_fetch_add_4
; ASM-NOT: {{f(add|sub|mul|div|sqrt|mov|cmp|cnv)}}
; ASM-NOT: fpchg
; ASM-NOT: synco
