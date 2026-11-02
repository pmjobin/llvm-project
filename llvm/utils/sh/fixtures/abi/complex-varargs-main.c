// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "complex-varargs.h"
#include <stdio.h>

int main(void) {
  complex_float single;
  complex_double wide;
  __real__ single = __real__ wide = 1.25;
  __imag__ single = __imag__ wide = -3.5;
  struct single_float later = {9.0f};
  unsigned tail = 0x31415926U;
  unsigned failures = !complex_even(0, single, tail, wide);
  failures |= !complex_odd(7.0f, 0, single, tail, later) << 1;
  failures |= !complex_double_odd(7.0f, 0, wide, later, tail) << 2;
  failures |= !complex_overflow(1, 2, 3, 4, 5, 6, 7, 0, single, later, tail)
              << 3;
  failures |= !complex_double_overflow(1, 2, 3, 4, 5, 6, 0, wide, single, tail)
              << 4;
  failures |=
      !complex_integer_overflow(1, 2, 3, 4, single, tail, wide, tail + 1) << 5;
  failures |=
      !complex_double_overflow_odd(1, 2, 3, 4, 5, 0, wide, later, single, tail)
      << 6;
  failures |= !complex_double_last_fit(1, 2, 3, 4, 0, wide, single, tail) << 7;
  failures |=
      !complex_exhausted(1, 2, 3, 4, 5, 6, 7, 8, 0, single, tail, wide, later)
      << 8;
  if (failures)
    printf("complex-varargs:FAIL:%x\n", failures);
  else
    puts("complex-varargs:PASS");
  return failures != 0;
}
