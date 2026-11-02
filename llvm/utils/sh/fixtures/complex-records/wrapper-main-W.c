// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "wrappers.h"
#include <stdio.h>
#include <string.h>
unsigned result[256][8];
int main(void) {
  {
    struct W v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_W_0(0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct W v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_W_1(1.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct W v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_W_2(1.0f, 2.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct W v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_W_4(1.0f, 2.0f, 3.0f, 4.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct W v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_W_5(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct W v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_W_6(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 0, v, (struct SF){9.0f},
            0x31415926U);
  }
  {
    struct W v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_W_7(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 0, v, (struct SF){9.0f},
            0x31415926U);
  }
  {
    struct W v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_W_8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 0, v,
            (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct W v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_W_0(v, 0x31415926U);
  }
  {
    struct W v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_W_1(1.0f, v, 0x31415926U);
  }
  {
    struct W v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_W_6(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, v, 0x31415926U);
  }
  {
    struct W v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_W_7(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, v, 0x31415926U);
  }
  {
    struct W v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_W_8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, v, 0x31415926U);
  }
  {
    struct W v = return_W();
    union {
      float f;
      unsigned u;
    } a = {.f = __real__ v.z}, b = {.f = __imag__ v.z};
    result[13][0] = a.u;
    result[13][1] = b.u;
  }
  printf("var_W_0 %08x %08x %08x %08x %08x %08x\n", result[0][0], result[0][1],
         result[0][2], result[0][3], result[0][4], result[0][5]);
  printf("var_W_1 %08x %08x %08x %08x %08x %08x\n", result[1][0], result[1][1],
         result[1][2], result[1][3], result[1][4], result[1][5]);
  printf("var_W_2 %08x %08x %08x %08x %08x %08x\n", result[2][0], result[2][1],
         result[2][2], result[2][3], result[2][4], result[2][5]);
  printf("var_W_4 %08x %08x %08x %08x %08x %08x\n", result[3][0], result[3][1],
         result[3][2], result[3][3], result[3][4], result[3][5]);
  printf("var_W_5 %08x %08x %08x %08x %08x %08x\n", result[4][0], result[4][1],
         result[4][2], result[4][3], result[4][4], result[4][5]);
  printf("var_W_6 %08x %08x %08x %08x %08x %08x\n", result[5][0], result[5][1],
         result[5][2], result[5][3], result[5][4], result[5][5]);
  printf("var_W_7 %08x %08x %08x %08x %08x %08x\n", result[6][0], result[6][1],
         result[6][2], result[6][3], result[6][4], result[6][5]);
  printf("var_W_8 %08x %08x %08x %08x %08x %08x\n", result[7][0], result[7][1],
         result[7][2], result[7][3], result[7][4], result[7][5]);
  printf("named_W_0 %08x %08x %08x %08x %08x %08x\n", result[8][0],
         result[8][1], result[8][2], result[8][3], result[8][4], result[8][5]);
  printf("named_W_1 %08x %08x %08x %08x %08x %08x\n", result[9][0],
         result[9][1], result[9][2], result[9][3], result[9][4], result[9][5]);
  printf("named_W_6 %08x %08x %08x %08x %08x %08x\n", result[10][0],
         result[10][1], result[10][2], result[10][3], result[10][4],
         result[10][5]);
  printf("named_W_7 %08x %08x %08x %08x %08x %08x\n", result[11][0],
         result[11][1], result[11][2], result[11][3], result[11][4],
         result[11][5]);
  printf("named_W_8 %08x %08x %08x %08x %08x %08x\n", result[12][0],
         result[12][1], result[12][2], result[12][3], result[12][4],
         result[12][5]);
  printf("return_W %08x %08x %08x %08x %08x %08x\n", result[13][0],
         result[13][1], result[13][2], result[13][3], result[13][4],
         result[13][5]);
  return 0;
}
