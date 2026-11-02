; RUN: split-file %s %t
; RUN: opt %t/sh4.ll -o %t/sh4-full.bc
; RUN: llvm-lto -O2 -j2 -exported-symbol=sh4_entry -o %t/sh4-full.o %t/sh4-full.bc
; RUN: llvm-readobj --file-headers %t/sh4-full.o.0 %t/sh4-full.o.1 | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t/sh4-full.o.0 %t/sh4-full.o.1 | FileCheck %s --check-prefix=SH4-CODE
; RUN: opt -module-summary %t/sh4.ll -o %t/sh4-thin.bc
; RUN: llvm-lto -O2 -j2 --thinlto-action=run -exported-symbol=sh4_entry %t/sh4-thin.bc
; RUN: llvm-readobj --file-headers %t/sh4-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t/sh4-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH4-CODE
; RUN: opt %t/sh2.ll -o %t/sh2-full.bc
; RUN: llvm-lto -O2 -j2 -exported-symbol=sh2_entry -o %t/sh2-full.o %t/sh2-full.bc
; RUN: llvm-readobj --file-headers %t/sh2-full.o.0 %t/sh2-full.o.1 | FileCheck %s --check-prefix=SH2-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh2 %t/sh2-full.o.0 %t/sh2-full.o.1 | FileCheck %s --check-prefix=SH2-CODE
; RUN: opt -module-summary %t/sh2.ll -o %t/sh2-thin.bc
; RUN: llvm-lto -O2 -j2 --thinlto-action=run -exported-symbol=sh2_entry %t/sh2-thin.bc
; RUN: llvm-readobj --file-headers %t/sh2-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH2-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh2 %t/sh2-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH2-CODE
; RUN: opt %t/sh3.ll -o %t/sh3-full.bc
; RUN: llvm-lto -O2 -j2 -exported-symbol=sh3_entry -o %t/sh3-full.o %t/sh3-full.bc
; RUN: llvm-readobj --file-headers %t/sh3-full.o.0 %t/sh3-full.o.1 | FileCheck %s --check-prefix=SH3-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh3 %t/sh3-full.o.0 %t/sh3-full.o.1 | FileCheck %s --check-prefix=SH3-CODE
; RUN: opt -module-summary %t/sh3.ll -o %t/sh3-thin.bc
; RUN: llvm-lto -O2 -j2 --thinlto-action=run -exported-symbol=sh3_entry %t/sh3-thin.bc
; RUN: llvm-readobj --file-headers %t/sh3-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH3-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh3 %t/sh3-thin.bc.thinlto.o | FileCheck %s --check-prefix=SH3-CODE

; Parallel Full LTO must preserve local linkage for SH's local-only fastcc.
; Full and Thin LTO must also retain the per-function canonical CPU identity.

; SH4-FLAGS: Flags [ (0x9)
; SH4-CODE-DAG: fadd fr{{[0-9]+}},fr{{[0-9]+}}
; SH4-CODE-DAG: fmul fr{{[0-9]+}},fr{{[0-9]+}}
; SH4-CODE-DAG: fcmp/gt fr{{[0-9]+}},fr{{[0-9]+}}
; SH4-CODE-DAG: ftrc fr{{[0-9]+}},fpul
; SH4-CODE-DAG: fcnvsd fpul,dr{{[0-9]+}}
; SH4-CODE-DAG: fcnvds dr{{[0-9]+}},fpul
; SH4-CODE-DAG: R_SH_DIR32 external_soft
; SH4-CODE-NOT: __addsf3
; SH4-CODE-NOT: __muldf3

; SH2-FLAGS: Flags [ (0x2)
; SH2-CODE: R_SH_DIR32 __addsf3
; SH2-CODE-NOT: fadd
; SH2-CODE-NOT: fpul
; SH2-CODE-NOT: fpscr

; SH3-FLAGS: Flags [ (0x3)
; SH3-CODE: R_SH_DIR32 __addsf3
; SH3-CODE-NOT: fadd
; SH3-CODE-NOT: fpul
; SH3-CODE-NOT: fpscr

;--- sh4.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

declare float @external_soft(float)

define internal fastcc float @local_f32(float %left, float %right) #0 {
	%result = fadd float %left, %right
	ret float %result
}

define internal fastcc double @local_f64(double %left, double %right) #0 {
	%result = fmul double %left, %right
	ret double %result
}

define i32 @sh4_entry(float %fa, float %fb, double %da, double %db, ptr %sink) #0 {
	%f = call fastcc float @local_f32(float %fa, float %fb)
	store volatile float %f, ptr %sink, align 4
	%d = call fastcc double @local_f64(double %da, double %db)
	store volatile double %d, ptr %sink, align 4
	%called = call float @external_soft(float %f)
	%extended = fpext float %called to double
	%sum = fadd double %d, %extended
	%truncated = fptrunc double %sum to float
	store volatile float %truncated, ptr %sink, align 4
	%comparison = fcmp olt double %sum, %db
	%converted = fptosi double %sum to i32
	%boolean = zext i1 %comparison to i32
	%result = add i32 %converted, %boolean
	ret i32 %result
}

attributes #0 = { noinline "target-cpu"="sh4" "use-soft-float"="true" }

;--- sh2.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define float @sh2_entry(float %left, float %right) #0 {
	%result = fadd float %left, %right
	ret float %result
}

attributes #0 = { noinline "target-cpu"="sh2" "use-soft-float"="true" }

;--- sh3.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define float @sh3_entry(float %left, float %right) #0 {
	%result = fadd float %left, %right
	ret float %result
}

attributes #0 = { noinline "target-cpu"="sh3" "use-soft-float"="true" }
