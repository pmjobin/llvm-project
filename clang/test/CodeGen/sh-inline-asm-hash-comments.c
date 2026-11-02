// RUN: %clang --target=shle-unknown-linux-gnu -m4 -Os -c -fintegrated-as -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -O3 -c -fintegrated-as -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -Os -S -fno-integrated-as -mllvm -verify-machineinstrs %s -o - | FileCheck %s
// RUN: %clang --target=shle-unknown-linux-gnu -m4 -O3 -S -fno-integrated-as -mllvm -verify-machineinstrs %s -o - | FileCheck %s

// CHECK-LABEL: tied_pointer:
// CHECK: !APP
// CHECK-NEXT: # an arbitrary comment
// CHECK-NEXT: !NO_APP
void *tied_pointer(const void *p) {
  void *q;
  __asm__ volatile("# an arbitrary comment" : "=r"(q) : "0"(p));
  return q;
}

// CHECK-LABEL: immediates:
// CHECK: add #1,r0
// CHECK: mov #7,r0
unsigned immediates(unsigned x) {
  __asm__ volatile("! ordinary SH comment\n\tadd #1,%0\n\tmov #7,%0" : "+r"(x));
  return x;
}
