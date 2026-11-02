// RUN: %clang --target=sh-unknown-linux-gnu -m2 -O3 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=sh-unknown-linux-gnu -m2 -O3 -DLIBRARY_FORM -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=sh-unknown-linux-gnu -m2 -Os -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=sh-unknown-linux-gnu -m2 -Os -DLIBRARY_FORM -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=sh-unknown-linux-gnu -m4 -O3 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=sh-unknown-linux-gnu -m4 -O3 -DLIBRARY_FORM -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=sh-unknown-linux-gnu -m4 -Os -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=sh-unknown-linux-gnu -m4 -Os -DLIBRARY_FORM -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=shle-unknown-linux-gnu -m2 -O3 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=shle-unknown-linux-gnu -m2 -O3 -DLIBRARY_FORM -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=shle-unknown-linux-gnu -m2 -Os -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=shle-unknown-linux-gnu -m2 -Os -DLIBRARY_FORM -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -O3 -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -O3 -DLIBRARY_FORM -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -Os -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -Os -DLIBRARY_FORM -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clangxx --target=shle-unknown-linux-gnu -m4 -O3 -x c++ -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clangxx --target=shle-unknown-linux-gnu -m4 -Os -DLIBRARY_FORM -x c++ -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s

#ifdef __cplusplus
extern "C" {
#endif
extern float ceilf(float);
extern float floorf(float);
extern double ceil(double);
extern double floor(double);

// Size optimization can merge equivalent wrappers; exercise the two forms
// in separate modules so each external call remains visible.
#ifdef LIBRARY_FORM
#define CEIL_F32 ceilf
#define FLOOR_F32 floorf
#define CEIL_F64 ceil
#define FLOOR_F64 floor
#else
#define CEIL_F32 __builtin_ceilf
#define FLOOR_F32 __builtin_floorf
#define CEIL_F64 __builtin_ceil
#define FLOOR_F64 __builtin_floor
#endif

// CHECK-LABEL: ceil_float:
// CHECK: jsr
// CHECK: .long	ceilf
void ceil_float(float x, float *out) {
	*(volatile float *)out = CEIL_F32(x);
}

// CHECK-LABEL: ceil_double:
// CHECK: jsr
// CHECK: .long	ceil
void ceil_double(double x, double *out) {
	*(volatile double *)out = CEIL_F64(x);
}

// CHECK-LABEL: floor_float:
// CHECK: jsr
// CHECK: .long	floorf
void floor_float(float x, float *out) {
	*(volatile float *)out = FLOOR_F32(x);
}

// CHECK-LABEL: floor_double:
// CHECK: jsr
// CHECK: .long	floor
void floor_double(double x, double *out) {
	*(volatile double *)out = FLOOR_F64(x);
}

#ifdef __cplusplus
}
#endif
