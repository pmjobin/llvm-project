# RUN: llvm-mc -triple=sh-unknown-elf -show-encoding %s | FileCheck %s --check-prefix=BASE
# RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh1 -show-encoding %s | FileCheck %s --check-prefix=BASE
# RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh2 -show-encoding %s | FileCheck %s --check-prefix=BASE
# RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh3 -show-encoding %s | FileCheck %s --check-prefix=BASE
# RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4 -show-encoding %s | FileCheck %s --check-prefix=BASE
# RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh2 -mattr=-sh2 -show-encoding %s | FileCheck %s --check-prefix=BASE
# RUN: echo 'dt r3' | not llvm-mc -triple=sh-unknown-elf -mcpu=sh1 2>&1 | FileCheck %s --check-prefix=NO-SH2
# RUN: echo 'shad r2,r3' | llvm-mc -triple=sh-unknown-elf -mcpu=sh2 -mattr=+dynamic-shift -show-encoding | FileCheck %s --check-prefix=DYNAMIC
# RUN: echo 'clrs' | not llvm-mc -triple=sh-unknown-elf -mcpu=sh3 -mattr=-s-bit-control 2>&1 | FileCheck %s --check-prefix=NO-SBIT
# RUN: echo 'stc dbr,r3' | llvm-mc -triple=sh-unknown-elf -mcpu=sh3 -mattr=+sh4-debug-state -show-encoding | FileCheck %s --check-prefix=DEBUG
# RUN: echo 'ocbi @r3' | llvm-mc -triple=sh-unknown-elf -mcpu=sh3 -mattr=+sh4-cache -show-encoding | FileCheck %s --check-prefix=CACHE
# RUN: echo 'ftrv xmtrx,fv12' | llvm-mc -triple=sh-unknown-elf -mcpu=sh2 -mattr=+fpu-vector -show-encoding | FileCheck %s --check-prefix=VECTOR
# RUN: echo 'fadd dr2,dr4' | not llvm-mc -triple=sh-unknown-elf -mcpu=sh4 -mattr=-double-fpu 2>&1 | FileCheck %s --check-prefix=NO-DOUBLE

# BASE: nop
# NO-SH2: error: instruction requires an unavailable feature
# DYNAMIC: shad	r2,r3
# DYNAMIC-SAME: encoding: [0x43,0x2c]
# NO-SBIT: error: instruction requires an unavailable feature
# DEBUG: stc	dbr,r3
# DEBUG-SAME: encoding: [0x03,0xfa]
# CACHE: ocbi	@r3
# CACHE-SAME: encoding: [0x03,0x93]
# VECTOR: ftrv	xmtrx,fv12
# VECTOR-SAME: encoding: [0xfd,0xfd]
# NO-DOUBLE: error: instruction requires an unavailable feature

nop
