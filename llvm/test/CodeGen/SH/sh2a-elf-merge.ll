; RUN: split-file %s %t
; RUN: llc -filetype=obj %t/compatible.ll -o %t.compatible.o
; RUN: llvm-readobj --file-headers %t.compatible.o | FileCheck %s --check-prefix=SH2A-NOFPU
; RUN: not llc -filetype=obj %t/sh3-incompatible.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INCOMPATIBLE
; RUN: not llc -filetype=obj %t/sh4-incompatible.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INCOMPATIBLE
; RUN: llc -filetype=obj %t/tune-only.ll -o %t.tune.o
; RUN: llvm-readobj --file-headers %t.tune.o | FileCheck %s --check-prefix=SH2

; SH2A-NOFPU: Flags [ (0x13)
; SH2: Flags [ (0x2)
; INCOMPATIBLE: error: SH-2A cannot be merged with the requested SH architecture

;--- compatible.ll
target datalayout = "E-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "sh-unknown-none-elf"

define i32 @sh2_entry() #0 {
	ret i32 1
}

define i32 @sh2a_entry() #1 {
	ret i32 524287
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh2" "use-soft-float"="true" }
attributes #1 = { noinline "target-abi"="gnu" "target-cpu"="sh2a" "target-features"="+dynamic-shift,+prefetch,+sh2,+sh2a,-double-fpu,-fpu-register-bank,-fpu-single-mode,-fpu-size-mode,-fpu-vector,-fsqrt,-single-fpu" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- sh3-incompatible.ll
target datalayout = "E-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "sh-unknown-none-elf"

define i32 @sh2a_entry() #0 {
	ret i32 524287
}

define i32 @sh3_entry() #1 {
	ret i32 1
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh2a" "target-features"="+dynamic-shift,+prefetch,+sh2,+sh2a,-double-fpu,-fpu-register-bank,-fpu-single-mode,-fpu-size-mode,-fpu-vector,-fsqrt,-single-fpu" "use-soft-float"="true" }
attributes #1 = { noinline "target-abi"="gnu" "target-cpu"="sh3" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- sh4-incompatible.ll
target datalayout = "E-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "sh-unknown-none-elf"

define double @sh2a_entry(double %value) #0 {
	ret double %value
}

define double @sh4_entry(double %value) #1 {
	ret double %value
}

attributes #0 = { noinline "target-abi"="gnu-hard-float" "target-cpu"="sh2a" }
attributes #1 = { noinline "target-abi"="gnu-hard-float" "target-cpu"="sh4" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float"}

;--- tune-only.ll
target datalayout = "E-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "sh-unknown-none-elf"

define i32 @sh2_tuned_for_sh2a() #0 {
	ret i32 524287
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh2" "tune-cpu"="sh2a" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}
