! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh2a -filetype=obj -o %t.full.be.o %s
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh2a -filetype=obj -o %t.full.le.o %s
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh2a -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -filetype=obj -o %t.nofpu.o %s
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh2a -mattr=+single-fpu,+fsqrt,+fpu-size-mode,+double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode -filetype=obj -o %t.single.o %s
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh2a -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode -filetype=obj -o %t.single-only.o %s
! RUN: llvm-readobj --file-headers --hex-dump=.text %t.full.be.o | FileCheck %s --check-prefixes=COMMON,FULL,BE
! RUN: llvm-readobj --file-headers --hex-dump=.text %t.full.le.o | FileCheck %s --check-prefixes=COMMON,FULL,LE
! RUN: llvm-readobj --file-headers %t.nofpu.o | FileCheck %s --check-prefixes=COMMON,NOFPU
! RUN: llvm-readobj --file-headers %t.single.o | FileCheck %s --check-prefixes=COMMON,FULL
! RUN: llvm-readobj --file-headers %t.single-only.o | FileCheck %s --check-prefixes=COMMON,FULL

! COMMON: Class: 32-bit (0x1)
! BE: DataEncoding: BigEndian (0x2)
! LE: DataEncoding: LittleEndian (0x1)
! COMMON: Machine: EM_SH (0x2A)
! FULL: Flags [ (0xD)
! NOFPU: Flags [ (0x13)
! BE: Hex dump of section '.text':
! BE-NEXT: 0x00000000 04000004 0009
! LE: Hex dump of section '.text':
! LE-NEXT: 0x00000000 00040400 0900

movi20 #4,r4
nop
