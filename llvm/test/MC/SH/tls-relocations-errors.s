! RUN: split-file %s %t
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/addends.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDENDS
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/width.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=WIDTH
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/subtraction.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SUBTRACTION
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/non-tls.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=NON-TLS
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/tlsdesc.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=TLSDESC
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/unknown.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNKNOWN

! ADDENDS-COUNT-3: error: SH @TLSGD, @TLSLDM, and @GOTTPOFF do not support addends
! WIDTH: error: SH TLS modifiers require a four-byte value
! SUBTRACTION: error: SH TLS modifiers do not support symbol subtraction
! NON-TLS: error: SH TLS modifier requires an STT_TLS symbol
! TLSDESC: error: invalid variant
! UNKNOWN: error: invalid variant

!--- addends.s
.type tls,@tls_object
.long tls@TLSGD+4
.long tls@TLSLDM-4
.long tls@GOTTPOFF+8

!--- width.s
.type tls,@tls_object
.short tls@TPOFF

!--- subtraction.s
.type tls,@tls_object
.type other,@tls_object
.long tls@DTPOFF-other

!--- non-tls.s
.type ordinary,@object
.long ordinary@TLSGD

!--- tlsdesc.s
.type tls,@tls_object
.long tls@TLSDESC

!--- unknown.s
.type tls,@tls_object
.long tls@TLSLDO
