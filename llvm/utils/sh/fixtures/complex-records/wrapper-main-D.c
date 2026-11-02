// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "wrappers.h"
#include <stdio.h>
#include <string.h>
unsigned result[256][8];
int main(void) {
  {
    struct D v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_D_0(0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct D v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_D_1(1.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct D v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_D_2(1.0f, 2.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct D v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_D_4(1.0f, 2.0f, 3.0f, 4.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct D v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_D_5(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct D v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_D_6(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 0, v, (struct SF){9.0f},
            0x31415926U);
  }
  {
    struct D v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_D_7(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 0, v, (struct SF){9.0f},
            0x31415926U);
  }
  {
    struct D v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_D_8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 0, v,
            (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct D v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_D_0(v, 0x31415926U);
  }
  {
    struct D v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_D_1(1.0f, v, 0x31415926U);
  }
  {
    struct D v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_D_6(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, v, 0x31415926U);
  }
  {
    struct D v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_D_7(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, v, 0x31415926U);
  }
  {
    struct D v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_D_8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, v, 0x31415926U);
  }
  {
    struct D v = return_D();
    union {
      float f;
      unsigned u;
    } a = {.f = __real__ v.z}, b = {.f = __imag__ v.z};
    result[41][0] = a.u;
    result[41][1] = b.u;
  }
  printf("var_D_0 %08x %08x %08x %08x %08x %08x\n", result[28][0],
         result[28][1], result[28][2], result[28][3], result[28][4],
         result[28][5]);
  printf("var_D_1 %08x %08x %08x %08x %08x %08x\n", result[29][0],
         result[29][1], result[29][2], result[29][3], result[29][4],
         result[29][5]);
  printf("var_D_2 %08x %08x %08x %08x %08x %08x\n", result[30][0],
         result[30][1], result[30][2], result[30][3], result[30][4],
         result[30][5]);
  printf("var_D_4 %08x %08x %08x %08x %08x %08x\n", result[31][0],
         result[31][1], result[31][2], result[31][3], result[31][4],
         result[31][5]);
  printf("var_D_5 %08x %08x %08x %08x %08x %08x\n", result[32][0],
         result[32][1], result[32][2], result[32][3], result[32][4],
         result[32][5]);
  printf("var_D_6 %08x %08x %08x %08x %08x %08x\n", result[33][0],
         result[33][1], result[33][2], result[33][3], result[33][4],
         result[33][5]);
  printf("var_D_7 %08x %08x %08x %08x %08x %08x\n", result[34][0],
         result[34][1], result[34][2], result[34][3], result[34][4],
         result[34][5]);
  printf("var_D_8 %08x %08x %08x %08x %08x %08x\n", result[35][0],
         result[35][1], result[35][2], result[35][3], result[35][4],
         result[35][5]);
  printf("named_D_0 %08x %08x %08x %08x %08x %08x\n", result[36][0],
         result[36][1], result[36][2], result[36][3], result[36][4],
         result[36][5]);
  printf("named_D_1 %08x %08x %08x %08x %08x %08x\n", result[37][0],
         result[37][1], result[37][2], result[37][3], result[37][4],
         result[37][5]);
  printf("named_D_6 %08x %08x %08x %08x %08x %08x\n", result[38][0],
         result[38][1], result[38][2], result[38][3], result[38][4],
         result[38][5]);
  printf("named_D_7 %08x %08x %08x %08x %08x %08x\n", result[39][0],
         result[39][1], result[39][2], result[39][3], result[39][4],
         result[39][5]);
  printf("named_D_8 %08x %08x %08x %08x %08x %08x\n", result[40][0],
         result[40][1], result[40][2], result[40][3], result[40][4],
         result[40][5]);
  printf("return_D %08x %08x %08x %08x %08x %08x\n", result[41][0],
         result[41][1], result[41][2], result[41][3], result[41][4],
         result[41][5]);
  return 0;
}
