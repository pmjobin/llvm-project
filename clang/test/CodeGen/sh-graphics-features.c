// RUN: %clang_cc1 -triple shle -target-cpu sh4 -DUSE_VECTOR -emit-llvm -o /dev/null %s
// RUN: %clang_cc1 -triple sh -target-cpu sh4a -DUSE_APPROX -emit-llvm -o /dev/null %s
// RUN: %clang --target=shle -m4a-single-only -DUSE_APPROX -O3 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh -m4a-single -DUSE_APPROX -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle -m4-single -DUSE_VECTOR -O3 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: not %clang_cc1 -triple shle -target-cpu sh4 -DUSE_APPROX -emit-llvm -o /dev/null %s 2>&1 | FileCheck %s --check-prefix=APPROX
// RUN: not %clang --target=shle -m4-nofpu -DUSE_VECTOR -S -emit-llvm -o /dev/null %s 2>&1 | FileCheck %s --check-prefix=VECTOR
// RUN: not %clang_cc1 -triple shle -target-cpu sh4al-dsp -DUSE_VECTOR -emit-llvm -o /dev/null %s 2>&1 | FileCheck %s --check-prefix=VECTOR
// RUN: not %clang_cc1 -triple shle -target-cpu sh2e -DUSE_VECTOR -emit-llvm -o /dev/null %s 2>&1 | FileCheck %s --check-prefix=VECTOR
// RUN: not %clang_cc1 -triple shle -target-cpu sh3e -DUSE_VECTOR -emit-llvm -o /dev/null %s 2>&1 | FileCheck %s --check-prefix=VECTOR
// RUN: not %clang_cc1 -triple shle -target-cpu sh4a -DUSE_STRICT -ffp-exception-behavior=strict -Werror=unsupported-floating-point-opt -emit-llvm -o /dev/null %s 2>&1 | FileCheck %s --check-prefix=STRICT
// RUN: not %clang_cc1 -triple shle -target-cpu sh4a -DUSE_HANDLER -emit-llvm -o /dev/null %s 2>&1 | FileCheck %s --check-prefix=HANDLER
// APPROX: needs target feature sh4a,single-fpu
// VECTOR: needs target feature fpu-vector
// STRICT: overriding currently unsupported use of floating point exceptions
// HANDLER: SH graphics intrinsic in a handler or strict FP function
typedef float float2 __attribute__((vector_size(8)));
typedef float float4 __attribute__((vector_size(16)));
#ifdef USE_VECTOR
float dot(float4 a, float4 b) { return __builtin_sh_fipr(a, b); }
void array(void *d, const void *s, unsigned n, const void *m) {
  __builtin_sh_ftrv_array(d, s, n, m);
}
void compose(void *d, const void *s, unsigned n, const void *a, const void *b) {
  __builtin_sh_ftrv_compose(d, s, n, a, b);
}
#endif
#ifdef USE_APPROX
float2 phase(unsigned p) { return __builtin_sh_fsca(p); }
#endif
#ifdef USE_STRICT
float reciprocal(float x) { return __builtin_sh_fsrra(x); }
#endif
#ifdef USE_HANDLER
__attribute__((interrupt_handler)) void handler(void) { __builtin_sh_fsrra(3); }
#endif
