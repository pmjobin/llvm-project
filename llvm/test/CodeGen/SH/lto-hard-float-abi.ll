; RUN: split-file %s %t
; RUN: opt %t/hard.ll -o %t/hard-full.bc
; RUN: llvm-lto -O2 -j2 -exported-symbol=hard_entry -o %t/hard-full.o %t/hard-full.bc
; RUN: llvm-readobj --file-headers %t/hard-full.o.0 | FileCheck %s --check-prefix=FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t/hard-full.o.0 %t/hard-full.o.1 | FileCheck %s --check-prefix=HARD
; RUN: opt -module-summary %t/hard.ll -o %t/hard-thin.bc
; RUN: llvm-lto -O2 -j2 --thinlto-action=run -exported-symbol=hard_entry %t/hard-thin.bc
; RUN: llvm-readobj --file-headers %t/hard-thin.bc.thinlto.o | FileCheck %s --check-prefix=FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t/hard-thin.bc.thinlto.o | FileCheck %s --check-prefix=HARD
; RUN: opt %t/soft.ll -o %t/soft-full.bc
; RUN: llvm-lto -O2 -j2 -exported-symbol=soft_entry -o %t/soft-full.o %t/soft-full.bc
; RUN: llvm-objdump -dr --mcpu=sh4 %t/soft-full.o.0 %t/soft-full.o.1 | FileCheck %s --check-prefix=SOFT
; RUN: opt -module-summary %t/soft.ll -o %t/soft-thin.bc
; RUN: llvm-lto -O2 -j2 --thinlto-action=run -exported-symbol=soft_entry %t/soft-thin.bc
; RUN: llvm-objdump -dr --mcpu=sh4 %t/soft-thin.bc.thinlto.o | FileCheck %s --check-prefix=SOFT
; RUN: not llvm-link %t/hard-full.bc %t/soft-full.bc -o /dev/null 2>&1 | FileCheck %s --check-prefix=MIXED
; RUN: llc -O1 %t/float-flag-hard.ll -o - | FileCheck %s --check-prefix=FLOAT-FLAG-HARD
; RUN: not llc -O1 %t/mismatched-flags.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MISMATCHED-FLAGS

; FLAGS: Flags [ (0x9)

; HARD-LABEL: <hard_entry>:
; HARD: fmac
; HARD-NOT: sts fpul,r4
; HARD: jsr
; HARD-NOT: lds r0,fpul
; HARD: fmul
; HARD-NOT: sts fpul,r0
; HARD: R_SH_DIR32 external_hard

; SOFT-LABEL: <soft_entry>:
; SOFT: fadd
; SOFT: jsr
; SOFT-NEXT: sts fpul,r4
; SOFT: lds r{{[0-9]+}},fpul
; SOFT: fmul
; SOFT: sts fpul,r0
; SOFT: R_SH_DIR32 external_soft

; MIXED: linking module flags 'target-abi': IDs have conflicting values

; FLOAT-FLAG-HARD-LABEL: float_flag_hard:
; FLOAT-FLAG-HARD: fmov fr5,fr0

; MISMATCHED-FLAGS: LLVM ERROR: SH float ABI does not match its target ABI

;--- hard.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-linux-gnu"

declare float @external_hard(float) #0
declare float @llvm.fmuladd.f32(float, float, float)

define float @hard_entry(float %left, float %right) #0 {
	%argument = call float @llvm.fmuladd.f32(float %left, float %right, float %left)
	%called = call float @external_hard(float %argument)
	%result = fmul float %called, %argument
	ret float %result
}

attributes #0 = { noinline "target-abi"="gnu-hard-float" "target-cpu"="sh4" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}

;--- soft.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-linux-gnu"

declare float @external_soft(float) #0

define float @soft_entry(float %left, float %right) #0 {
	%argument = fadd float %left, %right
	%called = call float @external_soft(float %argument)
	%result = fmul float %called, %argument
	ret float %result
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh4" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- float-flag-hard.ll
target triple = "shle-unknown-linux-gnu"

define float @float_flag_hard(float %value) #0 {
	ret float %value
}

attributes #0 = { "target-cpu"="sh4" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"float-abi", !"hard"}

;--- mismatched-flags.ll
target triple = "shle-unknown-linux-gnu"

define float @mismatched_flags(float %value) #0 {
	ret float %value
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4" }

!llvm.module.flags = !{!0, !1}
!0 = !{i32 1, !"float-abi", !"hard"}
!1 = !{i32 1, !"target-abi", !"gnu"}
