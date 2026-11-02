; RUN: split-file %s %t
; RUN: opt %t/sh2e.ll -o %t/sh2e.bc
; RUN: llvm-lto -O2 -j1 -exported-symbol=sh2e_entry -o %t/sh2e-full.o %t/sh2e.bc
; RUN: llvm-readobj --file-headers %t/sh2e-full.o | FileCheck %s --check-prefix=SH2E-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh2e %t/sh2e-full.o | FileCheck %s --check-prefix=SH2E
; RUN: opt -module-summary %t/sh2e.ll -o %t/sh2e-thin.bc
; RUN: llvm-lto -O2 -j2 --thinlto-action=run -exported-symbol=sh2e_entry %t/sh2e-thin.bc
; RUN: llvm-readobj --file-headers %t/sh2e-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH2E-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh2e %t/sh2e-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH2E
; RUN: opt %t/sh3e.ll -o %t/sh3e.bc
; RUN: llvm-lto -O2 -j1 -exported-symbol=sh3e_entry -o %t/sh3e-full.o %t/sh3e.bc
; RUN: llvm-readobj --file-headers %t/sh3e-full.o | FileCheck %s --check-prefix=SH3E-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh3e %t/sh3e-full.o | FileCheck %s --check-prefix=SH3E
; RUN: opt -module-summary %t/sh3e.ll -o %t/sh3e-thin.bc
; RUN: llvm-lto -O2 -j2 --thinlto-action=run -exported-symbol=sh3e_entry %t/sh3e-thin.bc
; RUN: llvm-readobj --file-headers %t/sh3e-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH3E-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh3e %t/sh3e-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH3E
; RUN: llvm-link %t/sh2e.bc %t/sh3e.bc -o %t/sh2e-sh3e.bc
; RUN: llvm-lto -O2 -j1 -exported-symbol=sh2e_entry -exported-symbol=sh3e_entry -o %t/sh2e-sh3e.o %t/sh2e-sh3e.bc
; RUN: llvm-readobj --file-headers %t/sh2e-sh3e.o | FileCheck %s --check-prefix=SH3E-FLAGS
; RUN: opt %t/sh4e.ll -o %t/sh4e.bc
; RUN: llvm-link %t/sh2e.bc %t/sh4e.bc -o %t/sh2e-sh4e.bc
; RUN: llvm-lto -O2 -j1 -exported-symbol=sh2e_entry -exported-symbol=sh4e_entry -o %t/sh2e-sh4e.o %t/sh2e-sh4e.bc
; RUN: llvm-readobj --file-headers %t/sh2e-sh4e.o | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: opt %t/soft.ll -o %t/soft.bc
; RUN: opt %t/hard.ll -o %t/hard.bc
; RUN: not llvm-link %t/sh2e.bc %t/soft.bc -o /dev/null 2>&1 | FileCheck %s --check-prefix=MIXED
; RUN: not llvm-link %t/sh3e.bc %t/hard.bc -o /dev/null 2>&1 | FileCheck %s --check-prefix=MIXED

; SH2E-FLAGS: Flags [ (0xB)
; SH3E-FLAGS: Flags [ (0x8)
; SH4-FLAGS: Flags [ (0x9)

; SH2E-LABEL: <sh2e_entry>:
; SH2E: fadd
; SH2E: R_SH_DIR32 __adddf3
; SH2E-NOT: fsqrt
; SH2E-NOT: fadd dr

; SH3E-LABEL: <sh3e_entry>:
; SH3E: fadd
; SH3E: fsqrt
; SH3E-NOT: fadd dr

; MIXED: linking module flags 'target-abi': IDs have conflicting values

;--- sh2e.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define double @sh2e_entry(float %single, double %wide) #0 {
	%single.sum = fadd float %single, %single
	%extended = fpext float %single.sum to double
	%result = fadd double %extended, %wide
	ret double %result
}

attributes #0 = { noinline "target-abi"="gnu-e-float" "target-cpu"="sh2e" "target-features"="+fpu-single-mode,+sh2,+single-fpu" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-e-float"}

;--- sh3e.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

declare float @llvm.sqrt.f32(float)

define float @sh3e_entry(float %left, float %right) #0 {
	%sum = fadd float %left, %right
	%result = call float @llvm.sqrt.f32(float %sum)
	ret float %result
}

attributes #0 = { noinline "target-abi"="gnu-e-float" "target-cpu"="sh3e" "target-features"="+banked-gprs,+dynamic-shift,+exception-state,+fpu-single-mode,+fsqrt,+mmu,+prefetch,+s-bit-control,+sh2,+single-fpu" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-e-float"}

;--- sh4e.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define float @sh4e_entry(float %left, float %right) #0 {
	%result = fmul float %left, %right
	ret float %result
}

attributes #0 = { noinline "target-abi"="gnu-e-float" "target-cpu"="sh4" "target-features"="+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-e-float"}

;--- soft.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define float @soft_entry(float %left, float %right) #0 {
	%result = fadd float %left, %right
	ret float %result
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh2" "target-features"="+sh2" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- hard.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define float @hard_entry(float %left, float %right) #0 {
	%result = fadd float %left, %right
	ret float %result
}

attributes #0 = { noinline "target-abi"="gnu-hard-float" "target-cpu"="sh4" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}
