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
! RUN: rm -f %t/external.o %t/external-le.o %t/cross.o %t/cross-le.o
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/external.s -o %t/external.o 2>&1 | FileCheck %s --check-prefix=RELOC
! RUN: not test -e %t/external.o
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/external.s -o %t/external-le.o 2>&1 | FileCheck %s --check-prefix=RELOC
! RUN: not test -e %t/external-le.o
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/cross.s -o %t/cross.o 2>&1 | FileCheck %s --check-prefix=RELOC
! RUN: not test -e %t/cross.o
! RUN: not llvm-mc -triple=shle-unknown-elf -filetype=obj %t/cross.s -o %t/cross-le.o 2>&1 | FileCheck %s --check-prefix=RELOC
! RUN: not test -e %t/cross-le.o
! RUN: not llvm-mc -triple=sh-unknown-elf -show-encoding %t/syntax.s 2>&1 | FileCheck %s --check-prefix=SYNTAX

! RANGE: error: SH branch target is out of range
! ALIGN: error: SH branch target must be two-byte aligned
! RELOC: error: SH branch relocations are not yet supported; SH call relocations are not yet supported
! RELOC-NOT: R_SH_NONE
! RELOC-NOT: LLVM ERROR
! RELOC-NOT: assertion
! SYNTAX: error: invalid operand for instruction
! SYNTAX: error: expected GPR after '@'
! SYNTAX: error: invalid operand for instruction
! SYNTAX: error: expected '-' after '@'
! SYNTAX: error: expected '+' after post-increment GPR
! SYNTAX: error: invalid operand for instruction
! SYNTAX: error: unrecognized instruction mnemonic
! SYNTAX: error: unrecognized instruction mnemonic
! SYNTAX: error: unrecognized instruction mnemonic
! SYNTAX: error: unrecognized instruction mnemonic
! SYNTAX: error: unrecognized instruction mnemonic
! SYNTAX: error: invalid register name
! SYNTAX: error: invalid register name
! SYNTAX: error: expected GPR after '@'

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

!--- syntax.s
jsr r4
jsr @pr
sts.l r0,@-r15
sts.l pr,@r15
lds.l @r15,pr
lds.l @r15+,r0
bsrf r4
braf r4
jmp @r4
sts pr,r0
lds r0,pr
sts.l PR,@-r15
lds.l @r15+,PR
jsr @R4
