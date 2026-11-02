; RUN: split-file %s %t
; RUN: llc -filetype=obj %t/full.ll -o %t.full.o
; RUN: llvm-readobj --file-headers %t.full.o | FileCheck %s --check-prefix=FULL
; RUN: llc -filetype=obj %t/nofpu.ll -o %t.nofpu.o
; RUN: llvm-readobj --file-headers %t.nofpu.o | FileCheck %s --check-prefix=NOFPU
; RUN: llc -filetype=obj %t/tune-only.ll -o %t.tune-only.o
; RUN: llvm-readobj --file-headers %t.tune-only.o | FileCheck %s --check-prefix=TUNE-ONLY

; FULL: Flags [ (0xC)
; NOFPU: Flags [ (0x11)
; TUNE-ONLY: Flags [ (0x9)

;--- full.ll
target triple = "shle-unknown-none-elf"

define void @generic_sh4() #0 {
	ret void
}

define void @st40_300() #1 {
	fence seq_cst
	ret void
}

attributes #0 = { "target-abi"="gnu-hard-float" "target-cpu"="sh4" }
attributes #1 = { "target-abi"="gnu-hard-float" "target-cpu"="sh4" "target-features"="+st40-300" "tune-cpu"="st40-300" }

;--- nofpu.ll
target triple = "shle-unknown-none-elf"

define void @generic_sh4_nofpu() #0 {
	ret void
}

define void @st40_300_nofpu() #1 {
	fence seq_cst
	ret void
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="+st40-300,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-300" "use-soft-float"="true" }

;--- tune-only.ll
target triple = "shle-unknown-none-elf"

define void @generic_sh4_st40_300_tune() #0 {
	fence seq_cst
	ret void
}

attributes #0 = { "target-abi"="gnu-hard-float" "target-cpu"="sh4" "tune-cpu"="st40-300" }
