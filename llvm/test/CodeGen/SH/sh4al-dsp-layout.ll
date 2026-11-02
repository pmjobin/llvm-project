; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4al-dsp -target-abi=gnu -O0 -verify-machineinstrs -filetype=obj %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4al-dsp -target-abi=gnu -O2 -verify-machineinstrs -filetype=obj %s -o %t.le.o
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4al-dsp -target-abi=gnu -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llvm-readobj --file-headers %t.be.o %t.le.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-objdump -d --mcpu=sh4al-dsp %t.be.o > /dev/null
; RUN: llvm-objdump -d --mcpu=sh4al-dsp %t.le.o > /dev/null

define i32 @dsp_long_branch(i32 %condition) #0 {
entry:
	%iszero = icmp eq i32 %condition, 0
	br i1 %iszero, label %return, label %large

large:
	call void asm sideeffect ".rept 1300\0A\09padd x0,y0,a0\0A\09.endr", "~{a0},~{x0},~{y0},~{dsr}"()
	br label %return

return:
	%result = phi i32 [ 1, %entry ], [ 2, %large ]
	ret i32 %result
}

define i32 @dsp_literal_island(i32 %value) #0 {
entry:
	%before = add i32 %value, 305419896
	call void asm sideeffect ".rept 300\0A\09padd x0,y0,a0\0A\09.endr", "r,~{a0},~{x0},~{y0},~{dsr}"(i32 %before)
	%after = xor i32 %before, 1985229328
	ret i32 %after
}

attributes #0 = { "target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="+dsp,+sh4al-dsp" "tune-cpu"="sh4al-dsp" "use-soft-float"="true" }

; ASM-LABEL: dsp_long_branch:
; ASM: mov.w	[[BRANCH_LITERAL:.Lsh_relative_branch[0-9]+]],r0
; ASM-NEXT: braf	r0
; ASM: [[BRANCH_LITERAL]]:
; ASM-NEXT: .short	[[FAR:.LBB[0-9_]+]]-[[BRANCH_LITERAL]]
; ASM: padd x0,y0,a0
; ASM: [[FAR]]:
; ASM-LABEL: dsp_literal_island:
; ASM: .long	305419896
; ASM: .long	1985229328
; ASM: padd x0,y0,a0
; OBJECT-COUNT-2: Flags [ (0x6)
