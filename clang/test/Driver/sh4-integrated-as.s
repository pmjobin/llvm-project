// RUN: %clang -target sh-unknown-none-elf -mcpu=sh4 -c %s -o %t.be.o
// RUN: %clang -target shle-unknown-none-elf -mcpu=sh4 -c %s -o %t.le.o
// RUN: llvm-readobj --file-headers %t.be.o | FileCheck %s --check-prefix=HEADER
// RUN: llvm-readobj --file-headers %t.le.o | FileCheck %s --check-prefix=HEADER
// RUN: llvm-objdump -d --mcpu=sh4 %t.be.o | FileCheck %s --check-prefix=DIS
// RUN: llvm-objdump -d --mcpu=sh4 %t.le.o | FileCheck %s --check-prefix=DIS

	.text
	.globl sh4_integrated_assembler
	.type sh4_integrated_assembler,@function
sh4_integrated_assembler:
	ldc r2,dbr
	ldc.l @r2+,dbr
	stc dbr,r3
	stc.l dbr,@-r3
	stc sgr,r3
	stc.l sgr,@-r3
	movca.l r0,@r3
	ocbi @r3
	ocbp @r3
	ocbwb @r3
	fmov fr2,fr3
	fmov.s @r2,fr3
	fmov.s fr2,@r3
	fmov.s @r2+,fr3
	fmov.s fr2,@-r3
	fmov.s @(r0,r2),fr3
	fmov.s fr2,@(r0,r3)
	fmov dr4,dr6
	fmov dr4,xd6
	fmov xd4,dr6
	fmov xd4,xd6
	fmov.d @r2,dr6
	fmov.d @r2,xd6
	fmov.d dr4,@r3
	fmov.d xd4,@r3
	fmov.d @r2+,dr6
	fmov.d @r2+,xd6
	fmov.d dr4,@-r3
	fmov.d xd4,@-r3
	fmov.d @(r0,r2),dr6
	fmov.d @(r0,r2),xd6
	fmov.d dr4,@(r0,r3)
	fmov.d xd4,@(r0,r3)
	fldi0 fr3
	fldi1 fr3
	flds fr2,fpul
	fsts fpul,fr3
	fabs fr3
	fneg fr3
	fadd fr2,fr3
	fsub fr2,fr3
	fmul fr2,fr3
	fmac fr0,fr2,fr3
	fdiv fr2,fr3
	fsqrt fr3
	fcmp/eq fr2,fr3
	fcmp/gt fr2,fr3
	float fpul,fr3
	ftrc fr2,fpul
	fipr fv8,fv12
	ftrv xmtrx,fv12
	fabs dr6
	fneg dr6
	fadd dr4,dr6
	fsub dr4,dr6
	fmul dr4,dr6
	fdiv dr4,dr6
	fsqrt dr6
	fcmp/eq dr4,dr6
	fcmp/gt dr4,dr6
	float fpul,dr6
	ftrc dr4,fpul
	fcnvds dr4,fpul
	fcnvsd fpul,dr6
	lds r2,fpscr
	sts fpscr,r3
	lds.l @r2+,fpscr
	sts.l fpscr,@-r3
	lds r2,fpul
	sts fpul,r3
	lds.l @r2+,fpul
	sts.l fpul,@-r3
	frchg
	fschg
	rts
	nop
	.size sh4_integrated_assembler,.-sh4_integrated_assembler

// HEADER: Machine: EM_SH (0x2A)
// HEADER: Flags [ (0x9)
// DIS: ldc	r2,dbr
// DIS: movca.l	r0,@r3
// DIS: fmov	fr2,fr3
// DIS: fadd	fr2,fr3
// DIS: fipr	fv8,fv12
// DIS: ftrv	xmtrx,fv12
// DIS: fcnvds	dr4,fpul
// DIS: frchg
// DIS-NEXT: fschg
// DIS-NEXT: rts
// DIS-NEXT: nop
