// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh4 -O1 -Wno-atomic-alignment -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefixes=BE,COMMON
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh4 -O1 -Wno-atomic-alignment -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefixes=LE,COMMON
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh4 -O0 -Wno-atomic-alignment -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh4 -O2 -Wno-atomic-alignment -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh4 -O3 -Wno-atomic-alignment -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh4 -Os -Wno-atomic-alignment -S -mllvm -verify-machineinstrs -o /dev/null %s

float add_f32(float a, float b) {
	return a + b;
}

// BE-LABEL: add_f32:
// BE: lds r5,fpul
// BE: lds r4,fpul
// BE: fadd
// BE: sts fpul,r0
// LE-LABEL: add_f32:
// LE: lds r5,fpul
// LE: lds r4,fpul
// LE: fadd
// LE: sts fpul,r0

double add_f64(double a, double b) {
	return a + b;
}

// BE-LABEL: add_f64:
// BE: lds r4,fpul
// BE: lds r5,fpul
// BE: fadd dr{{[0-9]+}},dr{{[0-9]+}}
// BE: sts fpul,r0
// BE: sts fpul,r1
// LE-LABEL: add_f64:
// LE: lds r5,fpul
// LE: lds r4,fpul
// LE: fadd dr{{[0-9]+}},dr{{[0-9]+}}
// LE: sts fpul,r1
// LE: sts fpul,r0

double inline_mode_clobber(double a, double b, volatile double *address) {
	double value = a + b;
	*address = value;
	__asm__ volatile("frchg" ::: "fpscr", "memory");
	return *address * b;
}

// BE-LABEL: inline_mode_clobber:
// BE: fadd
// BE: frchg
// BE: sts fpscr,r0
// BE: lds r0,fpscr
// BE: fmul
// LE-LABEL: inline_mode_clobber:
// LE: fadd
// LE: frchg
// LE: sts fpscr,r0
// LE: lds r0,fpscr
// LE: fmul

float inline_fr_clobber(float a, float b) {
	float value = a + b;
	__asm__ volatile("fldi0 fr0" ::: "fr0");
	return value * b;
}

// The scheduler may finish the arithmetic before the asm. The soft-ABI return
// bits must reach r0 before the asm destroys fr0, with no live FPR result left
// across the clobber.
// COMMON-LABEL: inline_fr_clobber:
// COMMON: lds r5,fpul
// COMMON-NEXT: fsts fpul,[[B:fr[0-9]+]]
// COMMON-NEXT: lds r4,fpul
// COMMON-NEXT: fsts fpul,fr0
// COMMON-NEXT: fadd [[B]],fr0
// COMMON-NEXT: fmul [[B]],fr0
// COMMON-NEXT: flds fr0,fpul
// COMMON-NEXT: sts fpul,r0
// COMMON-NEXT: !APP
// COMMON-NEXT: fldi0 fr0
// COMMON-NEXT: !NO_APP
// COMMON-NEXT: rts
// COMMON-NEXT: nop

typedef __builtin_va_list va_list;

double sum_varargs(int count, ...) {
	va_list values;
	__builtin_va_start(values, count);
	double result = __builtin_va_arg(values, double);
	result += __builtin_va_arg(values, double);
	__builtin_va_end(values);
	return result;
}

// BE-LABEL: sum_varargs:
// BE: fadd dr{{[0-9]+}},dr{{[0-9]+}}
// LE-LABEL: sum_varargs:
// LE: fadd dr{{[0-9]+}},dr{{[0-9]+}}

typedef _Atomic(float) atomic_float;
typedef _Atomic(double) atomic_double;

float atomic_add_f32(atomic_float *address, float value) {
	return __c11_atomic_load(address, __ATOMIC_ACQUIRE) + value;
}

// BE-LABEL: atomic_add_f32:
// BE: fadd fr{{[0-9]+}},fr{{[0-9]+}}
// BE: __atomic_load_4
// LE-LABEL: atomic_add_f32:
// LE: fadd fr{{[0-9]+}},fr{{[0-9]+}}
// LE: __atomic_load_4

double atomic_add_f64(atomic_double *address, double value) {
	return __c11_atomic_load(address, __ATOMIC_ACQUIRE) + value;
}

// BE-LABEL: atomic_add_f64:
// BE: fadd dr{{[0-9]+}},dr{{[0-9]+}}
// BE: __atomic_load
// LE-LABEL: atomic_add_f64:
// LE: fadd dr{{[0-9]+}},dr{{[0-9]+}}
// LE: __atomic_load
