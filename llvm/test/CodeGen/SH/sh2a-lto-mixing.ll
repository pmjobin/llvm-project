; RUN: split-file %s %t
; RUN: opt %t/sh2.ll -o %t/sh2.bc
; RUN: opt %t/sh2a.ll -o %t/sh2a.bc
; RUN: llvm-link %t/sh2.bc %t/sh2a.bc -o %t/compatible.bc
; RUN: llvm-lto -O2 -j1 -exported-symbol=sh2_entry -exported-symbol=sh2a_entry -o %t/compatible.o %t/compatible.bc
; RUN: llvm-readobj --file-headers %t/compatible.o | FileCheck %s --check-prefix=SH2A-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh2a %t/compatible.o | FileCheck %s --check-prefix=CODE
; RUN: opt -module-summary %t/sh2.ll -o %t/sh2-thin.bc
; RUN: opt -module-summary %t/sh2a.ll -o %t/sh2a-thin.bc
; RUN: llvm-lto -O2 -j1 --thinlto-action=run -exported-symbol=sh2_entry -exported-symbol=sh2a_entry %t/sh2-thin.bc %t/sh2a-thin.bc
; RUN: llvm-readobj --file-headers %t/sh2-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH2-FLAGS
; RUN: llvm-readobj --file-headers %t/sh2a-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH2A-FLAGS
; RUN: opt %t/sh3.ll -o %t/sh3.bc
; RUN: llvm-link %t/sh2a.bc %t/sh3.bc -o %t/incompatible-architecture.bc
; RUN: not llvm-lto -O2 -j1 -exported-symbol=sh2a_entry -exported-symbol=sh3_entry -o %t/incompatible.o %t/incompatible-architecture.bc 2>&1 | FileCheck %s --check-prefix=ARCH-CONFLICT
; RUN: opt %t/single.ll -o %t/single.bc
; RUN: not llvm-link %t/sh2a.bc %t/single.bc -o /dev/null 2>&1 | FileCheck %s --check-prefix=ABI-CONFLICT

; SH2-FLAGS: Flags [ (0x2)
; SH2A-FLAGS: Flags [ (0x13)

; CODE-LABEL: <sh2_entry>:
; CODE-NOT: movi20
; CODE-LABEL: <sh2a_entry>:
; CODE: movi20 #74565,r0

; ARCH-CONFLICT: SH-2A cannot be merged with the requested SH architecture
; ABI-CONFLICT: linking module flags 'target-abi': IDs have conflicting values

;--- sh2.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i32 @sh2_entry(i32 %value) #0 {
	%result = add i32 %value, 1
	ret i32 %result
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh2" "target-features"="+sh2,-single-fpu,-double-fpu" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- sh2a.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i32 @sh2a_entry() #0 {
	ret i32 74565
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh2a" "target-features"="+dynamic-shift,+prefetch,+sh2,+sh2a,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- sh3.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i32 @sh3_entry(i32 %value) #0 {
	ret i32 %value
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh3" "target-features"="+sh2,+dynamic-shift,-single-fpu" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- single.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define float @single_entry(float %value) #0 {
	ret float %value
}

attributes #0 = { noinline "target-abi"="gnu-hard-float-single" "target-cpu"="sh2a" "target-features"="+sh2,+sh2a,+single-fpu,+double-fpu,+fpu-size-mode,+fpu-single-mode" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float-single"}
