! RUN: split-file %s %t
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh2e -filetype=obj -o %t.sh2e.be.o %t/e-common.s
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh2e -filetype=obj -o %t.sh2e.le.o %t/e-common.s
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh3e -filetype=obj -o %t.sh3e.be.o %t/e-common.s
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh3e -filetype=obj -o %t.sh3e.le.o %t/e-common.s
! RUN: llvm-readobj --file-headers %t.sh2e.be.o %t.sh2e.le.o | FileCheck %s --check-prefix=SH2E
! RUN: llvm-readobj --file-headers %t.sh3e.be.o %t.sh3e.le.o | FileCheck %s --check-prefix=SH3E
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh2e -filetype=null %t/fsqrt.s 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh3e -filetype=null %t/fsqrt.s
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh2 -filetype=null %t/e-common.s 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh3 -filetype=null %t/e-common.s 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE

! SH2E-COUNT-2: Flags [ (0xB)
! SH3E-COUNT-2: Flags [ (0x8)
! UNAVAILABLE: error: instruction requires an unavailable feature

!--- e-common.s
	fadd	fr2,fr3
	float	fpul,fr4
	ftrc	fr5,fpul
	nop

!--- fsqrt.s
	fsqrt	fr0
