; RUN: split-file %s %t
; RUN: llc -filetype=obj %t/nofpu.ll -o %t.nofpu.o
; RUN: llvm-readobj --file-headers %t.nofpu.o | FileCheck %s --check-prefix=NOFPU
; RUN: llc -filetype=obj %t/fpu-upgrade.ll -o %t.fpu.o
; RUN: llvm-readobj --file-headers %t.fpu.o | FileCheck %s --check-prefix=FULL
; RUN: llc -filetype=obj %t/sh4-upgrade.ll -o %t.sh4.o
; RUN: llvm-readobj --file-headers %t.sh4.o | FileCheck %s --check-prefix=FULL
; RUN: not llc -filetype=obj %t/sh2a-conflict.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CONFLICT

; NOFPU: Flags [ (0x11)
; FULL: Flags [ (0xC)
; CONFLICT: error: SH-2A cannot be merged with the requested SH architecture

;--- nofpu.ll
target triple = "shle-unknown-none-elf"

define void @sh4_nofpu() #0 {
	ret void
}

define void @sh4a_nofpu() #1 {
	ret void
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu" "target-cpu"="sh4a" "target-features"="-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "use-soft-float"="true" }

;--- fpu-upgrade.ll
target triple = "shle-unknown-none-elf"

define void @sh4a_nofpu() #0 {
	ret void
}

define float @sh2e_fpu(float %left, float %right) #1 {
	%result = fadd float %left, %right
	ret float %result
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4a" "target-features"="-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu" "target-cpu"="sh2e" }

;--- sh4-upgrade.ll
target triple = "shle-unknown-none-elf"

define void @sh4a_nofpu() #0 {
	ret void
}

define float @sh4_fpu(float %left, float %right) #1 {
	%result = fadd float %left, %right
	ret float %result
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4a" "target-features"="-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu" "target-cpu"="sh4" }

;--- sh2a-conflict.ll
target triple = "shle-unknown-none-elf"

define void @sh4a() #0 {
	ret void
}

define void @sh2a() #1 {
	ret void
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4a" "target-features"="-single-fpu,-double-fpu" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu" "target-cpu"="sh2a" "target-features"="-single-fpu,-double-fpu" "use-soft-float"="true" }
