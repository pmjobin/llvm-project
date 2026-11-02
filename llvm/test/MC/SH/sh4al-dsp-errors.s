! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4al-dsp -filetype=null %s 2>&1 | FileCheck %s

! CHECK: error: movx cannot be combined with non-nopy
	movy.w @r2,y0 movx.w @r0,x0
! CHECK: error: movy requires nopx
	movx.w @r0,x0 movy.w @r6,y0
! CHECK: error: multiple parallel-processing operations
	padd x0,y0,a0 psub x0,y0,a1
! CHECK: error: invalid operands for DSP operation
	pshl #64,a0
! CHECK: error: invalid operands for DSP operation
	psha #-65,a0
! CHECK: error: pmuls cannot be combined with this DSP operation
	pand x0,y0,a0 pmuls x0,y0,m0
! CHECK: error: condition is not followed by a conditional DSP operation
	dct nopx nopy
! CHECK: error: movs cannot be combined with a parallel instruction
	movs.w @r4,x0 pabs x0,a0
! CHECK: error: invalid operands for DSP single-data transfer
	movs.w @r8,x0
! CHECK: error: repeat count must be an integer in the range [0, 255]
	repeat 10 20 #256

! CHECK: error: unrecognized instruction mnemonic
	dsp.ddt #0
! CHECK: error: unrecognized instruction mnemonic
	dsp.sdt #0
! CHECK: error: unrecognized instruction mnemonic
	dsp.ppi #0

! CHECK: error: invalid modulo index register for DSP single-data transfer
	movs.l @r4+r9,x0
! CHECK: error: invalid modulo index register for DSP double-data transfer
	nopx movy.w @r2+r8,y0
