// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh4 -target-abi gnu-hard-float -Os -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh4 -target-abi gnu-hard-float -Oz -S -mllvm -verify-machineinstrs -o - %s | FileCheck %s

// A size optimization must not rematerialize MOVri directly into MACL.
// CHECK-LABEL: read_constant_macl:
// CHECK: mov #7,[[REG:r[0-9]+]]
// CHECK: lds [[REG]],macl
// CHECK: sts macl,r0
int read_constant_macl(void) {
  int result;
  __asm__ volatile("sts macl,%0" : "=r"(result) : "x"(7));
  return result;
}
