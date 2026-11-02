// RUN: %clang --target=shle-unknown-linux-gnu -m4-nofpu -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=NOFPU
// RUN: %clang --target=shle-unknown-linux-gnu -m4-single -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=SINGLE
// RUN: %clang --target=shle-unknown-linux-gnu -m4-single-only -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=SINGLE-ONLY
// RUN: %clang --target=shle-unknown-linux-gnu -m2a -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=SH2A-FULL
// RUN: %clang --target=shle-unknown-linux-gnu -m2a-nofpu -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=SH2A-NOFPU
// RUN: %clang --target=shle-unknown-linux-gnu -m2a-single -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=SH2A-SINGLE
// RUN: %clang --target=shle-unknown-linux-gnu -m2a-single-only -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=SH2A-SINGLE-ONLY
// RUN: %clang --target=shle-unknown-linux-gnu -m2a -O1 -S %s -o - | FileCheck %s --check-prefix=SH2A-FULL-ASM
// RUN: %clang --target=shle-unknown-linux-gnu -m2a-nofpu -O1 -S %s -o - | FileCheck %s --check-prefix=SH2A-NOFPU-ASM
// RUN: %clang --target=shle-unknown-linux-gnu -m2a-single -O1 -S %s -o - | FileCheck %s --check-prefix=SH2A-SINGLE-ASM
// RUN: %clang --target=shle-unknown-linux-gnu -m2a-single-only -O1 -S %s -o - | FileCheck %s --check-prefix=SH2A-SINGLE-ONLY-ASM

float add_float(float left, float right) {
	return left + right;
}

double add_double(double left, double right) {
	return left + right;
}

long double add_long_double(long double left, long double right) {
	return left + right;
}

float root_float(float value) {
	return __builtin_sqrtf(value);
}

// NOFPU: define dso_local float @add_float(float noundef %left, float noundef %right) #[[NOFPU_ATTR:[0-9]+]]
// NOFPU: define dso_local double @add_double(double noundef %left, double noundef %right) #[[NOFPU_ATTR]]
// NOFPU: attributes #[[NOFPU_ATTR]] = { {{.*}}noimplicitfloat{{.*}} "target-abi"="gnu" "target-cpu"="sh4" "target-features"="{{[^"]*}}-double-fpu{{[^"]*}}-single-fpu" "use-soft-float"="true" }
// NOFPU: attributes #[[NOFPU_DECL:[0-9]+]] = { {{.*}}noimplicitfloat{{.*}} "target-abi"="gnu" "target-cpu"="sh4" "target-features"="{{[^"]*}}-double-fpu{{[^"]*}}-single-fpu" "use-soft-float"="true" }
// NOFPU: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu"}

// SINGLE: define dso_local float @add_float(float noundef %left, float noundef %right) #[[SINGLE_ATTR:[0-9]+]]
// SINGLE: define dso_local double @add_double(double noundef %left, double noundef %right) #[[SINGLE_ATTR]]
// SINGLE: attributes #[[SINGLE_ATTR]] = { {{.*}} "target-abi"="gnu-hard-float-single" "target-cpu"="sh4" "target-features"="{{[^"]*}}+double-fpu{{[^"]*}}+fpu-single-mode{{[^"]*}}+single-fpu" }
// SINGLE: attributes #[[SINGLE_DECL:[0-9]+]] = { {{.*}} "target-abi"="gnu-hard-float-single" "target-cpu"="sh4" "target-features"="{{[^"]*}}+double-fpu{{[^"]*}}+fpu-single-mode{{[^"]*}}+single-fpu" }
// SINGLE: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-hard-float-single"}

// SINGLE-ONLY: define dso_local float @add_float(float noundef %left, float noundef %right) #[[E_ATTR:[0-9]+]]
// SINGLE-ONLY: define dso_local float @add_double(float noundef %left, float noundef %right) #[[E_ATTR]]
// SINGLE-ONLY: define dso_local double @add_long_double(double noundef %left, double noundef %right) #[[E_ATTR]]
// SINGLE-ONLY: attributes #[[E_ATTR]] = { {{.*}} "target-abi"="gnu-e-float" "target-cpu"="sh4" "target-features"="{{[^"]*}}+fpu-single-mode{{[^"]*}}+single-fpu{{[^"]*}}-double-fpu{{[^"]*}}" }
// SINGLE-ONLY: attributes #[[E_DECL:[0-9]+]] = { {{.*}} "target-abi"="gnu-e-float" "target-cpu"="sh4" "target-features"="{{[^"]*}}+fpu-single-mode{{[^"]*}}+single-fpu{{[^"]*}}-double-fpu{{[^"]*}}" }
// SINGLE-ONLY: !{{[0-9]+}} = !{i32 1, !"target-abi", !"gnu-e-float"}

// SH2A-FULL: define dso_local float @add_float(float noundef %left, float noundef %right) #[[SH2A_FULL:[0-9]+]]
// SH2A-FULL: define dso_local double @add_double(double noundef %left, double noundef %right) #[[SH2A_FULL]]
// SH2A-FULL: attributes #[[SH2A_FULL]] = { {{.*}}"target-abi"="gnu-hard-float" "target-cpu"="sh2a" "target-features"="+double-fpu,+dynamic-shift,+fpu-size-mode,+fsqrt,+prefetch,+sh2,+sh2a,+single-fpu" }
// SH2A-NOFPU: define dso_local float @add_float(float noundef %left, float noundef %right) #[[SH2A_NOFPU:[0-9]+]]
// SH2A-NOFPU: define dso_local double @add_double(double noundef %left, double noundef %right) #[[SH2A_NOFPU]]
// SH2A-NOFPU: attributes #[[SH2A_NOFPU]] = { {{.*}}noimplicitfloat{{.*}}"target-abi"="gnu" "target-cpu"="sh2a" "target-features"="+dynamic-shift,+prefetch,+sh2,+sh2a,-double-fpu,-fpu-register-bank,-fpu-single-mode,-fpu-size-mode,-fpu-vector,-fsqrt,-single-fpu" "use-soft-float"="true" }
// SH2A-SINGLE: define dso_local double @add_double(double noundef %left, double noundef %right) #[[SH2A_SINGLE:[0-9]+]]
// SH2A-SINGLE: attributes #[[SH2A_SINGLE]] = { {{.*}}"target-abi"="gnu-hard-float-single" "target-cpu"="sh2a" "target-features"="+double-fpu,+dynamic-shift,+fpu-single-mode,+fpu-size-mode,+fsqrt,+prefetch,+sh2,+sh2a,+single-fpu,-fpu-register-bank,-fpu-vector" }
// SH2A-SINGLE-ONLY: define dso_local float @add_double(float noundef %left, float noundef %right) #[[SH2A_SINGLE_ONLY:[0-9]+]]
// SH2A-SINGLE-ONLY: attributes #[[SH2A_SINGLE_ONLY]] = { {{.*}}"target-abi"="gnu-e-float" "target-cpu"="sh2a" "target-features"="+dynamic-shift,+fpu-single-mode,+fsqrt,+prefetch,+sh2,+sh2a,+single-fpu,-double-fpu,-fpu-register-bank,-fpu-size-mode,-fpu-vector" }

// SH2A-FULL-ASM-LABEL: add_float:
// SH2A-FULL-ASM: lds r0,fpscr
// SH2A-FULL-ASM: fadd fr4,fr0
// SH2A-FULL-ASM-LABEL: add_double:
// SH2A-FULL-ASM: fadd dr6,dr0
// SH2A-NOFPU-ASM-LABEL: add_float:
// SH2A-NOFPU-ASM: __addsf3@PLT
// SH2A-NOFPU-ASM-LABEL: add_double:
// SH2A-NOFPU-ASM: __adddf3@PLT
// SH2A-SINGLE-ASM-LABEL: add_float:
// SH2A-SINGLE-ASM-NOT: fpscr
// SH2A-SINGLE-ASM: fadd fr4,fr0
// SH2A-SINGLE-ASM-LABEL: add_double:
// SH2A-SINGLE-ASM: lds r0,fpscr
// SH2A-SINGLE-ASM: fadd dr6,dr0
// SH2A-SINGLE-ONLY-ASM-LABEL: add_double:
// SH2A-SINGLE-ONLY-ASM: fadd fr5,fr0
