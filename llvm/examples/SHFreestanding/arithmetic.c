// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <stdint.h>

int64_t signed_divide(int64_t a, int64_t b) { return a / b; }
int64_t signed_remainder(int64_t a, int64_t b) { return a % b; }
uint64_t unsigned_divide(uint64_t a, uint64_t b) { return a / b; }
uint64_t unsigned_remainder(uint64_t a, uint64_t b) { return a % b; }
uint64_t multiply(uint64_t a, uint64_t b) { return a * b; }
uint64_t shift_left(uint64_t a, int n) { return a << n; }
uint64_t shift_right(uint64_t a, int n) { return a >> n; }
int64_t shift_signed(int64_t a, int n) { return a >> n; }
double signed_to_double(int64_t a) { return a; }
float unsigned_to_float(uint64_t a) { return a; }
int64_t double_to_signed(double a) { return a; }
uint64_t float_to_unsigned(float a) { return a; }
double add_double(double a, double b) { return a + b; }
double subtract_double(double a, double b) { return a - b; }
double multiply_double(double a, double b) { return a * b; }
double divide_double(double a, double b) { return a / b; }
float add_float(float a, float b) { return a + b; }
float divide_float(float a, float b) { return a / b; }

struct Big {
  int elements[67];
};
void copy_big(struct Big *a, const struct Big *b) { *a = *b; }

// Keep the cases different to exercise code-address jump tables, not only
// a table of return values. Callers supply the independently expected results.
volatile int switch_counter;
extern int archive_function(int);
int switch_test(unsigned n, int x) {
  switch (n) {
  case 0:
    return archive_function(x);
  case 1:
    switch_counter += x;
    return 17;
  case 2:
    return x * x;
  case 3:
    return x + 53;
  case 4:
    switch_counter -= x;
    return 23;
  case 5:
    return x ^ 97;
  case 6:
    return x - 11;
  case 7:
    return archive_function(x + 1);
  default:
    return -1;
  }
}
