! RUN: split-file %s %t
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4 -mattr=-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -show-encoding %t/common.s | FileCheck %s --check-prefix=BE
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -show-encoding %t/common.s | FileCheck %s --check-prefix=LE
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4 -mattr=-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -filetype=obj %t/common.s -o %t.340.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -filetype=obj %t/common.s -o %t.400.le.o
! RUN: llvm-readobj --file-headers --hex-dump=.text %t.340.be.o | FileCheck %s --check-prefixes=NOMMU-FLAGS,BE-OBJECT
! RUN: llvm-readobj --file-headers --hex-dump=.text %t.400.le.o | FileCheck %s --check-prefixes=NOMMU-FLAGS,LE-OBJECT
! RUN: llvm-objdump -d --mcpu=sh4 --mattr=-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode %t.340.be.o | FileCheck %s --check-prefix=DIS-COMMON
! RUN: llvm-objdump -d --mcpu=sh4 --mattr=-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode %t.400.le.o | FileCheck %s --check-prefix=DIS-COMMON
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4 -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -show-encoding %t/mmu.s | FileCheck %s --check-prefix=MMU-BE
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -filetype=obj %t/mmu.s -o %t.500.le.o
! RUN: llvm-readobj --file-headers --hex-dump=.text %t.500.le.o | FileCheck %s --check-prefixes=MMU-FLAGS,MMU-LE-OBJECT
! RUN: llvm-objdump -d --mcpu=sh4 --mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode %t.500.le.o | FileCheck %s --check-prefix=DIS-MMU
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode %t/mmu.s 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode %t/st40-300.s 2>&1 | FileCheck %s --check-prefix=THREE-UNAVAILABLE
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode %t/st40-300.s 2>&1 | FileCheck %s --check-prefix=THREE-UNAVAILABLE
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=-mmu,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode %t/fpu.s 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode %t/fpu.s 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE

! BE: movca.l	r0,@r1
! BE-SAME: encoding: [0x01,0xc3]
! BE: ocbi	@r2
! BE-SAME: encoding: [0x02,0x93]
! BE: ocbp	@r3
! BE-SAME: encoding: [0x03,0xa3]
! BE: ocbwb	@r4
! BE-SAME: encoding: [0x04,0xb3]
! BE: pref	@r5
! BE-SAME: encoding: [0x05,0x83]
! LE: movca.l	r0,@r1
! LE-SAME: encoding: [0xc3,0x01]
! LE: ocbi	@r2
! LE-SAME: encoding: [0x93,0x02]
! LE: ocbp	@r3
! LE-SAME: encoding: [0xa3,0x03]
! LE: ocbwb	@r4
! LE-SAME: encoding: [0xb3,0x04]
! LE: pref	@r5
! LE-SAME: encoding: [0x83,0x05]

! NOMMU-FLAGS: Flags [ (0x12)
! MMU-FLAGS: Flags [ (0x10)
! BE-OBJECT: Hex dump of section '.text':
! BE-OBJECT-NEXT: 0x00000000 01c30293 03a304b3 0583
! LE-OBJECT: Hex dump of section '.text':
! LE-OBJECT-NEXT: 0x00000000 c3019302 a303b304 8305
! MMU-BE: ldtlb
! MMU-BE-SAME: encoding: [0x00,0x38]
! MMU-LE-OBJECT: Hex dump of section '.text':
! MMU-LE-OBJECT-NEXT: 0x00000000 3800

! DIS-COMMON: movca.l	r0,@r1
! DIS-COMMON-NEXT: ocbi	@r2
! DIS-COMMON-NEXT: ocbp	@r3
! DIS-COMMON-NEXT: ocbwb	@r4
! DIS-COMMON-NEXT: pref	@r5
! DIS-MMU: ldtlb
! UNAVAILABLE: error: instruction requires an unavailable feature
! THREE-UNAVAILABLE-COUNT-3: error: instruction requires an unavailable feature

!--- common.s
	movca.l	r0,@r1
	ocbi	@r2
	ocbp	@r3
	ocbwb	@r4
	pref	@r5

!--- mmu.s
	ldtlb

!--- st40-300.s
	icbi	@r1
	synco
	fpchg

!--- fpu.s
	fadd	fr0,fr1
