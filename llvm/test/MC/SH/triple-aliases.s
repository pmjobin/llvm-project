! RUN: llvm-mc -triple=shbe-unknown-elf -show-encoding %s | FileCheck %s --check-prefix=BE
! RUN: llvm-mc -triple=sheb-unknown-elf -show-encoding %s | FileCheck %s --check-prefix=BE
! RUN: llvm-mc -triple=shl-unknown-elf -show-encoding %s | FileCheck %s --check-prefix=LE

! BE: encoding: [0x00,0x09]
! LE: encoding: [0x09,0x00]
nop
