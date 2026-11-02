; RUN: split-file %s %t
; RUN: opt %t/nofpu.ll -o %t/nofpu-full.bc
; RUN: llvm-lto -O2 -j1 -exported-symbol=nofpu_entry -o %t/nofpu-full.o %t/nofpu-full.bc
; RUN: llvm-readobj --file-headers %t/nofpu-full.o | FileCheck %s --check-prefix=NOFPU-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t/nofpu-full.o | FileCheck %s --check-prefix=NOFPU
; RUN: opt -module-summary %t/nofpu.ll -o %t/nofpu-thin.bc
; RUN: llvm-lto -O2 -j2 --thinlto-action=run -exported-symbol=nofpu_entry %t/nofpu-thin.bc
; RUN: llvm-readobj --file-headers %t/nofpu-thin.bc.thinlto.o | FileCheck %s --check-prefix=NOFPU-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t/nofpu-thin.bc.thinlto.o | FileCheck %s --check-prefix=NOFPU
; RUN: opt %t/single.ll -o %t/single-full.bc
; RUN: llvm-lto -O2 -j1 -exported-symbol=single_entry -o %t/single-full.o %t/single-full.bc
; RUN: llvm-readobj --file-headers %t/single-full.o | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t/single-full.o | FileCheck %s --check-prefix=SINGLE
; RUN: opt -module-summary %t/single.ll -o %t/single-thin.bc
; RUN: llvm-lto -O2 -j2 --thinlto-action=run -exported-symbol=single_entry %t/single-thin.bc
; RUN: llvm-readobj --file-headers %t/single-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t/single-thin.bc.thinlto.o | FileCheck %s --check-prefix=SINGLE
; RUN: opt %t/e.ll -o %t/e-full.bc
; RUN: llvm-lto -O2 -j1 -exported-symbol=e_entry -o %t/e-full.o %t/e-full.bc
; RUN: llvm-readobj --file-headers %t/e-full.o | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t/e-full.o | FileCheck %s --check-prefix=E
; RUN: opt -module-summary %t/e.ll -o %t/e-thin.bc
; RUN: llvm-lto -O2 -j2 --thinlto-action=run -exported-symbol=e_entry %t/e-thin.bc
; RUN: llvm-readobj --file-headers %t/e-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t/e-thin.bc.thinlto.o | FileCheck %s --check-prefix=E
; RUN: opt %t/soft.ll -o %t/soft.bc
; RUN: llvm-link %t/soft.bc %t/nofpu-full.bc -o %t/soft-nofpu.bc
; RUN: llvm-dis %t/soft-nofpu.bc -o - | FileCheck %s --check-prefix=SOFT-NOFPU
; RUN: not llvm-link %t/single-full.bc %t/e-full.bc -o /dev/null 2>&1 | FileCheck %s --check-prefix=MIXED
; RUN: not llvm-link %t/single-full.bc %t/soft.bc -o /dev/null 2>&1 | FileCheck %s --check-prefix=MIXED

; NOFPU-FLAGS: Flags [ (0x10)
; SH4-FLAGS: Flags [ (0x9)

; NOFPU-LABEL: <nofpu_entry>:
; NOFPU: R_SH_DIR32 __addsf3
; NOFPU: R_SH_DIR32 __adddf3
; NOFPU-NOT: fadd
; NOFPU-NOT: fpscr

; SINGLE-LABEL: <single_entry>:
; SINGLE: fadd fr{{[0-9]+}},fr{{[0-9]+}}
; SINGLE: fcnvsd fpul,dr{{[0-9]+}}
; SINGLE: fadd fr{{[0-9]+}},fr{{[0-9]+}}
; Read the current status after the double arithmetic before restoring PR0.
; SINGLE: sts fpscr
; SINGLE: lds {{r[0-9]+}},fpscr

; E-LABEL: <e_entry>:
; E: fadd fr{{[0-9]+}},fr{{[0-9]+}}
; E: R_SH_DIR32 __adddf3
; E-NOT: fadd dr
; E-NOT: fmov.d

; SOFT-NOFPU: "target-abi"="gnu" "target-cpu"="sh4" "target-features"="+single-fpu,+fsqrt,+fpu-size-mode,+double-fpu,+fpu-register-bank,+fpu-vector,-fpu-single-mode"
; SOFT-NOFPU: "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode"
; SOFT-NOFPU: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu"}
; MIXED: linking module flags 'target-abi': IDs have conflicting values

;--- nofpu.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define double @nofpu_entry(float %single, double %wide) #0 {
	%single.sum = fadd float %single, %single
	%extended = fpext float %single.sum to double
	%result = fadd double %extended, %wide
	ret double %result
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh4" "target-features"="-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}

;--- single.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define double @single_entry(float %single, double %wide) #0 {
	%single.sum = fadd float %single, %single
	%extended = fpext float %single.sum to double
	%result = fadd double %extended, %wide
	ret double %result
}

attributes #0 = { noinline "target-abi"="gnu-hard-float-single" "target-cpu"="sh4" "target-features"="+single-fpu,+fsqrt,+fpu-size-mode,+double-fpu,+fpu-register-bank,+fpu-vector,+fpu-single-mode" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-hard-float-single"}

;--- e.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define double @e_entry(float %single, double %wide) #0 {
	%single.sum = fadd float %single, %single
	%extended = fpext float %single.sum to double
	%result = fadd double %extended, %wide
	ret double %result
}

attributes #0 = { noinline "target-abi"="gnu-e-float" "target-cpu"="sh4" "target-features"="+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu-e-float"}

;--- soft.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define float @soft_entry(float %left, float %right) #0 {
	%result = fadd float %left, %right
	ret float %result
}

attributes #0 = { noinline "target-abi"="gnu" "target-cpu"="sh4" "target-features"="+single-fpu,+fsqrt,+fpu-size-mode,+double-fpu,+fpu-register-bank,+fpu-vector,-fpu-single-mode" "use-soft-float"="true" }

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"gnu"}
