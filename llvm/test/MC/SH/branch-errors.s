! RUN: split-file %s %t
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %t/cond-pos-boundary.s -o /dev/null
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %t/cond-neg-boundary.s -o /dev/null
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %t/bra-pos-boundary.s -o /dev/null
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %t/bra-neg-boundary.s -o /dev/null
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %t/cond-pos-boundary.s -o /dev/null
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %t/cond-neg-boundary.s -o /dev/null
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %t/bra-pos-boundary.s -o /dev/null
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %t/bra-neg-boundary.s -o /dev/null
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/cond-pos-overflow.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/cond-neg-overflow.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/bra-pos-overflow.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/bra-neg-overflow.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/cond-pos-overflow.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/cond-neg-overflow.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/bra-pos-overflow.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/bra-neg-overflow.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/odd.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=ALIGN
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/odd.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=ALIGN
! RUN: rm -f %t/external.o %t/external-le.o %t/cross-section.o %t/cross-section-le.o
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/external.s -o %t/external.o 2>&1 | FileCheck %s --check-prefix=RELOC
! RUN: not test -e %t/external.o
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/external.s -o %t/external-le.o 2>&1 | FileCheck %s --check-prefix=RELOC
! RUN: not test -e %t/external-le.o
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/cross-section.s -o %t/cross-section.o 2>&1 | FileCheck %s --check-prefix=RELOC
! RUN: not test -e %t/cross-section.o
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/cross-section.s -o %t/cross-section-le.o 2>&1 | FileCheck %s --check-prefix=RELOC
! RUN: not test -e %t/cross-section-le.o

! RANGE: error: SH branch target is out of range
! ALIGN: error: SH branch target must be two-byte aligned
! RELOC: error: SH unresolved branch relocations are not supported
! RELOC-NOT: R_SH_NONE
! RELOC-NOT: LLVM ERROR
! RELOC-NOT: assertion

!--- cond-pos-boundary.s
bt .Ltarget
.space 256
.Ltarget:
nop

!--- cond-neg-boundary.s
.Ltarget:
nop
.space 250
bf .Ltarget

!--- bra-pos-boundary.s
bra .Ltarget
.space 4096
.Ltarget:
nop

!--- bra-neg-boundary.s
.Ltarget:
nop
.space 4090
bra .Ltarget

!--- cond-pos-overflow.s
bt .Ltarget
.space 258
.Ltarget:
nop

!--- cond-neg-overflow.s
.Ltarget:
nop
.space 252
bf .Ltarget

!--- bra-pos-overflow.s
bra .Ltarget
.space 4098
.Ltarget:
nop

!--- bra-neg-overflow.s
.Ltarget:
nop
.space 4092
bra .Ltarget

!--- odd.s
bt .Lodd
.byte 0
.Lodd:
nop

!--- external.s
bra external_symbol

!--- cross-section.s
.text
bt data_target
.data
data_target:
.long 0
