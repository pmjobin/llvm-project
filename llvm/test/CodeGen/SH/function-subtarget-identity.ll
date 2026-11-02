; RUN: split-file %s %t
; RUN: llc -mtriple=shle-unknown-none-elf -O2 -filetype=obj %t/sh3.ll -o %t/sh3.o
; RUN: llvm-readobj --file-headers %t/sh3.o | FileCheck %s --check-prefix=SH3-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh3 %t/sh3.o | FileCheck %s --check-prefix=SH3-CODE
; RUN: llc -mtriple=shle-unknown-none-elf -O2 -filetype=obj %t/sh4.ll -o %t/sh4.o
; RUN: llvm-readobj --file-headers %t/sh4.o | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t/sh4.o | FileCheck %s --check-prefix=SH4-CODE
; RUN: llc -mtriple=shle-unknown-none-elf -O2 -filetype=obj %t/mixed.ll -o %t/mixed.o
; RUN: llvm-readobj --file-headers %t/mixed.o | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: opt %t/sh3.ll -o %t/sh3.bc
; RUN: llvm-lto2 run %t/sh3.bc -o %t/sh3-lto -r %t/sh3.bc,dynamic_left,plx -r %t/sh3.bc,dynamic_right,plx
; RUN: llvm-readobj --file-headers %t/sh3-lto.0 | FileCheck %s --check-prefix=SH3-FLAGS
; RUN: opt %t/sh4.ll -o %t/sh4.bc
; RUN: llvm-lto2 run %t/sh4.bc -o %t/sh4-lto -r %t/sh4.bc,soft_add,plx
; RUN: llvm-readobj --file-headers %t/sh4-lto.0 | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t/sh4-lto.0 | FileCheck %s --check-prefix=SH4-CODE
; RUN: opt -module-summary %t/sh4.ll -o %t/sh4-thin.bc
; RUN: llvm-lto2 run %t/sh4-thin.bc -o %t/sh4-thin -r %t/sh4-thin.bc,soft_add,plx
; RUN: llvm-readobj --file-headers %t/sh4-thin.1 | FileCheck %s --check-prefix=SH4-FLAGS
; RUN: llvm-objdump -dr --mcpu=sh4 %t/sh4-thin.1 | FileCheck %s --check-prefix=SH4-CODE

; SH3-FLAGS: Flags [ (0x3)
; SH3-CODE: shld
; SH3-CODE: shad
; SH4-FLAGS: Flags [ (0x9)
; SH4-CODE-LABEL: <soft_add>:
; SH4-CODE: sts fpscr
; SH4-CODE: fadd fr{{[0-9]+}},fr{{[0-9]+}}
; SH4-CODE-NOT: __adddf3

;--- sh3.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i32 @dynamic_left(i32 %value, i32 %count) #0 {
	%result = shl i32 %value, %count
	ret i32 %result
}

define i32 @dynamic_right(i32 %value, i32 %count) #0 {
	%result = ashr i32 %value, %count
	ret i32 %result
}

attributes #0 = { "target-cpu"="sh3" }

;--- sh4.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define double @soft_add(double %left, double %right) #0 {
	%result = fadd double %left, %right
	ret double %result
}

attributes #0 = { "target-cpu"="sh4" "use-soft-float"="true" }

;--- mixed.ll
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

define i32 @sh3_function(i32 %value, i32 %count) #0 {
	%result = shl i32 %value, %count
	ret i32 %result
}

define i32 @sh4_function(i32 %value, i32 %count) #1 {
	%result = lshr i32 %value, %count
	ret i32 %result
}

attributes #0 = { "target-cpu"="sh3" }
attributes #1 = { "target-cpu"="sh4" }
