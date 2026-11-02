// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
typedef float _Complex CF;
struct SF {
  float x;
};
struct observation {
  unsigned real, imag, later, tail, named;
  unsigned save[8];
  unsigned saved;
  unsigned cursor_delta;
};
extern struct observation observations[9];
extern unsigned captured[9];
void c0(int fixed, ...);
void c1(float f0, int fixed, ...);
void c2(float f0, float f1, int fixed, ...);
void c6(float f0, float f1, float f2, float f3, float f4, float f5, int fixed,
        ...);
void c7(float f0, float f1, float f2, float f3, float f4, float f5, float f6,
        int fixed, ...);
void c8(float f0, float f1, float f2, float f3, float f4, float f5, float f6,
        float f7, int fixed, ...);
