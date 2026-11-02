// RUN: %clang -target sh-unknown-none-elf -mcpu=sh2 -c %s -o %t.bare-be.o
// RUN: llvm-readobj --file-headers --sections --symbols %t.bare-be.o | FileCheck %s --check-prefixes=COMMON,BE
// RUN: %clang -target shle-unknown-none-elf -mcpu=sh2 -c %s -o %t.bare-le.o
// RUN: llvm-readobj --file-headers --sections --symbols %t.bare-le.o | FileCheck %s --check-prefixes=COMMON,LE
// RUN: %clang -target sh-unknown-linux-gnu -mcpu=sh2 -c %s -o %t.linux-be.o
// RUN: llvm-readobj --file-headers --sections --symbols %t.linux-be.o | FileCheck %s --check-prefixes=COMMON,BE
// RUN: %clang -target shle-unknown-linux-gnu -mcpu=sh2 -c %s -o %t.linux-le.o
// RUN: llvm-readobj --file-headers --sections --symbols %t.linux-le.o | FileCheck %s --check-prefixes=COMMON,LE
// RUN: %clang -target shle-unknown-none-elf -m2 -c %s -o %t.m2.o
// RUN: llvm-readobj --file-headers %t.m2.o | FileCheck %s --check-prefix=SH2
// RUN: %clang -target shle-unknown-none-elf -m3 -c %s -o %t.m3.o
// RUN: llvm-readobj --file-headers %t.m3.o | FileCheck %s --check-prefix=SH3
// RUN: %clang -target shle-unknown-none-elf -m4 -c %s -o %t.m4.o
// RUN: llvm-readobj --file-headers %t.m4.o | FileCheck %s --check-prefix=SH4

	.text
	.globl integrated_assembler
	.type integrated_assembler,@function
integrated_assembler:
	sett
	stc sr,r1
	muls.w r1,r2
	bf/s 1f
	nop
1:
	rts
	nop
	.size integrated_assembler,.-integrated_assembler

// BE: Format: elf32-sh
// LE: Format: elf32-shl
// COMMON: AddressSize: 32bit
// BE: DataEncoding: BigEndian
// LE: DataEncoding: LittleEndian
// COMMON: Machine: EM_SH (0x2A)
// COMMON: Flags [ (0x2)
// COMMON: Name: .text
// COMMON: Name: integrated_assembler
// COMMON: Type: Function
// SH2: Flags [ (0x2)
// SH3: Flags [ (0x3)
// SH4: Flags [ (0x9)
