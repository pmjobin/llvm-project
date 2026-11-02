; RUN: opt %s -o %t.full.bc
; RUN: llvm-lto -O2 -j1 -exported-symbol=restricted -exported-symbol=canonical -o %t.full.o %t.full.bc
; RUN: llvm-readobj --file-headers %t.full.o | FileCheck %s --check-prefix=FLAGS
; RUN: llvm-objdump -d --mcpu=sh4a %t.full.o | FileCheck %s --check-prefix=CODE
; RUN: opt -module-summary %s -o %t.thin.bc
; RUN: llvm-lto -O2 -j2 --thinlto-action=run -exported-symbol=restricted -exported-symbol=canonical %t.thin.bc
; RUN: llvm-readobj --file-headers %t.thin.bc.thinlto.o | FileCheck %s --check-prefix=FLAGS
; RUN: llvm-objdump -d --mcpu=sh4a %t.thin.bc.thinlto.o | FileCheck %s --check-prefix=CODE

; Both explicit profiles have the same ELF identity and public single-mode ABI.
; Retain restricted attributes from older bitcode when mixed with the canonical
; profile. The restricted function still cannot use the disabled instructions.
; FLAGS: Flags [ (0xC)
; CODE-DAG: <restricted>:
; CODE-DAG: <canonical>:
; CODE-DAG: fadd
; CODE-DAG: fadd

target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define float @restricted(float %a, float %b) #0 {
  %sum = fadd float %a, %b
  ret float %sum
}

define float @canonical(float %a, float %b) #1 {
  %sum = fadd float %a, %b
  ret float %sum
}

attributes #0 = { noinline "target-abi"="gnu-e-float" "target-cpu"="sh4a" "target-features"="+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode" }
attributes #1 = { noinline "target-abi"="gnu-e-float" "target-cpu"="sh4a" "target-features"="+single-fpu,+fsqrt,+fpu-size-mode,-double-fpu,+fpu-register-bank,+fpu-vector,+fpu-single-mode" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-e-float"}
