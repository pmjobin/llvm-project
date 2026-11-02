! RUN: split-file %s %t
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4 -mattr=+st40-300 -show-encoding %t/all.s | FileCheck %s --check-prefix=BE
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=+st40-300 -show-encoding %t/all.s | FileCheck %s --check-prefix=LE
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4 -mattr=+st40-300 -filetype=obj %t/all.s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=+st40-300 -filetype=obj %t/all.s -o %t.le.o
! RUN: llvm-readobj --file-headers --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=FULL-FLAGS,BE-OBJECT
! RUN: llvm-readobj --file-headers --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=FULL-FLAGS,LE-OBJECT
! RUN: llvm-objdump -d --mcpu=sh4 --mattr=+st40-300 %t.be.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -d --mcpu=sh4 --mattr=+st40-300 %t.le.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -d --mcpu=sh4 %t.be.o | FileCheck %s --check-prefix=DIS-GENERIC
! RUN: llvm-objdump -d --mcpu=sh4 %t.le.o | FileCheck %s --check-prefix=DIS-GENERIC
! RUN: llvm-objdump -d --mcpu=sh4 --mattr=+st40-300,-single-fpu,-double-fpu %t.le.o | FileCheck %s --check-prefix=DIS-NOFPU
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=+st40-300,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode -filetype=obj %t/integer.s -o %t.nofpu.o
! RUN: llvm-readobj --file-headers %t.nofpu.o | FileCheck %s --check-prefix=NOFPU-FLAGS
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4 %t/all.s 2>&1 | FileCheck %s --check-prefix=ALL-UNAVAILABLE
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=+st40-300,-single-fpu,-fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,-fpu-single-mode %t/fpu.s 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4 -mattr=+st40-300,+single-fpu,+fsqrt,-fpu-size-mode,-double-fpu,-fpu-register-bank,-fpu-vector,+fpu-single-mode %t/fpu.s 2>&1 | FileCheck %s --check-prefix=UNAVAILABLE

! BE: icbi	@r1
! BE-SAME: encoding: [0x01,0xe3]
! BE: synco
! BE-SAME: encoding: [0x00,0xab]
! BE: fpchg
! BE-SAME: encoding: [0xf7,0xfd]
! LE: icbi	@r1
! LE-SAME: encoding: [0xe3,0x01]
! LE: synco
! LE-SAME: encoding: [0xab,0x00]
! LE: fpchg
! LE-SAME: encoding: [0xfd,0xf7]

! FULL-FLAGS: Flags [ (0xC)
! NOFPU-FLAGS: Flags [ (0x11)
! BE-OBJECT: Hex dump of section '.text':
! BE-OBJECT-NEXT: 0x00000000 01e300ab f7fd
! LE-OBJECT: Hex dump of section '.text':
! LE-OBJECT-NEXT: 0x00000000 e301ab00 fdf7

! DIS: icbi	@r1
! DIS-NEXT: synco
! DIS-NEXT: fpchg
! DIS-GENERIC-COUNT-3: <unknown>
! DIS-NOFPU: icbi	@r1
! DIS-NOFPU-NEXT: synco
! DIS-NOFPU-NEXT: <unknown>
! UNAVAILABLE: error: instruction requires an unavailable feature
! ALL-UNAVAILABLE: error: instruction requires an unavailable feature
! ALL-UNAVAILABLE: error: instruction requires an unavailable feature
! ALL-UNAVAILABLE: error: instruction requires an unavailable feature

!--- all.s
	icbi	@r1
	synco
	fpchg

!--- integer.s
	icbi	@r1
	synco

!--- fpu.s
	fpchg
