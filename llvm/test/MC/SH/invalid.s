! RUN: not llvm-mc -triple=sh-unknown-elf -show-encoding %s 2>&1 | FileCheck %s

mov r16,r0
! CHECK: :[[@LINE-1]]:5: error: invalid register name

mov r0 r1
! CHECK: :[[@LINE-1]]:8: error: expected comma

mov -1,r0
! CHECK: :[[@LINE-1]]:5: error: unexpected operand

mov #-129,r0
! CHECK: :[[@LINE-1]]:5: error: immediate must be an integer in the range [-128, 127]

add #128,r0
! CHECK: :[[@LINE-1]]:5: error: immediate must be an integer in the range [-128, 127]

nop r0
! CHECK: :[[@LINE-1]]:5: error: invalid operand for instruction

mov r0
! CHECK: :[[@LINE-1]]:1: error: too few operands for instruction

mov r0,r1,r2
! CHECK: :[[@LINE-1]]:11: error: invalid operand for instruction

sleep
! CHECK: :[[@LINE-1]]:1: error: unrecognized instruction mnemonic

cmp/eq r5 r4
! CHECK: :[[@LINE-1]]:11: error: expected comma

cmp/ge r16,r4
! CHECK: :[[@LINE-1]]:8: error: invalid register name

cmp/hi r5,tbit
! CHECK: :[[@LINE-1]]:11: error: invalid register name

bt
! CHECK: :[[@LINE-1]]:1: error: too few operands for instruction

bra #1
! CHECK: :[[@LINE-1]]:5: error: unknown token in expression

bt/s .Ltarget
! CHECK: :[[@LINE-1]]:1: error: unrecognized instruction mnemonic
