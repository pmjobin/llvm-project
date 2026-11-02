// RUN: %clang --target=shle -m4-single-only -O2 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=IR
// RUN: %clang --target=sh -m4a-single-only -O2 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=IR
// RUN: %clang --target=shle -m4-single-only -O2 -S -mllvm -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM --implicit-check-not=fpchg
// RUN: %clang --target=sh -m4-single-only -O2 -S -mllvm -force-new-pm-codegen=true -mllvm -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM --implicit-check-not=fpchg
// RUN: %clang --target=shle -m4a-single-only -O2 -S -mllvm -force-new-pm-codegen=true -mllvm -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
// RUN: %clang --target=sh -m4a-single-only -O2 -S -mllvm -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM

// Paired data movement and graphics use single values and remain separate
// capabilities from double arithmetic. The single-only language/ABI stays put.
_Static_assert(sizeof(double) == 4, "single-only double");
_Static_assert(sizeof(long double) == 8, "single-only long double");
typedef float float4 __attribute__((vector_size(16)));

// IR-LABEL: define{{.*}} float @dot(
// IR: call float @llvm.sh.fipr
// ASM-LABEL: dot:
// ASM: fipr
float dot(const float4 *a, const float4 *b) {
  return __builtin_sh_fipr(*a, *b);
}

// IR-LABEL: define{{.*}} void @transform(
// IR: call <4 x float> @llvm.sh.ftrv
// ASM-LABEL: transform:
// ASM: ftrv
float4 transform(const void *m, float4 v) {
  return __builtin_sh_ftrv(m, v);
}

// IR-LABEL: define{{.*}} float @scalar(float{{.*}}, float
// IR: fadd float
// ASM-LABEL: scalar:
// ASM: fadd fr
// ASM-NOT: fadd dr
double scalar(double a, double b) { return a + b; }

// IR: "target-abi"="gnu-e-float"
// IR-SAME: "target-features"="{{[^"]*}}+fpu-register-bank,+fpu-single-mode,+fpu-size-mode,+fpu-vector{{[^"]*}}-double-fpu"
