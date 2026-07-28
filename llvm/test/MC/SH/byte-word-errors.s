! RUN: not llvm-mc -triple=sh-unknown-elf -show-encoding %s 2>&1 | FileCheck %s

mov.b r0,@(-1,r4)
! CHECK: :[[@LINE-1]]:10: error: byte displacement must be in the range [0, 15]

mov.b r0,@(16,r4)
! CHECK: :[[@LINE-1]]:10: error: byte displacement must be in the range [0, 15]

mov.b @(-1,r4),r0
! CHECK: :[[@LINE-1]]:7: error: byte displacement must be in the range [0, 15]

mov.b @(16,r4),r0
! CHECK: :[[@LINE-1]]:7: error: byte displacement must be in the range [0, 15]

mov.w r0,@(-2,r4)
! CHECK: :[[@LINE-1]]:10: error: word displacement must be an even byte offset in the range [0, 30]

mov.w r0,@(3,r4)
! CHECK: :[[@LINE-1]]:10: error: word displacement must be an even byte offset in the range [0, 30]

mov.w r0,@(32,r4)
! CHECK: :[[@LINE-1]]:10: error: word displacement must be an even byte offset in the range [0, 30]

mov.w @(-2,r4),r0
! CHECK: :[[@LINE-1]]:7: error: word displacement must be an even byte offset in the range [0, 30]

mov.w @(3,r4),r0
! CHECK: :[[@LINE-1]]:7: error: word displacement must be an even byte offset in the range [0, 30]

mov.w @(32,r4),r0
! CHECK: :[[@LINE-1]]:7: error: word displacement must be an even byte offset in the range [0, 30]

mov.b r1,@(3,r4)
! CHECK: :[[@LINE-1]]:7: error: operand must be r0

mov.w r1,@(6,r4)
! CHECK: :[[@LINE-1]]:7: error: operand must be r0

mov.b @(3,r4),r1
! CHECK: :[[@LINE-1]]:15: error: operand must be r0

mov.w @(6,r4),r1
! CHECK: :[[@LINE-1]]:15: error: operand must be r0

mov.b r5,r4
! CHECK: :[[@LINE-1]]:10: error: invalid operand for instruction

mov.w @r5 r4
! CHECK: :[[@LINE-1]]:11: error: expected comma

mov.b @(3 r4),r0
! CHECK: :[[@LINE-1]]:11: error: expected comma in byte/word memory operand

mov.w @(6,r4,r0
! CHECK: :[[@LINE-1]]:13: error: expected ')' in byte/word memory operand

mov.b (3,r4),r0
! CHECK: :[[@LINE-1]]:7: error: unexpected operand

mov.w @(6,r4) r0
! CHECK: :[[@LINE-1]]:15: error: expected comma
