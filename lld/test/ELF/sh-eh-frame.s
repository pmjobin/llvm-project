# REQUIRES: sh
# RUN: llvm-mc -triple=shle-unknown-none-elf -mcpu=sh4 -filetype=obj %s -o %t.o
# RUN: ld.lld --image-base=0 -Ttext=0x1000 --gc-sections %t.o -o %t
# RUN: llvm-dwarfdump --eh-frame %t | FileCheck %s
# RUN: ld.lld -r %t.o -o %t.r.o
# RUN: ld.lld --image-base=0 -Ttext=0x1000 --gc-sections %t.r.o -o %t.r
# RUN: llvm-dwarfdump --eh-frame %t.r | FileCheck %s

## This checks relocation of unwind metadata, not runtime unwinder support.
# CHECK: FDE cie={{.*}} pc=00001000...00001004
# CHECK-NOT: FDE

.section .text.start,"ax",@progbits
.globl _start
_start:
.cfi_startproc
.cfi_def_cfa r15,0
 rts
 nop
.cfi_endproc
.section .text.dead,"ax",@progbits
.globl dead
dead:
.cfi_startproc
.cfi_def_cfa r15,0
 rts
 nop
.cfi_endproc
