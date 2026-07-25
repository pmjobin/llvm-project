! RUN: not llvm-mc -triple=sh-unknown-elf -show-encoding %s 2>&1 | FileCheck %s

mov.l r4,r0
! CHECK: :[[@LINE-1]]:10: error: invalid operand for instruction

mov.l @4,r0
! CHECK: :[[@LINE-1]]:8: error: expected register or '(' after '@'

mov.l @(2,r4),r0
! CHECK: :[[@LINE-1]]:7: error: longword displacement must be a multiple of 4 in the range [0, 60]

mov.l @(64,r4),r0
! CHECK: :[[@LINE-1]]:7: error: longword displacement must be a multiple of 4 in the range [0, 60]

mov.l @(-4,r4),r0
! CHECK: :[[@LINE-1]]:7: error: longword displacement must be a multiple of 4 in the range [0, 60]

mov.l @(12,r16),r0
! CHECK: :[[@LINE-1]]:12: error: invalid register name

mov.l r5,(12,r4)
! CHECK: :[[@LINE-1]]:10: error: unexpected operand

mov.l @(12 r4),r0
! CHECK: :[[@LINE-1]]:12: error: expected comma in longword memory operand

mov.l @r4 r0
! CHECK: :[[@LINE-1]]:11: error: expected comma

mov.l @r4,
! CHECK: :[[@LINE-1]]:11: error: expected operand
