! RUN: not llvm-mc -triple=sh-unknown-elf -show-encoding %s 2>&1 | FileCheck %s

clrt r4
! CHECK: :[[@LINE-1]]:6: error: invalid operand for instruction

clrt #0
! CHECK: :[[@LINE-1]]:6: error: unexpected operand

rotcr
! CHECK: :[[@LINE-1]]:1: error: too few operands for instruction

rotcr r4,r5
! CHECK: :[[@LINE-1]]:10: error: invalid operand for instruction

rotcr #1
! CHECK: :[[@LINE-1]]:7: error: unexpected operand

rotcr tbit
! CHECK: :[[@LINE-1]]:7: error: invalid register name
