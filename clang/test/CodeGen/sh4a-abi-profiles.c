// RUN: %clang --target=shle-unknown-linux-gnu -m4a -O0 -S -emit-llvm -o - %s | FileCheck %s --check-prefix=FULL
// RUN: %clang --target=shle-unknown-linux-gnu -m4a-nofpu -O0 -S -emit-llvm -o - %s | FileCheck %s --check-prefix=NOFPU
// RUN: %clang --target=shle-unknown-linux-gnu -m4a-single -O0 -S -emit-llvm -o - %s | FileCheck %s --check-prefix=SINGLE
// RUN: %clang --target=shle-unknown-linux-gnu -m4a-single-only -O0 -S -emit-llvm -o - %s | FileCheck %s --check-prefix=SINGLE-ONLY
// RUN: %clang --target=shle-unknown-linux-gnu -m4al -O0 -S -emit-llvm -o - %s | FileCheck %s --check-prefix=SH4AL
// RUN: %clang --target=sh-unknown-linux-gnu -m4a -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4a -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -m4a-nofpu -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4a-nofpu -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -m4a-single -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4a-single -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -m4a-single-only -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4a-single-only -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -m4al -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4al -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s

// FULL: attributes #[[ATTR:[0-9]+]] = { {{.*}}"target-abi"="gnu-hard-float" "target-cpu"="sh4a" "target-features"="{{[^"]*}}+double-fpu{{[^"]*}}+sh4a,+sh4a-common,+single-fpu"
// NOFPU: attributes #[[ATTR:[0-9]+]] = { noimplicitfloat {{.*}}"target-abi"="gnu" "target-cpu"="sh4a" "target-features"="{{[^"]*}}+sh4a,+sh4a-common,-double-fpu{{[^"]*}}-single-fpu" "use-soft-float"="true" }
// SINGLE: attributes #[[ATTR:[0-9]+]] = { {{.*}}"target-abi"="gnu-hard-float-single" "target-cpu"="sh4a" "target-features"="{{[^"]*}}+fpu-single-mode{{[^"]*}}+sh4a,+sh4a-common,+single-fpu"
// SINGLE-ONLY: attributes #[[ATTR:[0-9]+]] = { {{.*}}"target-abi"="gnu-e-float" "target-cpu"="sh4a" "target-features"="{{[^"]*}}+fpu-single-mode{{[^"]*}}+sh4a,+sh4a-common,+single-fpu,-double-fpu{{[^"]*}}"
// SH4AL: attributes #[[ATTR:[0-9]+]] = { noimplicitfloat {{.*}}"target-abi"="gnu" "target-cpu"="sh4al-dsp" "target-features"="{{[^"]*}}+dsp{{[^"]*}}+sh4a,+sh4a-common,+sh4al-dsp" "use-soft-float"="true" }

typedef __builtin_va_list va_list;

struct Mixed {
	float f;
	double d;
	long long i;
};

struct Mixed sh4a_fixed(int i0, int i1, int i2, int i3, int i4, float f0, float f1, double d0, struct Mixed value) {
	value.f += f0 + f1;
	value.d += d0;
	value.i += i0 + i1 + i2 + i3 + i4;
	return value;
}

double sh4a_varargs(int count, ...) {
	va_list args;
	__builtin_va_start(args, count);
	double result = __builtin_va_arg(args, double);
	struct Mixed value = __builtin_va_arg(args, struct Mixed);
	__builtin_va_end(args);
	return result + value.f + value.d + value.i;
}
