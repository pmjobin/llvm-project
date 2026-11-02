; RUN: split-file %s %t
; RUN: opt %t/established.ll -o %t/established.bc
; RUN: opt %t/remaining.ll -o %t/remaining.bc
; RUN: llvm-link %t/established.bc %t/remaining.bc -o %t/mixed.bc
; RUN: llvm-dis %t/mixed.bc -o - | FileCheck %s --check-prefix=MIXED
; RUN: llvm-lto -O2 -j1 -exported-symbol=generic_entry -exported-symbol=st40_100_entry -exported-symbol=st40_200_entry -exported-symbol=st40_300_entry -exported-symbol=st40_340_entry -exported-symbol=st40_400_entry -exported-symbol=st40_500_entry -o %t/mixed.o %t/mixed.bc
; RUN: llvm-readobj --file-headers %t/mixed.o | FileCheck %s --check-prefix=FLAGS
; RUN: llvm-objdump -d --mcpu=sh4 --mattr=+st40-300 %t/mixed.o | FileCheck %s --check-prefix=CODE
; RUN: opt -module-summary %t/mixed.bc -o %t/mixed-thin.bc
; RUN: llvm-lto -O2 -j1 --thinlto-action=run -exported-symbol=generic_entry -exported-symbol=st40_100_entry -exported-symbol=st40_200_entry -exported-symbol=st40_300_entry -exported-symbol=st40_340_entry -exported-symbol=st40_400_entry -exported-symbol=st40_500_entry %t/mixed-thin.bc
; RUN: llvm-readobj --file-headers %t/mixed-thin.bc.thinlto.o | FileCheck %s --check-prefix=FLAGS
; RUN: llvm-objdump -d --mcpu=sh4 --mattr=+st40-300 %t/mixed-thin.bc.thinlto.o | FileCheck %s --check-prefix=CODE
; RUN: not llvm-link %t/remaining.bc %t/incompatible.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INCOMPATIBLE

; MIXED: define i32 @generic_entry(i32 %value) #[[GENERIC:[0-9]+]]
; MIXED: define i32 @st40_100_entry(i32 %value) #[[ST100:[0-9]+]]
; MIXED: define i32 @st40_200_entry(i32 %value) #[[ST200:[0-9]+]]
; MIXED: define i32 @st40_300_entry(i32 %value) #[[ST300:[0-9]+]]
; MIXED: define i32 @st40_340_entry(i32 %value) #[[ST340:[0-9]+]]
; MIXED: define i32 @st40_400_entry(i32 %value) #[[ST400:[0-9]+]]
; MIXED: define i32 @st40_500_entry(i32 %value) #[[ST500:[0-9]+]]
; MIXED: attributes #[[GENERIC]] = { {{.*}}"target-abi"="gnu" "target-cpu"="sh4"{{.*}} }
; MIXED: attributes #[[ST100]] = { {{.*}}"target-features"="{{[^"]*}}-single-fpu{{[^"]*}}"{{.*}}"tune-cpu"="st40-100"{{.*}} }
; MIXED: attributes #[[ST200]] = { {{.*}}"target-features"="{{[^"]*}}-single-fpu{{[^"]*}}"{{.*}}"tune-cpu"="st40-200"{{.*}} }
; MIXED: attributes #[[ST300]] = { {{.*}}"target-features"="{{[^"]*}}+st40-300{{[^"]*}}-single-fpu{{[^"]*}}"{{.*}}"tune-cpu"="st40-300"{{.*}} }
; MIXED: attributes #[[ST340]] = { {{.*}}"target-features"="{{[^"]*}}-mmu{{[^"]*}}-single-fpu{{[^"]*}}"{{.*}}"tune-cpu"="st40-340"{{.*}} }
; MIXED: attributes #[[ST400]] = { {{.*}}"target-features"="{{[^"]*}}-mmu{{[^"]*}}-single-fpu{{[^"]*}}"{{.*}}"tune-cpu"="st40-400"{{.*}} }
; MIXED: attributes #[[ST500]] = { {{.*}}"target-features"="{{[^"]*}}+mmu{{[^"]*}}-single-fpu{{[^"]*}}"{{.*}}"tune-cpu"="st40-500"{{.*}} }
; FLAGS: Flags [ (0x11)
; CODE-LABEL: <generic_entry>:
; CODE-NOT: synco
; CODE-LABEL: <st40_100_entry>:
; CODE-NOT: synco
; CODE-LABEL: <st40_200_entry>:
; CODE-NOT: synco
; CODE-LABEL: <st40_300_entry>:
; CODE: synco
; CODE-LABEL: <st40_340_entry>:
; CODE-NOT: synco
; CODE-LABEL: <st40_400_entry>:
; CODE-NOT: synco
; CODE-LABEL: <st40_500_entry>:
; CODE-NOT: synco
; INCOMPATIBLE: linking module flags 'target-abi': IDs have conflicting values

;--- established.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i32 @generic_entry(i32 %value) #0 {
	fence seq_cst
	%result = add i32 %value, 1
	ret i32 %result
}

define i32 @st40_100_entry(i32 %value) #1 {
	fence seq_cst
	%result = add i32 %value, 1
	ret i32 %result
}

define i32 @st40_200_entry(i32 %value) #2 {
	fence seq_cst
	%result = add i32 %value, 1
	ret i32 %result
}

define i32 @st40_300_entry(i32 %value) #3 {
	fence seq_cst
	%result = add i32 %value, 1
	ret i32 %result
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh4" "target-features"="+mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "use-soft-float"="true" }
attributes #1 = { noinline "target-abi"="gnu" "target-cpu"="sh4" "target-features"="+mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-100" "use-soft-float"="true" }
attributes #2 = { noinline "target-abi"="gnu" "target-cpu"="sh4" "target-features"="+mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-200" "use-soft-float"="true" }
attributes #3 = { noinline "target-abi"="gnu" "target-cpu"="sh4" "target-features"="+mmu,+st40-300,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-300" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- remaining.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i32 @st40_340_entry(i32 %value) #0 {
	fence seq_cst
	%result = add i32 %value, 1
	ret i32 %result
}

define i32 @st40_400_entry(i32 %value) #1 {
	fence seq_cst
	%result = add i32 %value, 1
	ret i32 %result
}

define i32 @st40_500_entry(i32 %value) #2 {
	fence seq_cst
	%result = add i32 %value, 1
	ret i32 %result
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-340" "use-soft-float"="true" }
attributes #1 = { noinline "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-400" "use-soft-float"="true" }
attributes #2 = { noinline "target-abi"="gnu" "target-cpu"="sh4" "target-features"="+mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "tune-cpu"="st40-500" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- incompatible.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i32 @incompatible(i32 %value) #0 {
	ret i32 %value
}

attributes #0 = { "target-abi"="gnu-hard-float" "target-cpu"="sh4" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}
