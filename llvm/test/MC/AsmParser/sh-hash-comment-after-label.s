# REQUIRES: sh-registered-target, aarch64-registered-target
# RUN: llvm-mc -triple=shle -mcpu=sh4 -show-encoding %s | FileCheck %s --check-prefix=SH
# RUN: llvm-mc -triple=aarch64 -show-encoding %s | FileCheck %s --check-prefix=OTHER

# Only SH changes: its hash comment must consume the semicolon and first nop.
# The independent AArch64 control must retain its existing two-instruction parse.
.text
label: # comment ; nop
nop

# SH-COUNT-1: encoding: [0x09,0x00]
# SH-NOT: encoding:
# OTHER-COUNT-2: encoding: [0x1f,0x20,0x03,0xd5]
# OTHER-NOT: encoding:
