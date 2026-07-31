! RUN: split-file %s %t
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/numeric.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=NUMERIC
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/destination.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=DEST
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/backward.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=BACKWARD
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/unaligned.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNALIGNED
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/range.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/unresolved.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNRESOLVED
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/syntax.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SYNTAX
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/incomplete.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=INCOMPLETE

! NUMERIC-COUNT-3: error: SH PC-relative literal displacement must be a multiple of 4 in the range [0, 1020]
! DEST: error: operand must be r0
! BACKWARD: error: SH PC-relative literal target is behind the instruction
! UNALIGNED: error: SH PC-relative literal target must be four-byte aligned
! RANGE: error: SH PC-relative literal target is out of range
! UNRESOLVED: error: SH PC-relative literal target must be defined in the same section
! SYNTAX: error: expected comma in longword memory operand
! SYNTAX: error: expected comma in longword memory operand
! SYNTAX: error: invalid register name
! INCOMPLETE: error: expected operand

!--- numeric.s
mova @(-4,pc),r0
mova @(2,pc),r0
mova @(1024,pc),r0

!--- destination.s
mova @(0,pc),r1

!--- backward.s
.Lbehind:
.long 0
mova .Lbehind,r0

!--- unaligned.s
mova .Lunaligned,r0
nop
nop
.Lunaligned:
.short 0

!--- range.s
mova .Lfar,r0
.space 1026
.Lfar:
.long 0

!--- unresolved.s
mova external_symbol,r0

!--- syntax.s
mova @(12),r0
mova @(12 pc),r0
mova @(12,pcr),r0

!--- incomplete.s
mova @(12,pc),
