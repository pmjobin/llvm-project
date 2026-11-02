# RUN: not llvm-mc -triple=sh-unknown-elf -mcpu=sh2 %s 2>&1 | FileCheck %s
# RUN: not llvm-mc -triple=shle-unknown-elf -mcpu=sh2 %s 2>&1 | FileCheck %s

	! SH2E/FPU
	fadd fr1,fr2
	! SH2A
	movi20 #1,r1
	! SH3
	ldc r1,ssr
	! SH3
	pref @r1
	! SH-DSP
	padd r1,r2

# CHECK: :5:2: error: instruction requires an unavailable feature
# CHECK: :7:9: error: unexpected operand
# CHECK: :9:2: error: instruction requires an unavailable feature
# CHECK: :11:2: error: instruction requires an unavailable feature
# CHECK: :13:2: error: unrecognized instruction mnemonic
