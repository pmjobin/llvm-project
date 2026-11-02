// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "complex-varargs.h"
#include <stdarg.h>

#define CHECK_COMPLEX(Z) (__real__(Z) == 1.25 && __imag__(Z) == -3.5)
#define TAIL 0x31415926U

int complex_even(int fixed, ...) {
  va_list ap, copy;
  va_start(ap, fixed);
  va_copy(copy, ap);
  complex_float z = va_arg(ap, complex_float);
  unsigned tail = va_arg(ap, unsigned);
  complex_double d = va_arg(ap, complex_double);
  complex_float zcopy = va_arg(copy, complex_float);
  unsigned tailcopy = va_arg(copy, unsigned);
  complex_double dcopy = va_arg(copy, complex_double);
  va_end(copy);
  va_end(ap);
  return CHECK_COMPLEX(z) && CHECK_COMPLEX(d) && CHECK_COMPLEX(zcopy) &&
         CHECK_COMPLEX(dcopy) && tail == TAIL && tailcopy == TAIL;
}

int complex_odd(float named, int fixed, ...) {
  va_list ap;
  va_start(ap, fixed);
  complex_float z = va_arg(ap, complex_float);
  unsigned tail = va_arg(ap, unsigned);
  struct single_float f = va_arg(ap, struct single_float);
  va_end(ap);
  return named == 7.0f && CHECK_COMPLEX(z) && tail == TAIL && f.value == 9.0f;
}

int complex_double_odd(float named, int fixed, ...) {
  va_list ap;
  va_start(ap, fixed);
  complex_double z = va_arg(ap, complex_double);
  struct single_float f = va_arg(ap, struct single_float);
  unsigned tail = va_arg(ap, unsigned);
  va_end(ap);
  return named == 7.0f && CHECK_COMPLEX(z) && f.value == 9.0f && tail == TAIL;
}

int complex_overflow(float a, float b, float c, float d, float e, float f,
                     float g, int fixed, ...) {
  va_list ap, copy;
  va_start(ap, fixed);
  va_copy(copy, ap);
  complex_float z = va_arg(ap, complex_float);
  struct single_float later = va_arg(ap, struct single_float);
  unsigned tail = va_arg(ap, unsigned);
  complex_float zcopy = va_arg(copy, complex_float);
  struct single_float latercopy = va_arg(copy, struct single_float);
  unsigned tailcopy = va_arg(copy, unsigned);
  va_end(copy);
  va_end(ap);
  return a + b + c + d + e + f + g == 28.0f && CHECK_COMPLEX(z) &&
         CHECK_COMPLEX(zcopy) && later.value == 9.0f &&
         latercopy.value == 9.0f && tail == TAIL && tailcopy == TAIL;
}

int complex_double_overflow(float a, float b, float c, float d, float e,
                            float f, int fixed, ...) {
  va_list ap;
  va_start(ap, fixed);
  complex_double z = va_arg(ap, complex_double);
  complex_float later = va_arg(ap, complex_float);
  unsigned tail = va_arg(ap, unsigned);
  va_end(ap);
  return a + b + c + d + e + f == 21.0f && CHECK_COMPLEX(z) &&
         CHECK_COMPLEX(later) && tail == TAIL;
}

int complex_integer_overflow(int a, int b, int c, int fixed, ...) {
  va_list ap;
  va_start(ap, fixed);
  complex_float z = va_arg(ap, complex_float);
  unsigned tail = va_arg(ap, unsigned);
  complex_double d = va_arg(ap, complex_double);
  unsigned second = va_arg(ap, unsigned);
  va_end(ap);
  return a + b + c + fixed == 10 && CHECK_COMPLEX(z) && CHECK_COMPLEX(d) &&
         tail == TAIL && second == TAIL + 1;
}

// Rounding an odd FPR cursor must not consume the skipped single slot when the
// complete complex double goes to the stack. The later float then consumes that
// odd slot, and the complex float starts at an even cursor.
int complex_double_overflow_odd(float a, float b, float c, float d, float e,
                                int fixed, ...) {
  va_list ap;
  va_start(ap, fixed);
  complex_double z = va_arg(ap, complex_double);
  struct single_float later = va_arg(ap, struct single_float);
  complex_float final = va_arg(ap, complex_float);
  unsigned tail = va_arg(ap, unsigned);
  va_end(ap);
  return a + b + c + d + e == 15.0f && CHECK_COMPLEX(z) &&
         later.value == 9.0f && CHECK_COMPLEX(final) && tail == TAIL;
}

// A complex double exactly fills FPR slots 4..7. The following complex float
// starts on the stack; copying that exhausted cursor must preserve both areas.
int complex_double_last_fit(float a, float b, float c, float d, int fixed,
                            ...) {
  va_list ap, copy;
  va_start(ap, fixed);
  complex_double wide = va_arg(ap, complex_double);
  va_copy(copy, ap);
  complex_float single = va_arg(ap, complex_float);
  unsigned tail = va_arg(ap, unsigned);
  complex_float singlecopy = va_arg(copy, complex_float);
  unsigned tailcopy = va_arg(copy, unsigned);
  va_end(copy);
  va_end(ap);
  return a + b + c + d == 10.0f && CHECK_COMPLEX(wide) &&
         CHECK_COMPLEX(single) && CHECK_COMPLEX(singlecopy) && tail == TAIL &&
         tailcopy == TAIL;
}

// Eight named floats leave no saved FPR slots. Both complex widths and the
// single-float record use consecutive stack slots while the integer tail still
// uses its independent cursor. Read the complete sequence through va_copy too.
int complex_exhausted(float a, float b, float c, float d, float e, float f,
                      float g, float h, int fixed, ...) {
  va_list ap, copy;
  va_start(ap, fixed);
  va_copy(copy, ap);
  complex_float single = va_arg(ap, complex_float);
  unsigned tail = va_arg(ap, unsigned);
  complex_double wide = va_arg(ap, complex_double);
  struct single_float later = va_arg(ap, struct single_float);
  complex_float singlecopy = va_arg(copy, complex_float);
  unsigned tailcopy = va_arg(copy, unsigned);
  complex_double widecopy = va_arg(copy, complex_double);
  struct single_float latercopy = va_arg(copy, struct single_float);
  va_end(copy);
  va_end(ap);
  return a + b + c + d + e + f + g + h == 36.0f && CHECK_COMPLEX(single) &&
         CHECK_COMPLEX(wide) && CHECK_COMPLEX(singlecopy) &&
         CHECK_COMPLEX(widecopy) && later.value == 9.0f &&
         latercopy.value == 9.0f && tail == TAIL && tailcopy == TAIL;
}
