! RUN: split-file %s %t
! RUN: %clang -target sh-unknown-none-elf -m4-nofpu -c %t/base.s -o %t.nofpu.be.o
! RUN: %clang -target shle-unknown-none-elf -m4-nofpu -c %t/base.s -o %t.nofpu.le.o
! RUN: llvm-readobj --file-headers %t.nofpu.be.o %t.nofpu.le.o | FileCheck %s --check-prefix=NOFPU-FLAGS
! RUN: not %clang -target sh-unknown-none-elf -m4-nofpu -c %t/f32.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE
! RUN: %clang -target sh-unknown-none-elf -m4-single -c %t/f64.s -o %t.single.o
! RUN: llvm-readobj --file-headers %t.single.o | FileCheck %s --check-prefix=SH4-FLAGS
! RUN: %clang -target shle-unknown-none-elf -m4-single-only -c %t/f32.s -o %t.single-only.o
! RUN: llvm-readobj --file-headers %t.single-only.o | FileCheck %s --check-prefix=SH4-FLAGS
! RUN: not %clang -target shle-unknown-none-elf -m4-single-only -c %t/f64.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE
! RUN: %clang -target shle-unknown-none-elf -m4-single-only -c %t/bank.s -o %t.bank.le.o
! RUN: %clang -target sh-unknown-none-elf -m4-single-only -c %t/bank.s -o %t.bank.be.o
! RUN: %clang -target shle-unknown-none-elf -m4a-single-only -c %t/bank.s -o %t.banka.le.o
! RUN: %clang -target sh-unknown-none-elf -m4a-single-only -c %t/bank.s -o %t.banka.be.o
! RUN: not %clang -target shle-unknown-none-elf -m2a-single-only -c %t/bank.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE
! RUN: %clang -target sh-unknown-none-elf -m2e -c %t/f32.s -o %t.sh2e.be.o
! RUN: %clang -target shle-unknown-none-elf -m2e -c %t/f32.s -o %t.sh2e.le.o
! RUN: llvm-readobj --file-headers %t.sh2e.be.o %t.sh2e.le.o | FileCheck %s --check-prefix=SH2E-FLAGS
! RUN: %clang -target sh-unknown-none-elf -m3e -c %t/fsqrt.s -o %t.sh3e.be.o
! RUN: %clang -target shle-unknown-none-elf -m3e -c %t/fsqrt.s -o %t.sh3e.le.o
! RUN: llvm-readobj --file-headers %t.sh3e.be.o %t.sh3e.le.o | FileCheck %s --check-prefix=SH3E-FLAGS
! RUN: not %clang -target shle-unknown-none-elf -m2e -c %t/fsqrt.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE
! RUN: not %clang -target shle-unknown-none-elf -m2e -c %t/sh3.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE
! RUN: not %clang -target shle-unknown-none-elf -m3e -c %t/sh4.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE
! RUN: not %clang -target shle-unknown-none-elf -m3e -c %t/f64.s -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE

! NOFPU-FLAGS-COUNT-2: Flags [ (0x10)
! SH4-FLAGS: Flags [ (0x9)
! SH2E-FLAGS-COUNT-2: Flags [ (0xB)
! SH3E-FLAGS-COUNT-2: Flags [ (0x8)
! UNAVAILABLE: error: instruction requires an unavailable feature

!--- base.s
	nop
	pref	@r0

!--- f32.s
	fadd	fr0,fr2
	lds	r0,fpscr

!--- f64.s
	fadd	dr0,dr2

!--- bank.s
	frchg
	fschg
	fmov	dr0,dr2
	fmov.d	@r0,xd0
	fipr	fv0,fv4
	ftrv	xmtrx,fv0

!--- fsqrt.s
	fsqrt	fr0

!--- sh3.s
	shad	r0,r1

!--- sh4.s
	movca.l	r0,@r1
