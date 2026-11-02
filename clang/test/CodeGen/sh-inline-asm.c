// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s

int inline_add(int lhs, int rhs) {
	int result;
	__asm__("add %2,%0" : "=r"(result) : "0"(lhs), "r"(rhs));
	return result;
}

// CHECK-LABEL: inline_add:
// CHECK: !APP
// CHECK-NEXT: add r{{[0-9]+}},r{{[0-9]+}}
// CHECK-NEXT: !NO_APP

int inline_immediate(void) {
	int result;
	__asm__("mov #%1,%0" : "=r"(result) : "i"(7));
	return result;
}

// CHECK-LABEL: inline_immediate:
// CHECK: !APP
// CHECK-NEXT: mov #7,r{{[0-9]+}}
// CHECK-NEXT: !NO_APP
