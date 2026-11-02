// RUN: %clang_cc1 -triple shle -target-cpu sh4a -target-abi gnu-hard-float -mfloat-abi hard -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple shle -target-cpu sh4a -target-abi gnu-hard-float -mfloat-abi hard -O0 -S -o /dev/null %s
// RUN: %clang_cc1 -triple sh -target-cpu sh4a -target-abi gnu-hard-float -mfloat-abi hard -O2 -S -o /dev/null %s
// RUN: not %clang_cc1 -triple shle -target-cpu sh2 -O2 -emit-llvm -o /dev/null %s 2>&1 | FileCheck %s --check-prefix=NOFPU

typedef float float2 __attribute__((vector_size(8)));
typedef float float4 __attribute__((vector_size(16)));
// CHECK-LABEL: define{{.*}} @phase(
// CHECK: call { float, float } @llvm.sh.fsca(i32
// NOFPU: needs target feature
float2 phase(unsigned p) { return __builtin_sh_fsca(p); }
// CHECK-LABEL: define{{.*}} float @rsqrt(
// CHECK: call float @llvm.sh.fsrra(float
float rsqrt(float x) { return __builtin_sh_fsrra(x); }
// CHECK-LABEL: define{{.*}} float @dot(
// CHECK: call float @llvm.sh.fipr(<4 x float>
float dot(float4 x, float4 y) { return __builtin_sh_fipr(x, y); }
// CHECK-LABEL: define{{.*}} @transform(
// CHECK: call <4 x float> @llvm.sh.ftrv(ptr
float4 transform(const void *m, float4 x) { return __builtin_sh_ftrv(m, x); }
// CHECK-LABEL: define{{.*}} void @batch(
// CHECK: call void @llvm.sh.ftrv4(ptr
void batch(void *d, const void *s, const void *m) { __builtin_sh_ftrv4(d,s,m); }
// CHECK-LABEL: define{{.*}} void @array(
// CHECK: call void @llvm.sh.ftrv.array(ptr{{.*}}, ptr{{.*}}, i32{{.*}}, ptr
void array(void *d, const void *s, unsigned n, const void *m) {
  __builtin_sh_ftrv_array(d, s, n, m);
}
// CHECK-LABEL: define{{.*}} void @compose(
// CHECK: call void @llvm.sh.ftrv.compose(ptr{{.*}}, ptr{{.*}}, i32{{.*}}, ptr{{.*}}, ptr
void compose(void *d, const void *s, unsigned n, const void *a, const void *b) {
  __builtin_sh_ftrv_compose(d, s, n, a, b);
}
