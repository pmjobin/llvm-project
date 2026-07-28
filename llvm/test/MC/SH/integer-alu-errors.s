! RUN: not llvm-mc -triple=sh-unknown-elf -show-encoding %s 2>&1 | FileCheck %s

sub r5 r4
! CHECK: :[[@LINE-1]]:8: error: expected comma

neg r5 r4
! CHECK: :[[@LINE-1]]:8: error: expected comma

and r5
! CHECK: :[[@LINE-1]]:1: error: too few operands for instruction

or r5,r4,r3
! CHECK: :[[@LINE-1]]:10: error: invalid operand for instruction

xor #1,r4
! CHECK: :[[@LINE-1]]:5: error: unexpected operand

not r5
! CHECK: :[[@LINE-1]]:1: error: too few operands for instruction

tst r5,r4,r3
! CHECK: :[[@LINE-1]]:11: error: invalid operand for instruction

dt
! CHECK: :[[@LINE-1]]:1: error: too few operands for instruction

shll r4,r5
! CHECK: :[[@LINE-1]]:9: error: invalid operand for instruction

shlr #1
! CHECK: :[[@LINE-1]]:6: error: unexpected operand

negc r5,r4
! CHECK: :[[@LINE-1]]:1: error: unrecognized instruction mnemonic

subv r5,r4
! CHECK: :[[@LINE-1]]:1: error: unrecognized instruction mnemonic

addv r5,r4
! CHECK: :[[@LINE-1]]:1: error: unrecognized instruction mnemonic

rotl r4
! CHECK: :[[@LINE-1]]:1: error: unrecognized instruction mnemonic

rotr r4
! CHECK: :[[@LINE-1]]:1: error: unrecognized instruction mnemonic

shal r4
! CHECK: :[[@LINE-1]]:1: error: unrecognized instruction mnemonic

and #1,r0
! CHECK: :[[@LINE-1]]:5: error: unexpected operand

tst #1,r0
! CHECK: :[[@LINE-1]]:5: error: unexpected operand
