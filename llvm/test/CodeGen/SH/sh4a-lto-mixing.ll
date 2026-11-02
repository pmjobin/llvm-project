; RUN: split-file %s %t
; RUN: opt %t/sh4.ll -o %t/sh4.bc
; RUN: opt %t/sh4a.ll -o %t/sh4a.bc
; RUN: opt %t/sh2a.ll -o %t/sh2a.bc
; RUN: opt %t/st40.ll -o %t/st40.bc
; RUN: llvm-link %t/sh4.bc %t/sh4a.bc -o %t/sh4-sh4a.bc
; RUN: llvm-dis %t/sh4-sh4a.bc -o - | FileCheck %s --check-prefix=SH4-SH4A-ATTR
; RUN: llvm-lto -O2 -j1 -exported-symbol=sh4_entry -exported-symbol=sh4a_entry -o %t/sh4-sh4a.o %t/sh4-sh4a.bc
; RUN: llvm-readobj --file-headers %t/sh4-sh4a.o | FileCheck %s --check-prefix=SH4A-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4a %t/sh4-sh4a.o | FileCheck %s --check-prefix=SH4-SH4A-CODE
; RUN: llvm-link %t/st40.bc %t/sh4a.bc -o %t/st40-sh4a.bc
; RUN: llvm-dis %t/st40-sh4a.bc -o - | FileCheck %s --check-prefix=ST40-SH4A-ATTR
; RUN: llvm-lto -O2 -j1 -exported-symbol=st40_entry -exported-symbol=sh4a_entry -o %t/st40-sh4a.o %t/st40-sh4a.bc
; RUN: llvm-readobj --file-headers %t/st40-sh4a.o | FileCheck %s --check-prefix=SH4A-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4a %t/st40-sh4a.o | FileCheck %s --check-prefix=ST40-SH4A-CODE
; RUN: llvm-link %t/sh2a.bc %t/sh4a.bc -o %t/sh2a-sh4a.bc
; RUN: not llvm-lto -O2 -j1 -exported-symbol=sh2a_entry -exported-symbol=sh4a_entry -o %t/incompatible.o %t/sh2a-sh4a.bc 2>&1 | FileCheck %s --check-prefix=ARCH-CONFLICT
; RUN: opt -module-summary %t/sh4.ll -o %t/sh4-thin.bc
; RUN: opt -module-summary %t/sh4a.ll -o %t/sh4a-thin.bc
; RUN: opt -module-summary %t/sh2a.ll -o %t/sh2a-thin.bc
; RUN: opt -module-summary %t/st40.ll -o %t/st40-thin.bc
; RUN: llvm-lto -O2 -j1 --thinlto-action=run -exported-symbol=sh4_entry -exported-symbol=sh4a_entry -exported-symbol=sh2a_entry -exported-symbol=st40_entry %t/sh4-thin.bc %t/sh4a-thin.bc %t/sh2a-thin.bc %t/st40-thin.bc
; RUN: llvm-readobj --file-headers %t/sh4-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: llvm-readobj --file-headers %t/sh4a-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH4A-FLAGS
; RUN: llvm-readobj --file-headers %t/sh2a-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH2A-FLAGS
; RUN: llvm-readobj --file-headers %t/st40-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH4A-FLAGS

; SH4-SH4A-ATTR: define void @sh4a_entry() #[[SH4A:[0-9]+]]
; SH4-SH4A-ATTR: attributes #[[SH4A]] = { {{.*}}"target-cpu"="sh4a" "target-features"="{{[^"]*}}+sh4a-common{{[^"]*}}+sh4a{{[^"]*}}-st40-300{{[^"]*}}" "tune-cpu"="sh4a"{{.*}} }
; ST40-SH4A-ATTR: define void @sh4a_entry() #[[SH4A:[0-9]+]]
; ST40-SH4A-ATTR: attributes {{.*}}"target-cpu"="sh4" "target-features"="{{[^"]*}}+sh4a-common{{[^"]*}}-sh4a{{[^"]*}}+st40-300{{[^"]*}}" "tune-cpu"="st40-300"{{.*}}
; ST40-SH4A-ATTR: attributes #[[SH4A]] = { {{.*}}"target-cpu"="sh4a" "target-features"="{{[^"]*}}+sh4a-common{{[^"]*}}+sh4a{{[^"]*}}-st40-300{{[^"]*}}" "tune-cpu"="sh4a"{{.*}} }

; SH4-FLAGS: Flags [ (0x10)
; SH4A-FLAGS: Flags [ (0x11)
; SH2A-FLAGS: Flags [ (0x13)

; SH4-SH4A-CODE-LABEL: <sh4_entry>:
; SH4-SH4A-CODE-NOT: synco
; SH4-SH4A-CODE-LABEL: <sh4a_entry>:
; SH4-SH4A-CODE: synco
; ST40-SH4A-CODE-LABEL: <st40_entry>:
; ST40-SH4A-CODE: synco
; ST40-SH4A-CODE-LABEL: <sh4a_entry>:
; ST40-SH4A-CODE: synco

; ARCH-CONFLICT: SH-2A cannot be merged with the requested SH architecture

;--- sh4.ll
target triple = "shle-unknown-none-elf"

define void @sh4_entry() #0 {
	fence seq_cst
	ret void
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-single-fpu,-double-fpu" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- sh4a.ll
target triple = "shle-unknown-none-elf"

define void @sh4a_entry() #0 {
	fence seq_cst
	ret void
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh4a" "target-features"="+sh4a-common,+sh4a,-st40-300,-single-fpu,-double-fpu" "tune-cpu"="sh4a" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- sh2a.ll
target triple = "shle-unknown-none-elf"

define void @sh2a_entry() #0 {
	ret void
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh2a" "target-features"="+sh2a,-single-fpu,-double-fpu" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- st40.ll
target triple = "shle-unknown-none-elf"

define void @st40_entry() #0 {
	fence seq_cst
	ret void
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh4" "target-features"="+sh4a-common,-sh4a,+st40-300,-single-fpu,-double-fpu" "tune-cpu"="st40-300" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}
