; RUN: split-file %s %t
; RUN: opt %t/sh4.ll -o %t/sh4.bc
; RUN: opt %t/sh4a.ll -o %t/sh4a.bc
; RUN: opt %t/st40.ll -o %t/st40.bc
; RUN: opt %t/sh4al.ll -o %t/sh4al.bc
; RUN: opt %t/hard.ll -o %t/hard.bc
; RUN: llvm-link %t/sh4.bc %t/sh4al.bc -o %t/sh4-sh4al.bc
; RUN: llvm-link %t/sh4a.bc %t/sh4al.bc -o %t/sh4a-sh4al.bc
; RUN: llvm-link %t/st40.bc %t/sh4al.bc -o %t/st40-sh4al.bc
; RUN: llvm-dis %t/sh4-sh4al.bc -o - | FileCheck %s --check-prefixes=MIXED,SH4
; RUN: llvm-dis %t/sh4a-sh4al.bc -o - | FileCheck %s --check-prefixes=MIXED,SH4A
; RUN: llvm-dis %t/st40-sh4al.bc -o - | FileCheck %s --check-prefixes=MIXED,ST40
; RUN: llvm-lto -O2 -j1 -exported-symbol=sh4_entry -exported-symbol=sh4al_entry -exported-symbol=sh4al_base -o %t/sh4-sh4al.o %t/sh4-sh4al.bc
; RUN: llvm-lto -O2 -j1 -exported-symbol=sh4a_entry -exported-symbol=sh4al_entry -exported-symbol=sh4al_base -o %t/sh4a-sh4al.o %t/sh4a-sh4al.bc
; RUN: llvm-lto -O2 -j1 -exported-symbol=st40_entry -exported-symbol=sh4al_entry -exported-symbol=sh4al_base -o %t/st40-sh4al.o %t/st40-sh4al.bc
; RUN: llvm-readobj --file-headers %t/sh4-sh4al.o %t/sh4a-sh4al.o %t/st40-sh4al.o | FileCheck %s --check-prefix=SH4AL-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4al-dsp %t/sh4-sh4al.o | FileCheck %s --check-prefix=CODE
; RUN: not llvm-link %t/sh4al.bc %t/hard.bc -o /dev/null 2>&1 | FileCheck %s --check-prefix=ABI-CONFLICT
; RUN: opt -module-summary %t/sh4.ll -o %t/sh4-thin.bc
; RUN: opt -module-summary %t/sh4a.ll -o %t/sh4a-thin.bc
; RUN: opt -module-summary %t/st40.ll -o %t/st40-thin.bc
; RUN: opt -module-summary %t/sh4al.ll -o %t/sh4al-thin.bc
; RUN: llvm-lto -O2 -j1 --thinlto-action=run -exported-symbol=sh4_entry -exported-symbol=sh4a_entry -exported-symbol=st40_entry -exported-symbol=sh4al_entry -exported-symbol=sh4al_base %t/sh4-thin.bc %t/sh4a-thin.bc %t/st40-thin.bc %t/sh4al-thin.bc
; RUN: llvm-readobj --file-headers %t/sh4-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: llvm-readobj --file-headers %t/sh4a-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH4A-FLAGS
; RUN: llvm-readobj --file-headers %t/st40-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH4A-FLAGS
; RUN: llvm-readobj --file-headers %t/sh4al-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH4AL-FLAGS

; MIXED-DAG: define void @sh4al_entry() #[[SH4AL:[0-9]+]]
; MIXED-DAG: define void @sh4al_base() #[[BASE:[0-9]+]]
; MIXED-DAG: attributes #[[SH4AL]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="{{[^"]*}}+dsp{{[^"]*}}+sh4al-dsp{{[^"]*}}" "tune-cpu"="sh4al-dsp"{{.*}} }
; MIXED-DAG: attributes #[[BASE]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="{{[^"]*}}-dsp{{[^"]*}}-sh4al-dsp{{[^"]*}}" "tune-cpu"="sh4al-dsp"{{.*}} }
; SH4-DAG: attributes {{.*}}"target-cpu"="sh4" "target-features"="{{[^"]*}}-dsp{{[^"]*}}-sh4al-dsp{{[^"]*}}"{{.*}}
; SH4A-DAG: attributes {{.*}}"target-cpu"="sh4a" "target-features"="{{[^"]*}}-dsp{{[^"]*}}-sh4al-dsp{{[^"]*}}"{{.*}}
; ST40-DAG: attributes {{.*}}"target-cpu"="sh4" "target-features"="{{[^"]*}}-dsp{{[^"]*}}-sh4al-dsp{{[^"]*}}+st40-300{{[^"]*}}"{{.*}}

; SH4-FLAGS: Flags [ (0x10)
; SH4A-FLAGS: Flags [ (0x11)
; SH4AL-FLAGS: Flags [ (0x6)
; CODE-LABEL: <sh4_entry>:
; CODE-NOT: padd
; CODE-LABEL: <sh4al_entry>:
; CODE: padd	x0,y0,a0
; CODE-LABEL: <sh4al_base>:
; CODE-NOT: padd
; ABI-CONFLICT: linking module flags 'target-abi': IDs have conflicting values

;--- sh4.ll
target triple = "shle-unknown-none-elf"

define void @sh4_entry() #0 {
	ret void
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-dsp,-sh4al-dsp,-single-fpu,-double-fpu" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- sh4a.ll
target triple = "shle-unknown-none-elf"

define void @sh4a_entry() #0 {
	ret void
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh4a" "target-features"="-dsp,-sh4al-dsp,-single-fpu,-double-fpu" "tune-cpu"="sh4a" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- st40.ll
target triple = "shle-unknown-none-elf"

define void @st40_entry() #0 {
	ret void
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-dsp,-sh4al-dsp,+st40-300,-single-fpu,-double-fpu" "tune-cpu"="st40-300" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- sh4al.ll
target triple = "shle-unknown-none-elf"

define void @sh4al_entry() #0 {
	call void asm sideeffect "padd x0,y0,a0", "~{a0},~{x0},~{y0},~{dsr}"()
	ret void
}

define void @sh4al_base() #1 {
	ret void
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="+dsp,+sh4al-dsp" "tune-cpu"="sh4al-dsp" "use-soft-float"="true" }
attributes #1 = { noinline "target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="-dsp,-sh4al-dsp" "tune-cpu"="sh4al-dsp" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- hard.ll
target triple = "shle-unknown-none-elf"

define void @hard() #0 {
	ret void
}

attributes #0 = { "target-abi"="gnu-hard-float" "target-cpu"="sh4" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}
