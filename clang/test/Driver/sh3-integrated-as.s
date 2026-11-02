// RUN: %clang -target sh-unknown-none-elf -mcpu=sh3 -c %s -o %t.be.o
// RUN: %clang -target shle-unknown-none-elf -mcpu=sh3 -c %s -o %t.le.o
// RUN: llvm-readobj --file-headers --hex-dump=.text %t.be.o | FileCheck %s --check-prefixes=COMMON,BE
// RUN: llvm-readobj --file-headers --hex-dump=.text %t.le.o | FileCheck %s --check-prefixes=COMMON,LE
// RUN: llvm-objdump -d --mcpu=sh3 %t.be.o | FileCheck %s --check-prefix=DIS
// RUN: llvm-objdump -d --mcpu=sh3 %t.le.o | FileCheck %s --check-prefix=DIS

	.text
	.globl sh3_integrated_assembler
	.type sh3_integrated_assembler,@function
sh3_integrated_assembler:
	shad r2,r3
	pref @r3
	rts
	nop
	.size sh3_integrated_assembler,.-sh3_integrated_assembler

// COMMON: Machine: EM_SH (0x2A)
// COMMON: Flags [ (0x3)
// BE: Hex dump of section '.text':
// BE-NEXT: 0x00000000 432c0383 000b0009
// LE: Hex dump of section '.text':
// LE-NEXT: 0x00000000 2c438303 0b000900
// DIS: shad	r2,r3
// DIS-NEXT: pref	@r3
// DIS-NEXT: rts
// DIS-NEXT: nop
