; RUN: split-file %s %t
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O2 -filetype=obj %t/inherit-hard.ll -o %t/inherit-hard.o
; RUN: llvm-readobj --file-headers %t/inherit-hard.o | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t/inherit-hard.o | FileCheck %s --check-prefix=HARD
; RUN: opt %t/inherit-hard.ll -o %t/inherit-hard.bc
; RUN: llvm-lto -O2 -mcpu=sh4 -exported-symbol=inherit_hard -o %t/inherit-hard-lto.o %t/inherit-hard.bc
; RUN: llvm-readobj --file-headers %t/inherit-hard-lto.o | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t/inherit-hard-lto.o | FileCheck %s --check-prefix=HARD
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O2 -filetype=obj %t/explicit-sh4-hard.ll -o %t/explicit-sh4-hard.o
; RUN: llvm-readobj --file-headers %t/explicit-sh4-hard.o | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: not llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 %t/explicit-sh3-hard.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INVALID
; RUN: not llc -mtriple=shle-unknown-linux-gnu -mcpu=sh3 %t/inherit-hard.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INVALID
; RUN: not llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 %t/inherit-hard.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INVALID
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh4 -O2 %t/explicit-sh3-soft.ll -o - | FileCheck %s --check-prefix=SH3-OVERRIDE

; SH4-FLAGS: Flags [ (0x9)
; HARD-LABEL: <inherit_hard>:
; HARD: fadd
; HARD: R_SH_DIR32 external_hard
; INVALID: LLVM ERROR: SH GNU hard-float ABI requires a double FPU in public double mode
; SH3-OVERRIDE-LABEL: explicit_sh3_soft:
; SH3-OVERRIDE: shld
; SH3-OVERRIDE: __addsf3
; SH3-OVERRIDE-NOT: fadd

;--- inherit-hard.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-linux-gnu"

declare float @external_hard(float)

define float @inherit_hard(float %left, float %right) {
	%sum = fadd float %left, %right
	%result = call float @external_hard(float %sum)
	ret float %result
}

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}

;--- explicit-sh4-hard.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-linux-gnu"

define float @explicit_sh4_hard(float %left, float %right) #0 {
	%result = fadd float %left, %right
	ret float %result
}

attributes #0 = { "target-cpu"="sh4" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}

;--- explicit-sh3-hard.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-linux-gnu"

define float @explicit_sh3_hard(float %value) #0 {
	ret float %value
}

attributes #0 = { "target-cpu"="sh3" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}

;--- explicit-sh3-soft.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-linux-gnu"

define float @explicit_sh3_soft(float %left, float %right, i32 %value, i32 %count) #0 {
	%sum = fadd float %left, %right
	%shift = shl i32 %value, %count
	%converted = sitofp i32 %shift to float
	%result = fadd float %sum, %converted
	ret float %result
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh3" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}
