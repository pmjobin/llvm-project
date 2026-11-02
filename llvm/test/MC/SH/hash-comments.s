! RUN: llvm-mc -triple=shle -mcpu=sh4 -filetype=obj %s -o %t
! RUN: llvm-objdump -s -j .text %t | FileCheck %s

# A comment with no instructions.
  # An indented comment ; invalid_is_comment
1: 2: # Comment after labels ; invalid_is_comment
add #1,r0
mov #7,r0 ! Ordinary SH comment.
nop; # The rest of this line is a comment ; invalid_is_comment
.ascii "# ! ;"

! CHECK: Contents of section .text:
! CHECK-NEXT: 0000 017007e0 09002320 21203b
