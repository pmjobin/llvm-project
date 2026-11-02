! RUN: not llvm-mc -triple=sh-unknown-elf -show-encoding %s 2>&1 | FileCheck %s
! RUN: not llvm-mc -triple=shle-unknown-elf -show-encoding %s 2>&1 | FileCheck %s

! CHECK: :[[@LINE+1]]:5: error: invalid operand for instruction
jmp r4
! CHECK: :[[@LINE+1]]:8: error: expected GPR after '@'
jmp @pr
! CHECK: :[[@LINE+1]]:1: error: too few operands for instruction
jmp
! CHECK: :[[@LINE+1]]:9: error: invalid operand for instruction
jmp @r4,r5
! CHECK: :[[@LINE+1]]:6: error: expected GPR after '@'
jmp @@r4
! CHECK: :[[@LINE+1]]:6: error: expected GPR after '@'
jmp @
! CHECK: :[[@LINE+1]]:6: error: expected GPR after '@'
jmp @r16
! CHECK: :[[@LINE+1]]:8: error: expected comma
jmp @r4+
! CHECK: :[[@LINE+1]]:9: error: expected operand
jmp @r4,
! CHECK: :[[@LINE+1]]:9: error: expected comma
jmp @r4 @r5
