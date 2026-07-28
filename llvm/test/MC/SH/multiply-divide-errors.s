! RUN: not llvm-mc -triple=sh-unknown-elf -show-encoding %s 2>&1 | FileCheck %s

mul.l r5 r4
! CHECK: :[[@LINE-1]]:10: error: expected comma

mul.l r5
! CHECK: :[[@LINE-1]]:1: error: too few operands for instruction

mul.l r5,r4,r3
! CHECK: :[[@LINE-1]]:13: error: invalid operand for instruction

sts macl r4
! CHECK: :[[@LINE-1]]:10: error: expected comma

sts macl
! CHECK: :[[@LINE-1]]:1: error: too few operands for instruction

sts r5,r4
! CHECK: :[[@LINE-1]]:5: error: invalid operand for instruction

sts mach,r4
! CHECK: :[[@LINE-1]]:5: error: invalid operand for instruction

sts macl,#1
! CHECK: :[[@LINE-1]]:10: error: unexpected operand

div0u r4
! CHECK: :[[@LINE-1]]:7: error: invalid operand for instruction

div0s r5 r4
! CHECK: :[[@LINE-1]]:10: error: expected comma

div0s r5
! CHECK: :[[@LINE-1]]:1: error: too few operands for instruction

div1 r5 r4
! CHECK: :[[@LINE-1]]:9: error: expected comma

div1 r5,r4,r3
! CHECK: :[[@LINE-1]]:12: error: invalid operand for instruction

rotcl
! CHECK: :[[@LINE-1]]:1: error: too few operands for instruction

rotcl r4,r5
! CHECK: :[[@LINE-1]]:10: error: invalid operand for instruction

addc r5 r4
! CHECK: :[[@LINE-1]]:9: error: expected comma

subc r5,r4,r3
! CHECK: :[[@LINE-1]]:12: error: invalid operand for instruction

div1 mbit,r4
! CHECK: :[[@LINE-1]]:6: error: invalid register name

div1 qbit,r4
! CHECK: :[[@LINE-1]]:6: error: invalid register name

dmuls.l r5,r4
! CHECK: :[[@LINE-1]]:1: error: unrecognized instruction mnemonic

dmulu.l r5,r4
! CHECK: :[[@LINE-1]]:1: error: unrecognized instruction mnemonic

muls.w r5,r4
! CHECK: :[[@LINE-1]]:1: error: unrecognized instruction mnemonic

mulu.w r5,r4
! CHECK: :[[@LINE-1]]:1: error: unrecognized instruction mnemonic

mac.l @r5+,@r4+
! CHECK: :[[@LINE-1]]:10: error: expected comma

mac.w @r5+,@r4+
! CHECK: :[[@LINE-1]]:10: error: expected comma

rotcr r4
! CHECK: :[[@LINE-1]]:1: error: unrecognized instruction mnemonic

negc r5,r4
! CHECK: :[[@LINE-1]]:1: error: unrecognized instruction mnemonic
