// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "wrappers.h"
#include <stdio.h>
#include <string.h>
unsigned result[256][8];
int main(void) {
  {
    struct N v;
    __real__ v.w.z = 1.25;
    __imag__ v.w.z = -3.5;
    var_N_0(0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct N v;
    __real__ v.w.z = 1.25;
    __imag__ v.w.z = -3.5;
    var_N_1(1.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct N v;
    __real__ v.w.z = 1.25;
    __imag__ v.w.z = -3.5;
    var_N_2(1.0f, 2.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct N v;
    __real__ v.w.z = 1.25;
    __imag__ v.w.z = -3.5;
    var_N_4(1.0f, 2.0f, 3.0f, 4.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct N v;
    __real__ v.w.z = 1.25;
    __imag__ v.w.z = -3.5;
    var_N_5(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct N v;
    __real__ v.w.z = 1.25;
    __imag__ v.w.z = -3.5;
    var_N_6(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 0, v, (struct SF){9.0f},
            0x31415926U);
  }
  {
    struct N v;
    __real__ v.w.z = 1.25;
    __imag__ v.w.z = -3.5;
    var_N_7(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 0, v, (struct SF){9.0f},
            0x31415926U);
  }
  {
    struct N v;
    __real__ v.w.z = 1.25;
    __imag__ v.w.z = -3.5;
    var_N_8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 0, v,
            (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct N v;
    __real__ v.w.z = 1.25;
    __imag__ v.w.z = -3.5;
    named_N_0(v, 0x31415926U);
  }
  {
    struct N v;
    __real__ v.w.z = 1.25;
    __imag__ v.w.z = -3.5;
    named_N_1(1.0f, v, 0x31415926U);
  }
  {
    struct N v;
    __real__ v.w.z = 1.25;
    __imag__ v.w.z = -3.5;
    named_N_6(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, v, 0x31415926U);
  }
  {
    struct N v;
    __real__ v.w.z = 1.25;
    __imag__ v.w.z = -3.5;
    named_N_7(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, v, 0x31415926U);
  }
  {
    struct N v;
    __real__ v.w.z = 1.25;
    __imag__ v.w.z = -3.5;
    named_N_8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, v, 0x31415926U);
  }
  {
    struct N v = return_N();
    union {
      float f;
      unsigned u;
    } a = {.f = __real__ v.w.z}, b = {.f = __imag__ v.w.z};
    result[27][0] = a.u;
    result[27][1] = b.u;
  }
  printf("var_N_0 %08x %08x %08x %08x %08x %08x\n", result[14][0],
         result[14][1], result[14][2], result[14][3], result[14][4],
         result[14][5]);
  printf("var_N_1 %08x %08x %08x %08x %08x %08x\n", result[15][0],
         result[15][1], result[15][2], result[15][3], result[15][4],
         result[15][5]);
  printf("var_N_2 %08x %08x %08x %08x %08x %08x\n", result[16][0],
         result[16][1], result[16][2], result[16][3], result[16][4],
         result[16][5]);
  printf("var_N_4 %08x %08x %08x %08x %08x %08x\n", result[17][0],
         result[17][1], result[17][2], result[17][3], result[17][4],
         result[17][5]);
  printf("var_N_5 %08x %08x %08x %08x %08x %08x\n", result[18][0],
         result[18][1], result[18][2], result[18][3], result[18][4],
         result[18][5]);
  printf("var_N_6 %08x %08x %08x %08x %08x %08x\n", result[19][0],
         result[19][1], result[19][2], result[19][3], result[19][4],
         result[19][5]);
  printf("var_N_7 %08x %08x %08x %08x %08x %08x\n", result[20][0],
         result[20][1], result[20][2], result[20][3], result[20][4],
         result[20][5]);
  printf("var_N_8 %08x %08x %08x %08x %08x %08x\n", result[21][0],
         result[21][1], result[21][2], result[21][3], result[21][4],
         result[21][5]);
  printf("named_N_0 %08x %08x %08x %08x %08x %08x\n", result[22][0],
         result[22][1], result[22][2], result[22][3], result[22][4],
         result[22][5]);
  printf("named_N_1 %08x %08x %08x %08x %08x %08x\n", result[23][0],
         result[23][1], result[23][2], result[23][3], result[23][4],
         result[23][5]);
  printf("named_N_6 %08x %08x %08x %08x %08x %08x\n", result[24][0],
         result[24][1], result[24][2], result[24][3], result[24][4],
         result[24][5]);
  printf("named_N_7 %08x %08x %08x %08x %08x %08x\n", result[25][0],
         result[25][1], result[25][2], result[25][3], result[25][4],
         result[25][5]);
  printf("named_N_8 %08x %08x %08x %08x %08x %08x\n", result[26][0],
         result[26][1], result[26][2], result[26][3], result[26][4],
         result[26][5]);
  printf("return_N %08x %08x %08x %08x %08x %08x\n", result[27][0],
         result[27][1], result[27][2], result[27][3], result[27][4],
         result[27][5]);
  return 0;
}
