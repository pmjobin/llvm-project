; RUN: split-file %s %t
; RUN: llc -filetype=obj %t/accepted.ll -o %t.accepted.o
; RUN: llvm-readobj --file-headers %t.accepted.o | FileCheck %s --check-prefix=FLAGS
; RUN: llc -filetype=obj %t/accepted-reverse.ll -o %t.accepted-reverse.o
; RUN: llvm-readobj --file-headers %t.accepted-reverse.o | FileCheck %s --check-prefix=FLAGS
; RUN: sed 's/shle-/sh-/' %t/accepted.ll | llc -filetype=obj -o %t.accepted.be.o
; RUN: llvm-readobj --file-headers %t.accepted.be.o | FileCheck %s --check-prefix=FLAGS
; RUN: not llc -filetype=obj %t/sh2e.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CONFLICT
; RUN: not llc -filetype=obj %t/sh2a.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CONFLICT
; RUN: not llc -filetype=obj %t/sh2a-nofpu.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CONFLICT
; RUN: not llc -filetype=obj %t/sh3e.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CONFLICT
; RUN: not llc -filetype=obj %t/sh4-full.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CONFLICT
; RUN: not llc -filetype=obj %t/sh4a-full.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CONFLICT
; RUN: not llc -filetype=obj %t/st40-full.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CONFLICT

; FLAGS: Flags [ (0x6)
; CONFLICT: error: SH4AL-DSP cannot be merged with the requested SH architecture

;--- accepted.ll
target triple = "shle-unknown-none-elf"

define void @sh1() #0 { ret void }
define void @sh2() #1 { ret void }
define void @sh3() #2 { ret void }
define void @sh4_nofpu() #3 { ret void }
define void @sh4a_nofpu() #4 { ret void }
define void @st40_300_nofpu() #5 { ret void }
define void @st40_340() #6 { ret void }
define void @st40_400() #7 { ret void }
define void @st40_500() #8 { ret void }
define void @sh4al_dsp() #9 { ret void }

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh1" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu" "target-cpu"="sh2" "use-soft-float"="true" }
attributes #2 = { "target-abi"="gnu" "target-cpu"="sh3" "use-soft-float"="true" }
attributes #3 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-single-fpu,-double-fpu" "use-soft-float"="true" }
attributes #4 = { "target-abi"="gnu" "target-cpu"="sh4a" "target-features"="-single-fpu,-double-fpu" "use-soft-float"="true" }
attributes #5 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="+st40-300,-single-fpu,-double-fpu" "tune-cpu"="st40-300" "use-soft-float"="true" }
attributes #6 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-double-fpu" "tune-cpu"="st40-340" "use-soft-float"="true" }
attributes #7 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-double-fpu" "tune-cpu"="st40-400" "use-soft-float"="true" }
attributes #8 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-single-fpu,-double-fpu" "tune-cpu"="st40-500" "use-soft-float"="true" }
attributes #9 = { "target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="+dsp,+sh4al-dsp" "tune-cpu"="sh4al-dsp" "use-soft-float"="true" }

;--- accepted-reverse.ll
target triple = "shle-unknown-none-elf"

define void @sh4al_dsp() #0 { ret void }
define void @st40_500() #1 { ret void }
define void @st40_400() #2 { ret void }
define void @st40_340() #3 { ret void }
define void @st40_300_nofpu() #4 { ret void }
define void @sh4a_nofpu() #5 { ret void }
define void @sh4_nofpu() #6 { ret void }
define void @sh3() #7 { ret void }
define void @sh2() #8 { ret void }
define void @sh1() #9 { ret void }

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="+dsp,+sh4al-dsp" "tune-cpu"="sh4al-dsp" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-single-fpu,-double-fpu" "tune-cpu"="st40-500" "use-soft-float"="true" }
attributes #2 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-double-fpu" "tune-cpu"="st40-400" "use-soft-float"="true" }
attributes #3 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-double-fpu" "tune-cpu"="st40-340" "use-soft-float"="true" }
attributes #4 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="+st40-300,-single-fpu,-double-fpu" "tune-cpu"="st40-300" "use-soft-float"="true" }
attributes #5 = { "target-abi"="gnu" "target-cpu"="sh4a" "target-features"="-single-fpu,-double-fpu" "use-soft-float"="true" }
attributes #6 = { "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-single-fpu,-double-fpu" "use-soft-float"="true" }
attributes #7 = { "target-abi"="gnu" "target-cpu"="sh3" "use-soft-float"="true" }
attributes #8 = { "target-abi"="gnu" "target-cpu"="sh2" "use-soft-float"="true" }
attributes #9 = { "target-abi"="gnu" "target-cpu"="sh1" "use-soft-float"="true" }

;--- sh2e.ll
target triple = "shle-unknown-none-elf"
define void @sh4al() #0 { ret void }
define void @conflict() #1 { ret void }
attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4al-dsp" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu" "target-cpu"="sh2e" }

;--- sh2a.ll
target triple = "shle-unknown-none-elf"
define void @sh4al() #0 { ret void }
define void @conflict() #1 { ret void }
attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4al-dsp" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu-hard-float" "target-cpu"="sh2a" }

;--- sh2a-nofpu.ll
target triple = "shle-unknown-none-elf"
define void @sh4al() #0 { ret void }
define void @conflict() #1 { ret void }
attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4al-dsp" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu" "target-cpu"="sh2a" "target-features"="-single-fpu,-double-fpu" "use-soft-float"="true" }

;--- sh3e.ll
target triple = "shle-unknown-none-elf"
define void @sh4al() #0 { ret void }
define void @conflict() #1 { ret void }
attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4al-dsp" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu" "target-cpu"="sh3e" }

;--- sh4-full.ll
target triple = "shle-unknown-none-elf"
define void @sh4al() #0 { ret void }
define void @conflict() #1 { ret void }
attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4al-dsp" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu-hard-float" "target-cpu"="sh4" }

;--- sh4a-full.ll
target triple = "shle-unknown-none-elf"
define void @sh4al() #0 { ret void }
define void @conflict() #1 { ret void }
attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4al-dsp" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu-hard-float" "target-cpu"="sh4a" }

;--- st40-full.ll
target triple = "shle-unknown-none-elf"
define void @sh4al() #0 { ret void }
define void @conflict() #1 { ret void }
attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4al-dsp" "use-soft-float"="true" }
attributes #1 = { "target-abi"="gnu-hard-float" "target-cpu"="sh4" "target-features"="+st40-300" "tune-cpu"="st40-300" }
