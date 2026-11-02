; RUN: llc -mtriple=shle-unknown-none-elf -O2 -verify-machineinstrs -stop-after=machine-scheduler -o - %s | FileCheck %s
; RUN: llc -mtriple=sh-unknown-none-elf -O2 -verify-machineinstrs -stop-after=machine-scheduler -o /dev/null %s

define i32 @st40_100(ptr %pointer, i32 %left, i32 %right) #0 {
	%loaded = load i32, ptr %pointer, align 4
	%product = mul i32 %left, %right
	%result = add i32 %loaded, %product
	ret i32 %result
}

define i32 @st40_200(ptr %pointer, i32 %left, i32 %right) #1 {
	%loaded = load i32, ptr %pointer, align 4
	%product = mul i32 %left, %right
	%result = add i32 %loaded, %product
	ret i32 %result
}

define i32 @st40_300(ptr %pointer, i32 %left, i32 %right) #2 {
	%loaded = load i32, ptr %pointer, align 4
	%product = mul i32 %left, %right
	%result = add i32 %loaded, %product
	ret i32 %result
}

define i32 @st40_340(ptr %pointer, i32 %left, i32 %right) #3 {
	%loaded = load i32, ptr %pointer, align 4
	%product = mul i32 %left, %right
	%result = add i32 %loaded, %product
	ret i32 %result
}

define i32 @st40_400(ptr %pointer, i32 %left, i32 %right) #4 {
	%loaded = load i32, ptr %pointer, align 4
	%product = mul i32 %left, %right
	%result = add i32 %loaded, %product
	ret i32 %result
}

define i32 @st40_500(ptr %pointer, i32 %left, i32 %right) #5 {
	%loaded = load i32, ptr %pointer, align 4
	%product = mul i32 %left, %right
	%result = add i32 %loaded, %product
	ret i32 %result
}

attributes #0 = { "target-cpu"="sh4" "tune-cpu"="st40-100" }
attributes #1 = { "target-cpu"="sh4" "tune-cpu"="st40-200" }
attributes #2 = { "target-cpu"="sh4" "target-features"="+st40-300" "tune-cpu"="st40-300" }
attributes #3 = { "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-340" }
attributes #4 = { "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-400" }
attributes #5 = { "target-cpu"="sh4" "target-features"="-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-500" }

; CHECK: define i32 @st40_100({{.*}}) #[[ST100:[0-9]+]]
; CHECK: define i32 @st40_200({{.*}}) #[[ST200:[0-9]+]]
; CHECK: define i32 @st40_300({{.*}}) #[[ST300:[0-9]+]]
; CHECK: define i32 @st40_340({{.*}}) #[[ST340:[0-9]+]]
; CHECK: define i32 @st40_400({{.*}}) #[[ST400:[0-9]+]]
; CHECK: define i32 @st40_500({{.*}}) #[[ST500:[0-9]+]]
; CHECK: attributes #[[ST100]] = { "target-cpu"="sh4" "tune-cpu"="st40-100" }
; CHECK: attributes #[[ST200]] = { "target-cpu"="sh4" "tune-cpu"="st40-200" }
; CHECK: attributes #[[ST300]] = { "target-cpu"="sh4" "target-features"="+st40-300" "tune-cpu"="st40-300" }
; CHECK: attributes #[[ST340]] = { "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-340" }
; CHECK: attributes #[[ST400]] = { "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-400" }
; CHECK: attributes #[[ST500]] = { "target-cpu"="sh4" "target-features"="-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-500" }

; CHECK-LABEL: name: st40_100
; CHECK: MUL_L
; CHECK: %{{[0-9]+}}:gpr = MOVL_load_reg
; CHECK-NEXT: %{{[0-9]+}}:gpr = STS_MACL
; CHECK-NEXT: %{{[0-9]+}}:gpr = ADDrr

; CHECK-LABEL: name: st40_200
; CHECK: MUL_L
; CHECK: %{{[0-9]+}}:gpr = MOVL_load_reg
; CHECK-NEXT: %{{[0-9]+}}:gpr = STS_MACL
; CHECK-NEXT: %{{[0-9]+}}:gpr = ADDrr

; CHECK-LABEL: name: st40_300
; CHECK: MUL_L
; CHECK: %{{[0-9]+}}:gpr = MOVL_load_reg
; CHECK-NEXT: %{{[0-9]+}}:gpr = STS_MACL
; CHECK-NEXT: %{{[0-9]+}}:gpr = ADDrr

; ST40-340 shares the seven-stage ST40-300 execution family.
; CHECK-LABEL: name: st40_340
; CHECK: MUL_L
; CHECK: %{{[0-9]+}}:gpr = MOVL_load_reg
; CHECK-NEXT: %{{[0-9]+}}:gpr = STS_MACL
; CHECK-NEXT: %{{[0-9]+}}:gpr = ADDrr

; ST40-400 and ST40-500 share the documented five-stage execution family.
; CHECK-LABEL: name: st40_400
; CHECK: MUL_L
; CHECK: %{{[0-9]+}}:gpr = MOVL_load_reg
; CHECK-NEXT: %{{[0-9]+}}:gpr = STS_MACL
; CHECK-NEXT: %{{[0-9]+}}:gpr = ADDrr

; CHECK-LABEL: name: st40_500
; CHECK: MUL_L
; CHECK: %{{[0-9]+}}:gpr = MOVL_load_reg
; CHECK-NEXT: %{{[0-9]+}}:gpr = STS_MACL
; CHECK-NEXT: %{{[0-9]+}}:gpr = ADDrr
