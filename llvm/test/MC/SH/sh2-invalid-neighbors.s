! RUN: not llvm-mc -triple=sh-unknown-elf -mcpu=sh2 %s 2>&1 | FileCheck %s
! RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh2 %s 2>&1 | FileCheck %s

	ldc r1,mach
	lds r1,sr
	stc mach,r1
	sts sr,r1
	ldc.l @r1+,pr
	lds.l @r1+,gbr
	stc.l mach,@-r1
	sts.l sr,@-r1
	ldc r1,ssr
	mac.l @r1,@r2+
	mac.w @r1+,@r2
	mov.b @-r1,r2
	mov.b r1,@r2+
	mov.b @(0,gbr),r1
	mov.b r1,@(0,gbr)
	and.b #1,@(r1,gbr)
	trapa #-1
	trapa #256
	cmp/eq #128,r0
	and #-1,r0
	mov.w @(1,gbr),r0
	mov.l @(2,gbr),r0
	mov.b @(r1,r2),r3
	sett r1

! CHECK-COUNT-24: error:
