// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

typedef float _Complex complex_float;
typedef double _Complex complex_double;
struct single_float {
  float value;
};

int complex_even(int, ...);
int complex_odd(float, int, ...);
int complex_double_odd(float, int, ...);
int complex_overflow(float, float, float, float, float, float, float, int, ...);
int complex_double_overflow(float, float, float, float, float, float, int, ...);
int complex_double_overflow_odd(float, float, float, float, float, int, ...);
int complex_double_last_fit(float, float, float, float, int, ...);
int complex_exhausted(float, float, float, float, float, float, float, float,
                      int, ...);
int complex_integer_overflow(int, int, int, int, ...);
