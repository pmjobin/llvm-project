# REQUIRES: sh-registered-target
# RUN: llvm-mca -mtriple=shle-unknown-none-elf -mcpu=sh4al-dsp -iterations=1 %s | FileCheck %s

	add r0,r1
	mov.l @r2,r3
	mul.l r4,r5
	mac.l @r6+,@r7+
	ldrc #1
	movx.w @r4,x0 nopy
	padd x0,y0,a0
	padd a0,m0,a0 pmuls x0,y0,m0
	synco

# CHECK: Instructions:      9
# CHECK: Dispatch Width:    2
# CHECK: Instruction Info:
# CHECK: add	r0,r1
# CHECK: mov.l	@r2,r3
# CHECK: mul.l	r4,r5
# CHECK: mac.l	@r6+,@r7+
# CHECK: ldrc	#1
# CHECK: movx.w	@r4,x0
# CHECK: padd	x0,y0,a0
# CHECK: padd	a0,m0,a0	pmuls	x0,y0,m0
# CHECK: synco
# CHECK: Resources:
# CHECK-DAG: SH4ALDSP_CO
# CHECK-DAG: SH4ALDSP_DSP_EX
# CHECK-DAG: SH4ALDSP_DSP_LS
# CHECK-DAG: SH4ALDSP_DSP_Mult
# CHECK-DAG: SH4ALDSP_EX
# CHECK-DAG: SH4ALDSP_LS
# CHECK-DAG: SH4ALDSP_Memory
# CHECK-DAG: SH4ALDSP_Mult
# CHECK: Resource pressure by instruction:
