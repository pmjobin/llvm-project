// RUN: %clang -target sh-unknown-none-elf -mcpu=sh2 -c %s -o %t.bare-be.o
// RUN: llvm-readobj --file-headers --sections --symbols %t.bare-be.o | FileCheck %s --check-prefixes=COMMON,BE
// RUN: %clang -target shle-unknown-none-elf -mcpu=sh2 -c %s -o %t.bare-le.o
// RUN: llvm-readobj --file-headers --sections --symbols %t.bare-le.o | FileCheck %s --check-prefixes=COMMON,LE
// RUN: %clang -target sh-unknown-linux-gnu -mcpu=sh2 -c %s -o %t.linux-be.o
// RUN: llvm-readobj --file-headers --sections --symbols %t.linux-be.o | FileCheck %s --check-prefixes=COMMON,BE
// RUN: %clang -target shle-unknown-linux-gnu -mcpu=sh2 -c %s -o %t.linux-le.o
// RUN: llvm-readobj --file-headers --sections --symbols %t.linux-le.o | FileCheck %s --check-prefixes=COMMON,LE

void integrated_assembler(void) {
	__asm__ volatile("sett\n\tstc sr,r1\n\tmuls.w r1,r2\n\tbf/s 1f\n\tnop\n1:" ::: "r1", "r2", "t");
}

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
