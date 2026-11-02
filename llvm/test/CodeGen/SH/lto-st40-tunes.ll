; RUN: split-file %s %t
; RUN: opt %t/generic.ll -o %t/generic.bc
; RUN: opt %t/st40-100.ll -o %t/st40-100.bc
; RUN: opt %t/st40-200.ll -o %t/st40-200.bc
; RUN: opt %t/st40-300.ll -o %t/st40-300.bc
; RUN: llvm-link %t/generic.bc %t/st40-100.bc %t/st40-200.bc %t/st40-300.bc -o %t/mixed.bc
; RUN: llvm-dis %t/mixed.bc -o - | FileCheck %s --check-prefix=MIXED
; RUN: llvm-lto -O2 -j1 -exported-symbol=generic_entry -exported-symbol=st40_100_entry -exported-symbol=st40_200_entry -exported-symbol=st40_300_entry -o %t/mixed.o %t/mixed.bc
; RUN: llvm-readobj --file-headers %t/mixed.o | FileCheck %s --check-prefix=FLAGS
; RUN: opt -module-summary %t/mixed.bc -o %t/mixed-thin.bc
; RUN: llvm-lto -O2 -j1 --thinlto-action=run -exported-symbol=generic_entry -exported-symbol=st40_100_entry -exported-symbol=st40_200_entry -exported-symbol=st40_300_entry %t/mixed-thin.bc
; RUN: llvm-readobj --file-headers %t/mixed-thin.bc.thinlto.o | FileCheck %s --check-prefix=FLAGS
; RUN: opt %t/inline.ll -passes='cgscc(inline)' -S -o - | FileCheck %s --check-prefix=INLINE
; RUN: not llvm-link %t/st40-100.bc %t/incompatible.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INCOMPATIBLE

; MIXED: define i32 @generic_entry(i32 %value) #[[GENERIC:[0-9]+]]
; MIXED: define i32 @st40_100_entry(i32 %value) #[[ST100:[0-9]+]]
; MIXED: define i32 @st40_200_entry(i32 %value) #[[ST200:[0-9]+]]
; MIXED: define i32 @st40_300_entry(i32 %value) #[[ST300:[0-9]+]]
; MIXED: attributes #[[GENERIC]] = { {{.*}}"target-abi"="gnu-hard-float" "target-cpu"="sh4"{{.*}} }
; MIXED: attributes #[[ST100]] = { {{.*}}"target-abi"="gnu-hard-float" "target-cpu"="sh4"{{.*}}"tune-cpu"="st40-100"{{.*}} }
; MIXED: attributes #[[ST200]] = { {{.*}}"target-abi"="gnu-hard-float" "target-cpu"="sh4"{{.*}}"tune-cpu"="st40-200"{{.*}} }
; MIXED: attributes #[[ST300]] = { {{.*}}"target-abi"="gnu-hard-float" "target-cpu"="sh4"{{.*}}"target-features"="{{[^"]*}}+st40-300{{[^"]*}}"{{.*}}"tune-cpu"="st40-300"{{.*}} }
; FLAGS: Flags [ (0xC)
; INLINE: define i32 @caller(i32 %value) #[[CALLER:[0-9]+]] {
; INLINE-NOT: call
; INLINE: add i32 %value, 1
; INLINE: attributes #[[CALLER]] = { {{.*}}"target-cpu"="sh4"{{.*}}"tune-cpu"="st40-200"{{.*}} }
; INCOMPATIBLE: linking module flags 'target-abi': IDs have conflicting values

;--- generic.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i32 @generic_entry(i32 %value) #0 {
	%result = add i32 %value, 1
	ret i32 %result
}

attributes #0 = { noinline "target-abi"="gnu-hard-float" "target-cpu"="sh4" "target-features"="+single-fpu,+fsqrt,+fpu-size-mode,+double-fpu,+fpu-register-bank,+fpu-vector,-fpu-single-mode" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}

;--- st40-100.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i32 @st40_100_entry(i32 %value) #0 {
	%result = add i32 %value, 1
	ret i32 %result
}

attributes #0 = { noinline "target-abi"="gnu-hard-float" "target-cpu"="sh4" "target-features"="+single-fpu,+fsqrt,+fpu-size-mode,+double-fpu,+fpu-register-bank,+fpu-vector,-fpu-single-mode" "tune-cpu"="st40-100" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}

;--- st40-200.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i32 @st40_200_entry(i32 %value) #0 {
	%result = add i32 %value, 1
	ret i32 %result
}

attributes #0 = { noinline "target-abi"="gnu-hard-float" "target-cpu"="sh4" "target-features"="+single-fpu,+fsqrt,+fpu-size-mode,+double-fpu,+fpu-register-bank,+fpu-vector,-fpu-single-mode" "tune-cpu"="st40-200" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}

;--- st40-300.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i32 @st40_300_entry(i32 %value) #0 {
	fence seq_cst
	%result = add i32 %value, 1
	ret i32 %result
}

attributes #0 = { noinline "target-abi"="gnu-hard-float" "target-cpu"="sh4" "target-features"="+single-fpu,+fsqrt,+fpu-size-mode,+double-fpu,+fpu-register-bank,+fpu-vector,-fpu-single-mode,+st40-300" "tune-cpu"="st40-300" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}

;--- inline.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define internal i32 @inlinee(i32 %value) alwaysinline #0 {
	%result = add i32 %value, 1
	ret i32 %result
}

define i32 @caller(i32 %value) #1 {
	%result = call i32 @inlinee(i32 %value)
	ret i32 %result
}

attributes #0 = { alwaysinline "target-abi"="gnu-hard-float" "target-cpu"="sh4" "tune-cpu"="st40-100" }
attributes #1 = { "target-abi"="gnu-hard-float" "target-cpu"="sh4" "tune-cpu"="st40-200" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}

;--- incompatible.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i32 @incompatible(i32 %value) #0 {
	ret i32 %value
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4" "tune-cpu"="st40-100" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}
