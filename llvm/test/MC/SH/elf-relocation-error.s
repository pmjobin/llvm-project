! RUN: split-file %s %t
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj -o /dev/null %t/byte.s 2>&1 | FileCheck %s --check-prefix=BYTE
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj -o /dev/null %t/short.s 2>&1 | FileCheck %s --check-prefix=SHORT
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj -o /dev/null %t/quad.s 2>&1 | FileCheck %s --check-prefix=QUAD
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj -o /dev/null %t/pcrel.s 2>&1 | FileCheck %s --check-prefix=PCREL

! BYTE: error: SH unresolved one-byte relocations are not supported
! SHORT: error: SH unresolved two-byte relocations are not supported
! QUAD: error: SH unresolved eight-byte relocations are not supported
! PCREL: error: SH PC-relative data relocations are not supported

!--- byte.s
.byte external_symbol

!--- short.s
.short external_symbol

!--- quad.s
.quad external_symbol

!--- pcrel.s
.long external_symbol-.
