! RUN: split-file %s %t
! RUN: not llvm-mc -triple=sh-unknown-none-elf -filetype=obj %t/missing.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=MISSING
! RUN: not llvm-mc -triple=sh-unknown-none-elf -filetype=obj %t/extra.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=EXTRA
! RUN: not llvm-mc -triple=sh-unknown-none-elf -filetype=obj %t/invalid.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=INVALID
! RUN: not llvm-mc -triple=sh-unknown-none-elf -filetype=obj %t/quad-reloc.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=QUAD
! RUN: not llvm-mc -triple=sh-unknown-none-elf -filetype=obj %t/overflow.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=OVERFLOW

! MISSING: error: expected expression
! EXTRA: error: expected newline
! INVALID: error: unknown token in expression
! QUAD: error: SH unresolved eight-byte relocations are not supported
! OVERFLOW: error: value evaluated as 65536 is out of range
! OVERFLOW: error: value evaluated as 4294967296 is out of range
! OVERFLOW: error: literal value out of range for directive

!--- missing.s
.uaword

!--- extra.s
.ualong 1, 2

!--- invalid.s
.uaquad %r0

!--- quad-reloc.s
.uaquad external_symbol

!--- overflow.s
.uaword 0x10000
.ualong 0x100000000
.uaquad 0x10000000000000000
