! RUN: split-file %s %t
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4 -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode -filetype=obj %t/identity.s -o %t.sh4-restricted-be.o
! RUN: llvm-readobj --file-headers %t.sh4-restricted-be.o | FileCheck %s --check-prefix=SH4
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4 -mattr=+single-fpu,+fsqrt,+fpu-size-mode,-double-fpu,+fpu-register-bank,+fpu-vector,+fpu-single-mode -filetype=obj %t/identity.s -o %t.sh4-canonical-be.o
! RUN: llvm-readobj --file-headers %t.sh4-canonical-be.o | FileCheck %s --check-prefix=SH4
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode -filetype=obj %t/identity.s -o %t.sh4-restricted-le.o
! RUN: llvm-readobj --file-headers %t.sh4-restricted-le.o | FileCheck %s --check-prefix=SH4
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=+single-fpu,+fsqrt,+fpu-size-mode,-double-fpu,+fpu-register-bank,+fpu-vector,+fpu-single-mode -filetype=obj %t/identity.s -o %t.sh4-canonical-le.o
! RUN: llvm-readobj --file-headers %t.sh4-canonical-le.o | FileCheck %s --check-prefix=SH4
! RUN: not llvm-mc -triple=shle -mcpu=sh4 -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode,+fpu-size-mode -filetype=obj %t/identity.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=IDENTITY
! RUN: not llvm-mc -triple=shle -mcpu=sh4 -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode,-fsqrt -filetype=obj %t/identity.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=IDENTITY
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4a -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode -filetype=obj %t/identity.s -o %t.sh4a-restricted-be.o
! RUN: llvm-readobj --file-headers %t.sh4a-restricted-be.o | FileCheck %s --check-prefix=SH4A
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4a -mattr=+single-fpu,+fsqrt,+fpu-size-mode,-double-fpu,+fpu-register-bank,+fpu-vector,+fpu-single-mode -filetype=obj %t/identity.s -o %t.sh4a-canonical-be.o
! RUN: llvm-readobj --file-headers %t.sh4a-canonical-be.o | FileCheck %s --check-prefix=SH4A
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4a -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode -filetype=obj %t/identity.s -o %t.sh4a-restricted-le.o
! RUN: llvm-readobj --file-headers %t.sh4a-restricted-le.o | FileCheck %s --check-prefix=SH4A
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4a -mattr=+single-fpu,+fsqrt,+fpu-size-mode,-double-fpu,+fpu-register-bank,+fpu-vector,+fpu-single-mode -filetype=obj %t/identity.s -o %t.sh4a-canonical-le.o
! RUN: llvm-readobj --file-headers %t.sh4a-canonical-le.o | FileCheck %s --check-prefix=SH4A
! RUN: not llvm-mc -triple=shle -mcpu=sh4a -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode,+fpu-size-mode -filetype=obj %t/identity.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=IDENTITY
! RUN: not llvm-mc -triple=shle -mcpu=sh4a -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode,-fsqrt -filetype=obj %t/identity.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=IDENTITY
! RUN: llvm-mc -triple=shle -mcpu=sh4 -mattr=+st40-300,+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode -filetype=obj %t/identity.s -o %t.st40.o
! RUN: llvm-readobj --file-headers %t.st40.o | FileCheck %s --check-prefix=SH4A
! RUN: not llvm-mc -triple=shle -mcpu=sh4 -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode,-mmu -filetype=obj %t/identity.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=IDENTITY
! RUN: not llvm-mc -triple=shle -mcpu=sh4 -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode %t/vector.s 2>&1 | FileCheck %s --check-prefix=FEATURE

! Explicit restrictions preserve object identity; they do not enable graphics.
! Partial profiles and unrelated architecture changes remain unsupported.
! SH4: Flags [ (0x9)
! SH4A: Flags [ (0xC)
! IDENTITY: error: SH feature combination has no supported ELF machine identity
! FEATURE: error: instruction requires an unavailable feature

!--- identity.s
nop

!--- vector.s
fipr fv0,fv4
