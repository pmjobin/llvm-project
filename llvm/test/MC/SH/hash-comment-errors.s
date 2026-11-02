! RUN: not llvm-mc -triple=shle -mcpu=sh4 -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s
1: # Valid comment ; invalid_is_comment
invalid_sh_assembly_mnemonic
! CHECK: error: unrecognized instruction mnemonic
nop # This is an operand, not a statement-start comment.
! CHECK: error: unexpected operand
