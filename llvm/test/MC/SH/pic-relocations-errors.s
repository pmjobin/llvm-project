! RUN: split-file %s %t
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/got-addend.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=GOT-ADDEND
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/width.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=WIDTH
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/modifier.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=MODIFIER
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/subtraction.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SUBTRACTION
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/tls.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=TLS

! GOT-ADDEND: error: SH @GOT does not support an addend
! WIDTH: error: SH GOT and PLT modifiers require a four-byte value
! MODIFIER: error:
! SUBTRACTION: error: SH GOT and PLT modifiers do not support symbol subtraction
! TLS: error: invalid variant

!--- got-addend.s
.long external@GOT+4

!--- width.s
.short external@GOTOFF

!--- modifier.s
.long external@GOT@PLT

!--- subtraction.s
.long external@GOTOFF-other

!--- tls.s
.long external@TLSGD
