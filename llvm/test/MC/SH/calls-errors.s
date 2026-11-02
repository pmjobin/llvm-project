! RUN: split-file %s %t
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %t/pos-boundary.s -o /dev/null
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %t/neg-boundary.s -o /dev/null
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %t/pos-boundary.s -o /dev/null
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %t/neg-boundary.s -o /dev/null
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/pos-overflow.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/neg-overflow.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/pos-overflow.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/neg-overflow.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/odd.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=ALIGN
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/odd.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=ALIGN
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/unresolved-odd.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNRESOLVED-ALIGN
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/unresolved-odd.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNRESOLVED-ALIGN
! RUN: not llvm-mc -triple=sh-unknown-elf -show-encoding %t/syntax.s 2>&1 | FileCheck %s --check-prefix=SYNTAX
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %t/external.s -o %t/external.o
! RUN: llvm-readobj --relocations --hex-dump=.text %t/external.o | FileCheck %s --check-prefixes=EXTERNAL,EXTERNAL-BE
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %t/external.s -o %t/external-le.o
! RUN: llvm-readobj --relocations --hex-dump=.text %t/external-le.o | FileCheck %s --check-prefixes=EXTERNAL,EXTERNAL-LE

! RANGE: error: SH branch target is out of range
! ALIGN: error: SH branch target must be two-byte aligned
! UNRESOLVED-ALIGN: error: SH unresolved branch addend must be two-byte aligned
! SYNTAX: error: invalid operand for instruction
! SYNTAX: error: expected GPR after '@'
! SYNTAX: error: invalid operand for instruction
! SYNTAX: error: invalid operand for instruction
! SYNTAX: error: invalid operand for instruction
! SYNTAX: error: invalid register name
! SYNTAX: error: invalid register name
! SYNTAX: error: expected GPR after '@'
! EXTERNAL: 0x0 R_SH_IND12W external_symbol 0xFFFFFFFC
! EXTERNAL-BE: 0x00000000 b000
! EXTERNAL-LE: 0x00000000 00b0

!--- pos-boundary.s
bsr .Ltarget
.space 4096
.Ltarget:
nop

!--- neg-boundary.s
.Ltarget:
nop
.space 4090
bsr .Ltarget

!--- pos-overflow.s
bsr .Ltarget
.space 4098
.Ltarget:
nop

!--- neg-overflow.s
.Ltarget:
nop
.space 4092
bsr .Ltarget

!--- odd.s
bsr .Lodd
.byte 0
.Lodd:
nop

!--- external.s
bsr external_symbol

!--- cross.s
.text
bsr data_target
.data
data_target:
.long 0

!--- unresolved-odd.s
bsr external_symbol+1

!--- syntax.s
jsr r4
jsr @pr
sts.l r0,@-r15
sts.l pr,@r15
lds.l @r15,pr
lds.l @r15+,r0
sts.l PR,@-r15
lds.l @r15+,PR
jsr @R4
