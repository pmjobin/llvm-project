! RUN: split-file %s %t
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/numeric.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=NUMERIC
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/backward.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=BACKWARD
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/unaligned.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNALIGNED
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/range.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %t/unresolved.s -o %t/unresolved.o
! RUN: llvm-readobj --relocations --hex-dump=.text %t/unresolved.o | FileCheck %s --check-prefixes=UNRESOLVED,UNRESOLVED-BE
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %t/unresolved.s -o %t/unresolved-le.o
! RUN: llvm-readobj --relocations --hex-dump=.text %t/unresolved-le.o | FileCheck %s --check-prefixes=UNRESOLVED,UNRESOLVED-LE
! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %t/unresolved-word.s -o %t/unresolved-word.o
! RUN: llvm-readobj --relocations --hex-dump=.text %t/unresolved-word.o | FileCheck %s --check-prefixes=UNRESOLVED-WORD,UNRESOLVED-WORD-BE
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %t/unresolved-word.s -o %t/unresolved-word-le.o
! RUN: llvm-readobj --relocations --hex-dump=.text %t/unresolved-word-le.o | FileCheck %s --check-prefixes=UNRESOLVED-WORD,UNRESOLVED-WORD-LE
! RUN: not llvm-mc -triple=sh-unknown-elf -filetype=obj %t/syntax.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SYNTAX

! NUMERIC: error: SH PC-relative literal displacement must be a multiple of 4 in the range [0, 1020]
! NUMERIC: error: SH PC-relative literal displacement must be a multiple of 4 in the range [0, 1020]
! NUMERIC: error: SH PC-relative literal displacement must be a multiple of 4 in the range [0, 1020]
! BACKWARD: error: SH PC-relative literal target is behind the instruction
! UNALIGNED: error: SH PC-relative literal target must be four-byte aligned
! RANGE: error: SH PC-relative literal target is out of range
! UNRESOLVED: 0x0 R_SH_DIR8WPL external_symbol 0x0
! UNRESOLVED-BE: 0x00000000 d400
! UNRESOLVED-LE: 0x00000000 00d4
! UNRESOLVED-WORD: 0x0 R_SH_DIR8WPZ external_symbol 0x0
! UNRESOLVED-WORD-BE: 0x00000000 9400
! UNRESOLVED-WORD-LE: 0x00000000 0094
! SYNTAX: error: expected comma in longword memory operand
! SYNTAX: error: expected comma in longword memory operand
! SYNTAX: error: invalid register name

!--- numeric.s
mov.l @(-4,pc),r4
mov.l @(2,pc),r4
mov.l @(1024,pc),r4

!--- backward.s
.Lbehind:
.long 0
mov.l .Lbehind,r4

!--- unaligned.s
mov.l .Lunaligned,r4
nop
nop
.Lunaligned:
.short 0

!--- range.s
mov.l .Lfar,r4
.space 1026
.Lfar:
.long 0

!--- unresolved.s
mov.l external_symbol,r4

!--- syntax.s
mov.l @(12),r4
mov.l @(12 pc),r4
mov.l @(12,pcr),r4

!--- unresolved-word.s
mov.w external_symbol,r4
