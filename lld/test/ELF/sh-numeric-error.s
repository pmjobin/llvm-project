# REQUIRES: sh
# RUN: llvm-mc -triple=shle-unknown-none-elf -mcpu=sh4 -filetype=obj %s -o %t.o
# RUN: not ld.lld -Ttext=0 --image-base=0 %t.o -o /dev/null 2>&1 | FileCheck %s
# RUN: not ld.lld -r %t.o -o /dev/null 2>&1 | FileCheck %s

## Preserve compile-only numeric operand behavior, but reject the absolute
## short PC-relative relocations rather than discard the target in r_addend.
# CHECK-COUNT-3: absolute SH short PC-relative targets are not supported; use a code or data label
.globl _start
_start:
 bt 4
 mov.l 4,r0
 mov.w 4,r1
