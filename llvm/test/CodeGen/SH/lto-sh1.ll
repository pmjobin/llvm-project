; RUN: split-file %s %t
; RUN: opt %t/sh1.ll -o %t/sh1.bc
; RUN: llvm-lto -O2 -j1 -exported-symbol=sh1_entry -o %t/sh1-full.o %t/sh1.bc
; RUN: llvm-readobj --file-headers %t/sh1-full.o | FileCheck %s --check-prefix=SH1-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh1 %t/sh1-full.o | FileCheck %s --check-prefix=SH1-CODE
; RUN: opt -module-summary %t/sh1.ll -o %t/sh1-thin.bc
; RUN: llvm-lto -O2 -j2 --thinlto-action=run -exported-symbol=sh1_entry %t/sh1-thin.bc
; RUN: llvm-readobj --file-headers %t/sh1-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH1-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh1 %t/sh1-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH1-CODE
; RUN: opt %t/sh2.ll -o %t/sh2.bc
; RUN: llvm-link %t/sh1.bc %t/sh2.bc -o %t/sh1-sh2.bc
; RUN: llvm-lto -O2 -j1 -exported-symbol=sh1_entry -exported-symbol=sh2_entry -o %t/sh1-sh2.o %t/sh1-sh2.bc
; RUN: llvm-readobj --file-headers %t/sh1-sh2.o | FileCheck %s --check-prefix=SH2-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh2 %t/sh1-sh2.o | FileCheck %s --check-prefix=MIXED-CODE
; RUN: opt %t/e.ll -o %t/e.bc
; RUN: opt %t/hard.ll -o %t/hard.bc
; RUN: not llvm-link %t/sh1.bc %t/e.bc -o /dev/null 2>&1 | FileCheck %s --check-prefix=ABI-MIX
; RUN: not llvm-link %t/sh1.bc %t/hard.bc -o /dev/null 2>&1 | FileCheck %s --check-prefix=ABI-MIX

; SH1-FLAGS: Flags [ (0x1)
; SH2-FLAGS: Flags [ (0x2)

; SH1-CODE-LABEL: <sh1_entry>:
; SH1-CODE: R_SH_DIR32 __mulsi3
; SH1-CODE-NOT: mul.l
; SH1-CODE-NOT: dt
; SH1-CODE-NOT: braf

; MIXED-CODE-LABEL: <sh1_entry>:
; MIXED-CODE: R_SH_DIR32 __mulsi3
; MIXED-CODE-LABEL: <sh2_entry>:
; MIXED-CODE: mul.l

; ABI-MIX: linking module flags 'target-abi': IDs have conflicting values

;--- sh1.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i32 @sh1_entry(i32 %left, i32 %right) #0 {
	%result = mul i32 %left, %right
	ret i32 %result
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh1" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- sh2.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i32 @sh2_entry(i32 %left, i32 %right) #0 {
	%result = mul i32 %left, %right
	ret i32 %result
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh2" "target-features"="+sh2" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- e.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define float @e_entry(float %left, float %right) #0 {
	%result = fadd float %left, %right
	ret float %result
}

attributes #0 = { noinline "target-abi"="gnu-e-float" "target-cpu"="sh2e" "target-features"="+fpu-single-mode,+sh2,+single-fpu" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-e-float"}

;--- hard.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define double @hard_entry(double %left, double %right) #0 {
	%result = fadd double %left, %right
	ret double %result
}

attributes #0 = { noinline "target-abi"="gnu-hard-float" "target-cpu"="sh4" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}
