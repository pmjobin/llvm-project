; RUN: split-file %s %t
; RUN: llc -filetype=obj %t/st40-340.ll -o %t.340.o
; RUN: llvm-readobj --file-headers %t.340.o | FileCheck %s --check-prefix=NOMMU-NOFPU
; RUN: llc -filetype=obj %t/st40-400.ll -o %t.400.o
; RUN: llvm-readobj --file-headers %t.400.o | FileCheck %s --check-prefix=NOMMU-NOFPU
; RUN: llc -filetype=obj %t/st40-500.ll -o %t.500.o
; RUN: llvm-readobj --file-headers %t.500.o | FileCheck %s --check-prefix=NOFPU
; RUN: llc -filetype=obj %t/all.ll -o %t.all.o
; RUN: llvm-readobj --file-headers %t.all.o | FileCheck %s --check-prefix=NOFPU
; RUN: llc -filetype=obj %t/with-st40-300-nofpu.ll -o %t.with-st40-300-nofpu.o
; RUN: llvm-readobj --file-headers %t.with-st40-300-nofpu.o | FileCheck %s --check-prefix=ST40-300-NOFPU
; RUN: llc -filetype=obj %t/with-st40-300-full.ll -o %t.with-st40-300-full.o
; RUN: llvm-readobj --file-headers %t.with-st40-300-full.o | FileCheck %s --check-prefix=ST40-300-FULL
; RUN: sed 's/shle-/sh-/' %t/st40-340.ll | llc -filetype=obj -o %t.340.be.o
; RUN: llvm-readobj --file-headers %t.340.be.o | FileCheck %s --check-prefix=NOMMU-NOFPU

; NOMMU-NOFPU: Flags [ (0x12)
; NOFPU: Flags [ (0x10)
; ST40-300-NOFPU: Flags [ (0x11)
; ST40-300-FULL: Flags [ (0xC)

;--- st40-340.ll
target triple = "shle-unknown-none-elf"

define void @st40_340() #0 {
	ret void
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-340" "use-soft-float"="true" }

;--- st40-400.ll
target triple = "shle-unknown-none-elf"

define void @st40_400() #0 {
	ret void
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-400" "use-soft-float"="true" }

;--- st40-500.ll
target triple = "shle-unknown-none-elf"

define void @st40_500() #0 {
	ret void
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-500" "use-soft-float"="true" }

;--- all.ll
target triple = "shle-unknown-none-elf"

define void @st40_340() #0 {
	ret void
}

define void @st40_400() #1 {
	ret void
}

define void @st40_500() #2 {
	ret void
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-340" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-400" "use-soft-float"="true" }
attributes #2 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-500" "use-soft-float"="true" }

;--- with-st40-300-nofpu.ll
target triple = "shle-unknown-none-elf"

define void @st40_340() #0 {
	ret void
}

define void @st40_300_nofpu() #1 {
	ret void
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-340" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="+st40-300,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-300" "use-soft-float"="true" }

;--- with-st40-300-full.ll
target triple = "shle-unknown-none-elf"

define void @st40_340() #0 {
	ret void
}

define void @st40_300_full() #1 {
	ret void
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-340" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu-hard-float" "target-cpu"="sh4" "target-features"="+st40-300,+single-fpu,+fsqrt,+fpu-size-mode,+double-fpu,+fpu-register-bank,+fpu-vector,-fpu-single-mode" "tune-cpu"="st40-300" }
