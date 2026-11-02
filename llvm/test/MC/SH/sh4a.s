! RUN: split-file %s %t
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4a -show-encoding %t/all.s | FileCheck %s --check-prefix=BE
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4a -show-encoding %t/all.s | FileCheck %s --check-prefix=LE
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4a -filetype=obj %t/all.s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4a -filetype=obj %t/all.s -o %t.le.o
! RUN: llvm-readobj --file-headers --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=FULL-FLAGS,BE-OBJECT
! RUN: llvm-readobj --file-headers --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=FULL-FLAGS,LE-OBJECT
! RUN: llvm-objdump -d --mcpu=sh4a %t.be.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -d --mcpu=sh4a %t.le.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4a -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -filetype=obj %t/integer.s -o %t.nofpu.o
! RUN: llvm-readobj --file-headers %t.nofpu.o | FileCheck %s --check-prefix=NOFPU-FLAGS
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4a -mattr=+single-fpu,+fsqrt,+fpu-size-mode,+double-fpu,+fpu-register-bank,+fpu-vector,+fpu-single-mode -filetype=obj %t/all.s -o %t.single.o
! RUN: llvm-readobj --file-headers %t.single.o | FileCheck %s --check-prefix=FULL-FLAGS
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4a -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode -filetype=obj %t/single-only.s -o %t.single-only.o
! RUN: llvm-readobj --file-headers %t.single-only.o | FileCheck %s --check-prefix=FULL-FLAGS
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4a -mattr=-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode %t/fpu.s 2>&1 | FileCheck %s --check-prefix=FPU-UNAVAILABLE
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4a -mattr=+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode %t/fpchg.s 2>&1 | FileCheck %s --check-prefix=FPU-UNAVAILABLE
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4 %t/private.s 2>&1 | FileCheck %s --check-prefix=PRIVATE-UNAVAILABLE
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh2a %t/private.s 2>&1 | FileCheck %s --check-prefix=PRIVATE-UNAVAILABLE
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=+st40-300 %t/private.s 2>&1 | FileCheck %s --check-prefix=PRIVATE-UNAVAILABLE
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=+st40-300 -filetype=null %t/common.s

! BE: movco.l	r0,@r1
! BE-SAME: encoding: [0x01,0x73]
! BE: movli.l	@r2,r0
! BE-SAME: encoding: [0x02,0x63]
! BE: movua.l	@r3,r0
! BE-SAME: encoding: [0x43,0xa9]
! BE: movua.l	@r4+,r0
! BE-SAME: encoding: [0x44,0xe9]
! BE: icbi	@r5
! BE-SAME: encoding: [0x05,0xe3]
! BE: ldc	r6,sgr
! BE-SAME: encoding: [0x46,0x3a]
! BE: ldc.l	@r7+,sgr
! BE-SAME: encoding: [0x47,0x36]
! BE: prefi	@r8
! BE-SAME: encoding: [0x08,0xd3]
! BE: synco
! BE-SAME: encoding: [0x00,0xab]
! BE: fsrra	fr9
! BE-SAME: encoding: [0xf9,0x7d]
! BE: fsca	fpul,dr10
! BE-SAME: encoding: [0xfa,0xfd]
! BE: fpchg
! BE-SAME: encoding: [0xf7,0xfd]

! LE: encoding: [0x73,0x01]
! LE: encoding: [0x63,0x02]
! LE: encoding: [0xa9,0x43]
! LE: encoding: [0xe9,0x44]
! LE: encoding: [0xe3,0x05]
! LE: encoding: [0x3a,0x46]
! LE: encoding: [0x36,0x47]
! LE: encoding: [0xd3,0x08]
! LE: encoding: [0xab,0x00]
! LE: encoding: [0x7d,0xf9]
! LE: encoding: [0xfd,0xfa]
! LE: encoding: [0xfd,0xf7]

! FULL-FLAGS: Flags [ (0xC)
! NOFPU-FLAGS: Flags [ (0x11)
! BE-OBJECT: Hex dump of section '.text':
! BE-OBJECT-NEXT: 0x00000000 01730263 43a944e9 05e3463a 473608d3
! BE-OBJECT-NEXT: 0x00000010 00abf97d fafdf7fd
! LE-OBJECT: Hex dump of section '.text':
! LE-OBJECT-NEXT: 0x00000000 73016302 a943e944 e3053a46 3647d308
! LE-OBJECT-NEXT: 0x00000010 ab007df9 fdfafdf7

! DIS: movco.l	r0,@r1
! DIS-NEXT: movli.l	@r2,r0
! DIS-NEXT: movua.l	@r3,r0
! DIS-NEXT: movua.l	@r4+,r0
! DIS-NEXT: icbi	@r5
! DIS-NEXT: ldc	r6,sgr
! DIS-NEXT: ldc.l	@r7+,sgr
! DIS-NEXT: prefi	@r8
! DIS-NEXT: synco
! DIS-NEXT: fsrra	fr9
! DIS-NEXT: fsca	fpul,dr10
! DIS-NEXT: fpchg
! FPU-UNAVAILABLE: error: instruction requires an unavailable feature
! PRIVATE-UNAVAILABLE-COUNT-6: error: instruction requires an unavailable feature

!--- integer.s
	movco.l	r0,@r1
	movli.l	@r2,r0
	movua.l	@r3,r0
	movua.l	@r4+,r0
	icbi	@r5
	ldc	r6,sgr
	ldc.l	@r7+,sgr
	prefi	@r8
	synco

!--- fpu.s
	fsrra	fr9
	fsca	fpul,dr10
	fpchg

!--- fpchg.s
	fpchg

!--- single-only.s
	fsrra	fr9
	fsca	fpul,dr10

!--- private.s
	movco.l	r0,@r1
	movli.l	@r2,r0
	movua.l	@r3,r0
	movua.l	@r4+,r0
	ldc	r6,sgr
	prefi	@r8

!--- common.s
	icbi	@r5
	synco
	fpchg

!--- all.s
	movco.l	r0,@r1
	movli.l	@r2,r0
	movua.l	@r3,r0
	movua.l	@r4+,r0
	icbi	@r5
	ldc	r6,sgr
	ldc.l	@r7+,sgr
	prefi	@r8
	synco
	fsrra	fr9
	fsca	fpul,dr10
	fpchg
