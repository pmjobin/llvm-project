! RUN: llvm-mc -triple=sh-unknown-elf -filetype=obj %s -o %t-be.o
! RUN: llvm-nm -S %t-be.o | FileCheck %s
! RUN: llvm-mc -triple=shle-unknown-elf -filetype=obj %s -o %t-le.o
! RUN: llvm-nm -S %t-le.o | FileCheck %s

! These expressions also appear in SHInlineAsmSizeTest. A GNU assembler
! differential check can use the same source and symbol-size oracle.
! CHECK-DAG: 0000000a {{[tT]}} shift_add
! CHECK-DAG: 00000011 {{[tT]}} add_shift
! CHECK-DAG: 00000002 {{[tT]}} comparison_truth
! CHECK-DAG: 00000001 {{[tT]}} negative_comparison
! CHECK-DAG: 00000000 {{[tT]}} equal_bitwise_precedence
.text
shift_add:
.space 16 >> 1 + 2
.size shift_add, .-shift_add
add_shift:
.space 1 + 2 << 3
.size add_shift, .-add_shift
comparison_truth:
.if (1 == 1) < 0
.byte 0,0
.endif
.size comparison_truth, .-comparison_truth
negative_comparison:
.space -(3 > 2)
.size negative_comparison, .-negative_comparison
equal_bitwise_precedence:
.space 1 | 2 & 4
.size equal_bitwise_precedence, .-equal_bitwise_precedence
