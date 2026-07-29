! RUN: not llvm-mc -triple=sh-unknown-elf -show-encoding %s 2>&1 | FileCheck %s

tas.b r4
! CHECK: :[[@LINE-1]]:7: error: invalid operand for instruction

tas.b @pr
! CHECK: :[[@LINE-1]]:7: error: invalid operand for instruction

tas.b @
! CHECK: :[[@LINE-1]]:8: error: expected register or '(' after '@'

tas.b @r4,r5
! CHECK: :[[@LINE-1]]:11: error: invalid operand for instruction

movt @r4
! CHECK: :[[@LINE-1]]:6: error: invalid operand for instruction

movt pr
! CHECK: :[[@LINE-1]]:6: error: invalid operand for instruction

movt
! CHECK: :[[@LINE-1]]:1: error: too few operands for instruction

movt r4,r5
! CHECK: :[[@LINE-1]]:9: error: invalid operand for instruction
