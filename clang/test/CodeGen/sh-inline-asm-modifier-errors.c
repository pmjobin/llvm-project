// RUN: not %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O1 -S -o /dev/null %s 2>&1 | FileCheck %s
// RUN: not %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O1 -S -o /dev/null %s 2>&1 | FileCheck %s

void bad_o_register(int value) {
	__asm__ volatile("! %O0" : : "r"(value));
}

void bad_o_memory(int *value) {
	__asm__ volatile("! %O0" : : "m"(*value));
}

void bad_r_register(int value) {
	__asm__ volatile("! %R0" : : "r"(value));
}

void bad_s_integer(void) {
	__asm__ volatile("! %S0" : : "i"(1));
}

void bad_t_register(int value) {
	__asm__ volatile("! %T0" : : "r"(value));
}

void bad_m_memory(int *value) {
	__asm__ volatile("! %M0" : : "m"(*value));
}

// CHECK-COUNT-2: error: invalid operand in inline asm: '! ${0:O}'
// CHECK: error: invalid operand in inline asm: '! ${0:R}'
// CHECK: error: invalid operand in inline asm: '! ${0:S}'
// CHECK: error: invalid operand in inline asm: '! ${0:T}'
// CHECK: error: invalid operand in inline asm: '! ${0:M}'
