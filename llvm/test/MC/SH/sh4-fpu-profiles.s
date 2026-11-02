! RUN: split-file %s %t
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4 -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -filetype=obj %t/base.s -o %t.nofpu.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -filetype=obj %t/base.s -o %t.nofpu.le.o
! RUN: llvm-readobj --file-headers %t.nofpu.be.o %t.nofpu.le.o | FileCheck %s --check-prefix=NOFPU-FLAGS
! RUN: not llvm-mc -triple=sh-unknown-elf -mcpu=sh4 -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode %t/f32.s 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4 -mattr=+fpu-single-mode -filetype=obj %t/full.s -o %t.single.o
! RUN: llvm-readobj --file-headers %t.single.o | FileCheck %s --check-prefix=SH4-FLAGS
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode -filetype=obj %t/f32.s -o %t.single-only.o
! RUN: llvm-readobj --file-headers %t.single-only.o | FileCheck %s --check-prefix=SH4-FLAGS
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode %t/f64.s 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode %t/bank.s 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE

! NOFPU-FLAGS-COUNT-2: Flags [ (0x10)
! SH4-FLAGS: Flags [ (0x9)
! UNAVAILABLE: error: instruction requires an unavailable feature

!--- base.s
	nop
	pref	@r0

!--- f32.s
	fmov.s	@r0,fr2
	fadd	fr0,fr2
	fsqrt	fr2
	ftrc	fr2,fpul
	lds	r0,fpscr

!--- full.s
	fadd	fr0,fr2
	fadd	dr0,dr2
	frchg
	fschg

!--- f64.s
	fmov	dr0,dr2
	fadd	dr0,dr2

!--- bank.s
	frchg
	fschg
