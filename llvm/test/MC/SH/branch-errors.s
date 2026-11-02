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
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/unresolved-odd.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNRESOLVED-ALIGN
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/unresolved-odd.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNRESOLVED-ALIGN
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %t/external.s -o %t/external.o
! RUN: llvm-readobj --relocations --hex-dump=.text %t/external.o | FileCheck %s --check-prefixes=EXTERNAL,EXTERNAL-BE
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %t/external.s -o %t/external-le.o
! RUN: llvm-readobj --relocations --hex-dump=.text %t/external-le.o | FileCheck %s --check-prefixes=EXTERNAL,EXTERNAL-LE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %t/cross-section.s -o %t/cross-section.o
! RUN: llvm-readobj --relocations --hex-dump=.text %t/cross-section.o | FileCheck %s --check-prefixes=CROSS,CROSS-BE
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %t/cross-section.s -o %t/cross-section-le.o
! RUN: llvm-readobj --relocations --hex-dump=.text %t/cross-section-le.o | FileCheck %s --check-prefixes=CROSS,CROSS-LE

! RANGE: error: SH branch target is out of range
! ALIGN: error: SH branch target must be two-byte aligned
! UNRESOLVED-ALIGN: error: SH unresolved branch addend must be two-byte aligned
! EXTERNAL: 0x0 R_SH_IND12W external_symbol 0xFFFFFFFC
! EXTERNAL-BE: 0x00000000 a000
! EXTERNAL-LE: 0x00000000 00a0
! CROSS: 0x0 R_SH_DIR8WPN data_target 0x0
! CROSS-BE: 0x00000000 8900
! CROSS-LE: 0x00000000 0089

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

!--- unresolved-odd.s
bra external_symbol+1
