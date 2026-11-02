; RUN: split-file %s %t
; RUN: not llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 %t/sh2.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CPU
; RUN: not llc -mtriple=shle-unknown-linux-gnu -mcpu=sh3 %t/sh3.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CPU
; RUN: not llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 %t/mismatch.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MISMATCH
; RUN: not llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 %t/canonical-in-single-mode.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CANONICAL-MODE
; RUN: not llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 %t/single-missing-mode.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SINGLE-MODE
; RUN: not llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 %t/e-full-fpu.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=E-PROFILE

; CPU: LLVM ERROR: SH GNU hard-float ABI requires a double FPU in public double mode
; MISMATCH: LLVM ERROR: SH function target ABI does not match its module
; CANONICAL-MODE: LLVM ERROR: SH GNU hard-float ABI requires a double FPU in public double mode
; SINGLE-MODE: LLVM ERROR: SH GNU single-mode hard-float ABI requires a double FPU in public single mode
; E-PROFILE: LLVM ERROR: SH GNU E-class ABI requires a single-only FPU in public single mode

;--- sh2.ll
target triple = "shle-unknown-linux-gnu"

define float @invalid_sh2(float %value) #0 {
	ret float %value
}

attributes #0 = { "target-abi"="gnu-hard-float" "target-cpu"="sh2" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}

;--- sh3.ll
target triple = "shle-unknown-linux-gnu"

define float @invalid_sh3(float %value) #0 {
	ret float %value
}

attributes #0 = { "target-abi"="gnu-hard-float" "target-cpu"="sh3" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}

;--- mismatch.ll
target triple = "shle-unknown-linux-gnu"

define float @invalid_mismatch(float %value) #0 {
	ret float %value
}

attributes #0 = { "target-abi"="gnu-hard-float" "target-cpu"="sh4" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- canonical-in-single-mode.ll
target triple = "shle-unknown-linux-gnu"

define float @invalid_canonical_mode(float %value) #0 {
	ret float %value
}

attributes #0 = { "target-abi"="gnu-hard-float" "target-cpu"="sh4" "target-features"="+fpu-single-mode" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}

;--- single-missing-mode.ll
target triple = "shle-unknown-linux-gnu"

define float @invalid_single_mode(float %value) #0 {
	ret float %value
}

attributes #0 = { "target-abi"="gnu-hard-float-single" "target-cpu"="sh4" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float-single"}

;--- e-full-fpu.ll
target triple = "shle-unknown-linux-gnu"

define float @invalid_e_profile(float %value) #0 {
	ret float %value
}

attributes #0 = { "target-abi"="gnu-e-float" "target-cpu"="sh4" "target-features"="+fpu-single-mode" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-e-float"}
