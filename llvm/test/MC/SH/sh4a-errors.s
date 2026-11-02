! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh4a %s 2>&1 | FileCheck %s

! CHECK-COUNT-10: error:
	movco.l	r1,@r2
	movli.l	@r1,r2
	movua.l	@r1,r2
	movua.l	@r1+,r1
	icbi	r1
	ldc	r16,sgr
	ldc.l	@r1,sgr
	prefi	r1
	fsrra	dr2
	fpchg	r0
