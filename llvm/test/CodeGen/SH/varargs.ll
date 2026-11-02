; Current LLVM requires frontends to expand aggregate va_arg.
; The original cases now use legal per-endian IR in Inputs; scalar va_arg
; instructions and ABI/stack checks are retained. No verifier is disabled here.
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %S/Inputs/varargs-be.ll -o - | FileCheck %s --check-prefix=SH
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs %S/Inputs/varargs-le.ll -o - | FileCheck %s --check-prefix=SHLE
; SH-LABEL: sum:
; SH-DAG: mov r5,r0
; SH-DAG: mov.l r0,@(4,r15)
; SH-DAG: mov.l r6,@(8,r15)
; SH-DAG: mov.l r7,@(12,r15)
; SHLE-LABEL: sum:
; SHLE-DAG: mov r5,r0
; SHLE-DAG: mov.l r0,@(4,r15)
; SHLE-DAG: mov.l r6,@(8,r15)
; SHLE-DAG: mov.l r7,@(12,r15)
; SH-LABEL: after_four:
; SH: mov r15,r0
; SH-NEXT: add #8,r0
; SH-NEXT: mov.l r0,@r15
; SH-NEXT: mov.l @(4,r15),r0
; SHLE-LABEL: after_four:
; SHLE: mov r15,r0
; SHLE-NEXT: add #8,r0
; SHLE-NEXT: mov.l r0,@r15
; SHLE-NEXT: mov.l @(4,r15),r0
; SH-LABEL: take_split_i64:
; SH: mov.l r7,@(4,r15)
; SH: mov.l @(8,r15),r0
; SHLE-LABEL: take_split_i64:
; SHLE: mov r7,r0
; SHLE: mov.l r0,@(4,r15)
; SH-LABEL: take_s5:
; SH-DAG: mov.l r7,@(20,r15)
; SH-DAG: mov.l r1,@(16,r15)
; SH-DAG: mov.l r1,@(8,r15)
; SH-DAG: mov.l r0,@(12,r15)
; SHLE-LABEL: take_s5:
; SHLE-DAG: mov.l r7,@(20,r15)
; SHLE-DAG: mov.l r6,@(16,r15)
; SHLE-DAG: mov.l r6,@(8,r15)
; SHLE-DAG: mov.l r0,@(12,r15)
; SH-LABEL: copy_args:
; SH-NOT: __va_
; SHLE-LABEL: copy_args:
; SHLE-NOT: __va_
; SH-LABEL: byval_then_vararg:
; SH-DAG: mov r6,r0
; SH-DAG: mov.l r7,
; SHLE-LABEL: byval_then_vararg:
; SHLE-DAG: mov r6,r0
; SHLE-DAG: mov.l r7,
; SH-LABEL: sret_vararg:
; SH-DAG: mov.l r5,
; SH-DAG: mov.l r6,
; SH-DAG: mov.l r7,
; SH-DAG: mov r2,r0
; SHLE-LABEL: sret_vararg:
; SHLE-DAG: mov.l r5,
; SHLE-DAG: mov.l r6,
; SHLE-DAG: mov.l r7,
; SHLE-DAG: mov r2,r0
; SH-LABEL: caller:
; SH: add #-8,r15
; SH: mov #1,r4
; SH: mov #2,r5
; SH: mov #3,r6
; SH: jsr @r0
; SH-NEXT: mov #4,r7
; SH: add #8,r15
; SHLE-LABEL: caller:
; SHLE: add #-8,r15
; SHLE: mov #1,r4
; SHLE: mov #2,r5
; SHLE: mov #3,r6
; SHLE: jsr @r0
; SHLE-NEXT: mov #4,r7
; SHLE: add #8,r15
