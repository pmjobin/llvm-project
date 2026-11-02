/* Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

/* Compile-only GNU controls; no SH graphics intrinsic is used here. */
extern void unknown_call(void);

double double_pressure(const double *v) {
  double a = v[0] + v[1], b = v[2] + v[3], c = v[4] + v[5];
  double d = v[6] + v[7], e = v[8] + v[9], f = v[10] + v[11];
  double g = v[12] + v[13], h = v[14] + v[15];
  unknown_call();
  return ((((((a + b) + c) + d) + e) + f) + g) + h;
}

/* Empty asm declares clobbers to expose GNU's callee-save policy. It is a
 * compiler-contract witness, not a runtime instruction that changes XD0. */
void declared_xd_clobbers(void) {
  __asm__ volatile("" : : : "xd0", "xd2", "xd12", "xd14");
}

float ordinary_float(float a, float b) { return a + b; }
double ordinary_double(double a, double b) { return a + b; }
