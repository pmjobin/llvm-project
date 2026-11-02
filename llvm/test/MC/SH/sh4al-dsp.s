! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4al-dsp -show-encoding < %s | FileCheck %s --check-prefixes=ASM,BE
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4al-dsp -show-encoding < %s | FileCheck %s --check-prefixes=ASM,LE
! RUN: llvm-mc -triple=sh-unknown-elf -mcpu=sh4al-dsp -filetype=obj %s -o %t.be.o
! RUN: llvm-mc -triple=shle-unknown-elf -mcpu=sh4al-dsp -filetype=obj %s -o %t.le.o
! RUN: llvm-readobj --file-headers %t.be.o %t.le.o | FileCheck %s --check-prefix=ELF
! RUN: llvm-objdump -d --mcpu=sh4al-dsp %t.be.o | FileCheck %s --check-prefix=DIS
! RUN: llvm-objdump -d --mcpu=sh4al-dsp %t.le.o | FileCheck %s --check-prefix=DIS

! ASM: clrdmxy
! BE-SAME: encoding: [0x00,0x88]
! LE-SAME: encoding: [0x88,0x00]
	clrdmxy
! ASM: ldc	r3,mod
! BE-SAME: encoding: [0x43,0x5e]
! LE-SAME: encoding: [0x5e,0x43]
	ldc r3,mod
! ASM: ldrc	#4
! BE-SAME: encoding: [0x8a,0x04]
! LE-SAME: encoding: [0x04,0x8a]
	ldrc #4
! ASM: lds	r3,a0
! BE-SAME: encoding: [0x43,0x7a]
! LE-SAME: encoding: [0x7a,0x43]
	lds r3,a0
! ASM: setdmx
! BE-SAME: encoding: [0x00,0x98]
! LE-SAME: encoding: [0x98,0x00]
	setdmx
! ASM: stc	mod,r3
! BE-SAME: encoding: [0x03,0x52]
! LE-SAME: encoding: [0x52,0x03]
	stc mod,r3
! ASM: sts	a0,r3
! BE-SAME: encoding: [0x03,0x7a]
! LE-SAME: encoding: [0x7a,0x03]
	sts a0,r3
! ASM: movx.w	@r4,x0
! BE-SAME: encoding: [0xf0,0x04]
! LE-SAME: encoding: [0x04,0xf0]
	movx.w @r4,x0 nopy
! ASM: movx.w	@r4,x0
! BE-SAME: encoding: [0xf0,0x04]
! LE-SAME: encoding: [0x04,0xf0]
	MOVX.W @R4,X0 NOPY
! ASM: movs.l	@r4+,x0
! BE-SAME: encoding: [0xf4,0x8a]
! LE-SAME: encoding: [0x8a,0xf4]
	movs.l @r4+,x0
! ASM: padd	x0,y0,a0
! BE-SAME: encoding: [0xf8,0x00,0xb1,0x07]
! LE-SAME: encoding: [0x00,0xf8,0x07,0xb1]
	padd x0,y0,a0
! ASM: dct pabs	x0,a0
! BE-SAME: encoding: [0xf8,0x00,0x8a,0x17]
! LE-SAME: encoding: [0x00,0xf8,0x17,0x8a]
	dct pabs x0,a0
! ASM: padd	a0,m0,a0	pmuls	x0,y0,m0
! BE-SAME: encoding: [0xf8,0x00,0x70,0xa2]
! LE-SAME: encoding: [0x00,0xf8,0xa2,0x70]
	padd a0,m0,a0 pmuls x0,y0,m0
! ASM: pwsb	x0,y0,a0
! BE-SAME: encoding: [0xf8,0x00,0xa4,0x07]
! LE-SAME: encoding: [0x00,0xf8,0x07,0xa4]
	pwsb x0,y0,a0
! ASM: pwad	x0,y0,a0
! BE-SAME: encoding: [0xf8,0x00,0xb4,0x07]
! LE-SAME: encoding: [0x00,0xf8,0x07,0xb4]
	pwad x0,y0,a0

! ELF-COUNT-2: Flags [ (0x6)

! DIS: clrdmxy
! DIS-NEXT: ldc	r3,mod
! DIS-NEXT: ldrc	#4
! DIS-NEXT: lds	r3,a0
! DIS-NEXT: setdmx
! DIS-NEXT: stc	mod,r3
! DIS-NEXT: sts	a0,r3
! DIS-NEXT: movx.w	@r4,x0
! DIS-NEXT: movx.w	@r4,x0
! DIS-NEXT: movs.l	@r4+,x0
! DIS-NEXT: padd	x0,y0,a0
! DIS-NEXT: dct pabs	x0,a0
! DIS-NEXT: padd	a0,m0,a0	pmuls	x0,y0,m0
! DIS-NEXT: pwsb	x0,y0,a0
! DIS-NEXT: pwad	x0,y0,a0
