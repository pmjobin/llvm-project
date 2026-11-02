! RUN: split-file %s %t
! RUN: rm -f %t/cross-section.o %t/undefined.o %t/defined-addition.o
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/cross-section.s -o %t/cross-section.o 2>&1 | FileCheck %s --check-prefix=CROSS
! RUN: not test -e %t/cross-section.o
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/cross-section.s -o %t/cross-section.o 2>&1 | FileCheck %s --check-prefix=CROSS
! RUN: not test -e %t/cross-section.o
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/undefined.s -o %t/undefined.o 2>&1 | FileCheck %s --check-prefix=UNDEFINED
! RUN: not test -e %t/undefined.o
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/undefined.s -o %t/undefined.o 2>&1 | FileCheck %s --check-prefix=UNDEFINED
! RUN: not test -e %t/undefined.o
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/defined-addition.s -o %t/defined-addition.o 2>&1 | FileCheck %s --check-prefix=UNDEFINED
! RUN: not test -e %t/defined-addition.o
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/defined-addition.s -o %t/defined-addition.o 2>&1 | FileCheck %s --check-prefix=UNDEFINED
! RUN: not test -e %t/defined-addition.o

! CROSS: error: SH R_SH_REL32 subtraction across sections is not supported
! UNDEFINED: error: SH R_SH_REL32 subtraction symbol must be defined
! CROSS-NOT: R_SH_NONE
! UNDEFINED-NOT: R_SH_NONE

!--- cross-section.s
.text
.long external - data_label

.data
data_label:
.long 0

!--- undefined.s
.text
.long external_a - external_b

!--- defined-addition.s
.text
.globl defined_add
defined_add:
nop
.long defined_add - missing_sub
