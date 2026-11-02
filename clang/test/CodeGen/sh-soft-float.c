// RUN: %clang_cc1 -triple sh-unknown-none-elf -O0 -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -O0 -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -O0 -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -O0 -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -O1 -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -O1 -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -O2 -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -O2 -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -Os -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -Os -emit-obj -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh4 -O0 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=SH4-SOFT --implicit-check-not='xf{{[0-9]+}}' --implicit-check-not='xd{{[0-9]+}}' --implicit-check-not=frchg --implicit-check-not=fmac
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh4 -O2 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=SH4-SOFT --implicit-check-not='xf{{[0-9]+}}' --implicit-check-not='xd{{[0-9]+}}' --implicit-check-not=frchg --implicit-check-not=fmac
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh4 -Os -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=SH4-SOFT --implicit-check-not='xf{{[0-9]+}}' --implicit-check-not='xd{{[0-9]+}}' --implicit-check-not=frchg --implicit-check-not=fmac
// RUN: %clang_cc1 -triple shle-unknown-none-elf -O0 -emit-llvm -o - %s | FileCheck %s

struct pair {
	float f;
	double d;
};

// CHECK: define{{.*}} double @long_double_arithmetic(double{{.*}} %a, double{{.*}} %b)
// CHECK: fadd double
long double long_double_arithmetic(long double a, long double b) {
	return a + b;
}

// SH4-SOFT-LABEL: long_double_arithmetic:
// SH4-SOFT: fadd dr{{[0-9]+}},dr{{[0-9]+}}

// CHECK: define{{.*}} float @float_arithmetic(float{{.*}} %a, float{{.*}} %b)
// CHECK: fadd float
// CHECK: fsub float
// CHECK: fmul float
// CHECK: fdiv float
float float_arithmetic(float a, float b) {
	return (a + b) * (a - b) / b;
}

// SH4-SOFT-LABEL: float_arithmetic:
// SH4-SOFT-DAG: fsub fr{{[0-9]+}},fr{{[0-9]+}}
// SH4-SOFT-DAG: fadd fr{{[0-9]+}},fr{{[0-9]+}}
// SH4-SOFT-DAG: fmul fr{{[0-9]+}},fr{{[0-9]+}}
// SH4-SOFT-DAG: fdiv fr{{[0-9]+}},fr{{[0-9]+}}

// CHECK: define{{.*}} double @double_arithmetic(double{{.*}} %a, double{{.*}} %b)
// CHECK: alloca double, align 4
// CHECK: fadd double
// CHECK: fsub double
// CHECK: fmul double
// CHECK: fdiv double
double double_arithmetic(double a, double b) {
	return (a + b) * (a - b) / b;
}

// SH4-SOFT-LABEL: double_arithmetic:
// SH4-SOFT-DAG: fsub dr{{[0-9]+}},dr{{[0-9]+}}
// SH4-SOFT-DAG: fadd dr{{[0-9]+}},dr{{[0-9]+}}
// SH4-SOFT-DAG: fmul dr{{[0-9]+}},dr{{[0-9]+}}
// SH4-SOFT-DAG: fdiv dr{{[0-9]+}},dr{{[0-9]+}}

// CHECK: define{{.*}} float @float_remainder(float{{.*}} %a, float{{.*}} %b)
// CHECK: frem float
float float_remainder(float a, float b) {
	return __builtin_fmodf(a, b);
}

// SH4-SOFT-LABEL: float_remainder:
// SH4-SOFT: .long fmodf{{(@PLT)?}}

// CHECK: define{{.*}} double @double_remainder(double{{.*}} %a, double{{.*}} %b)
// CHECK: frem double
double double_remainder(double a, double b) {
	return __builtin_fmod(a, b);
}

// SH4-SOFT-LABEL: double_remainder:
// SH4-SOFT: .long fmod{{(@PLT)?}}

// CHECK: define{{.*}} i32 @compare(float{{.*}}, double{{.*}})
// CHECK: fcmp olt float
// CHECK: fcmp une double
int compare(float f, double d) {
	return f < 0.0f || d != 0.0;
}

// SH4-SOFT-LABEL: compare:
// SH4-SOFT: fcmp/gt fr{{[0-9]+}},fr{{[0-9]+}}
// SH4-SOFT: fcmp/eq dr{{[0-9]+}},dr{{[0-9]+}}

// CHECK: define{{.*}} float @convert(i32{{.*}}, i32{{.*}}, double{{.*}})
// CHECK: sitofp i32
// CHECK: uitofp i32
// CHECK: fptosi double
float convert(int s, unsigned u, double d) {
	return (float)s + (float)u + (float)(int)d;
}

// SH4-SOFT-LABEL: convert:
// SH4-SOFT: float fpul,fr{{[0-9]+}}
// SH4-SOFT: ftrc dr{{[0-9]+}},fpul
// SH4-SOFT: .long __floatunsisf{{(@PLT)?}}

// CHECK: define{{.*}} void @aggregate(ptr{{.*}} sret(%struct.pair) align 4{{.*}}, ptr{{.*}} byval(%struct.pair) align 4 %value)
struct pair aggregate(struct pair value) {
	value.f += (float)value.d;
	value.d += value.f;
	return value;
}

// SH4-SOFT-LABEL: aggregate:
// SH4-SOFT: fcnvds dr{{[0-9]+}},fpul
// SH4-SOFT: fadd fr{{[0-9]+}},fr{{[0-9]+}}
// SH4-SOFT: fcnvsd fpul,dr{{[0-9]+}}
// SH4-SOFT: fadd dr{{[0-9]+}},dr{{[0-9]+}}

// CHECK: define{{.*}} double @vararg(i32{{.*}} %fixed, ...)
// CHECK: call void @llvm.va_start
// CHECK: load double, ptr {{.*}}, align 4
double vararg(int fixed, ...) {
	__builtin_va_list args;
	__builtin_va_start(args, fixed);
	double result = __builtin_va_arg(args, double);
	__builtin_va_end(args);
	return result;
}

// SH4-SOFT-LABEL: vararg:
// SH4-SOFT: fmov.s @r0+,fr{{[0-9]+}}
// SH4-SOFT: fmov.s @r0+,fr{{[0-9]+}}
// SH4-SOFT: flds fr{{[0-9]+}},fpul
// SH4-SOFT: sts fpul,r{{[01]}}
