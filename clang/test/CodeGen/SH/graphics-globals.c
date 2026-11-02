// RUN: %clang --target=shle-unknown-linux-gnu -m4 -fno-pic -fno-pie -O3 -S %s -o - | FileCheck %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -fno-pic -fno-pie -O3 -mllvm -force-new-pm-codegen=true -S %s -o - | FileCheck %s
// REQUIRES: sh-registered-target
#include <shintrin.h>

void constant_matrix(sh_float4 out[4], const sh_float4 in[4]) {
  static const sh_matrix4 matrix = {{{2, 0, 0, 0}, {0, 3, 0, 0},
                              {0, 0, 4, 0}, {0, 0, 0, 1}}};
  sh_ftrv4(out, in, &matrix);
}
// CHECK-LABEL: constant_matrix:
// CHECK-COUNT-4: ftrv
// CHECK: .rodata
