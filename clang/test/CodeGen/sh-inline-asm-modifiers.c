// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O1 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=BE
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O1 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=LE
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O1 -S -pic-level 2 -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=BE
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O1 -S -pic-level 2 -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=LE

void parts(long long *source, int *low, int *high, int *next) {
	int low_value;
	int high_value;
	int next_value;
	__asm__("mov.l %R3,%0\n\tmov.l %S3,%1\n\tmov.l %T3,%2" : "=&r"(low_value), "=&r"(high_value), "=&r"(next_value) : "m"(*source));
	*low = low_value;
	*high = high_value;
	*next = next_value;
}

// BE-LABEL: parts:
// BE: mov.l @(4,r4),r{{[0-9]+}}
// BE-NEXT: mov.l @r4,r{{[0-9]+}}
// BE-NEXT: mov.l @(4,r4),r{{[0-9]+}}

// LE-LABEL: parts:
// LE: mov.l @r4,r{{[0-9]+}}
// LE-NEXT: mov.l @(4,r4),r{{[0-9]+}}
// LE-NEXT: mov.l @(4,r4),r{{[0-9]+}}

static long long static_source;

void static_parts(int *low, int *high, int *next) {
	int low_value;
	int high_value;
	int next_value;
	__asm__("mov.l %R3,%0\n\tmov.l %S3,%1\n\tmov.l %T3,%2" : "=&r"(low_value), "=&r"(high_value), "=&r"(next_value) : "m"(static_source));
	*low = low_value;
	*high = high_value;
	*next = next_value;
}

// BE-LABEL: static_parts:
// BE: mov.l @(4,r{{[0-9]+}}),r{{[0-9]+}}
// BE-NEXT: mov.l @r{{[0-9]+}},r{{[0-9]+}}
// BE-NEXT: mov.l @(4,r{{[0-9]+}}),r{{[0-9]+}}

// LE-LABEL: static_parts:
// LE: mov.l @r{{[0-9]+}},r{{[0-9]+}}
// LE-NEXT: mov.l @(4,r{{[0-9]+}}),r{{[0-9]+}}
// LE-NEXT: mov.l @(4,r{{[0-9]+}}),r{{[0-9]+}}

static int static_symbol;

void symbol_without_prefix(void) {
	__asm__ volatile(".long %O0" : : "i"(&static_symbol));
}

// BE-LABEL: symbol_without_prefix:
// BE: .long static_symbol
// LE-LABEL: symbol_without_prefix:
// LE: .long static_symbol
