// RUN: not %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -S -o /dev/null %s 2>&1 | FileCheck %s
// RUN: not %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -S -o /dev/null %s 2>&1 | FileCheck %s

void invalid_sets(int value) {
	__asm__ volatile("" : : "P27"(4));
	__asm__ volatile("" : : "I08"(value));
	__asm__ volatile("" : : "Jmb"(0xfe));
	__asm__ volatile("" : : "Jmw"(0xfffe));
	__asm__ volatile("" : : "Jhb"(0x40000000));
}

// CHECK: error: value out of range for constraint 'P27'
// CHECK: error: constraint 'I08' expects an integer constant expression
// CHECK: error: value out of range for constraint 'Jmb'
// CHECK: error: value out of range for constraint 'Jmw'
// CHECK: error: value out of range for constraint 'Jhb'
