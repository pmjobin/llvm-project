// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "wrappers.h"
#include <stdio.h>
#include <string.h>
unsigned result[256][8];
int main(void) {
  {
    struct P v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_P_0(0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct P v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_P_1(1.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct P v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_P_2(1.0f, 2.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct P v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_P_4(1.0f, 2.0f, 3.0f, 4.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct P v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_P_5(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct P v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_P_6(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 0, v, (struct SF){9.0f},
            0x31415926U);
  }
  {
    struct P v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_P_7(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 0, v, (struct SF){9.0f},
            0x31415926U);
  }
  {
    struct P v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_P_8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 0, v,
            (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct P v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_P_0(v, 0x31415926U);
  }
  {
    struct P v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_P_1(1.0f, v, 0x31415926U);
  }
  {
    struct P v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_P_6(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, v, 0x31415926U);
  }
  {
    struct P v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_P_7(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, v, 0x31415926U);
  }
  {
    struct P v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_P_8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, v, 0x31415926U);
  }
  {
    struct P v = return_P();
    union {
      float f;
      unsigned u;
    } a = {.f = __real__ v.z}, b = {.f = __imag__ v.z};
    result[69][0] = a.u;
    result[69][1] = b.u;
  }
  printf("var_P_0 %08x %08x %08x %08x %08x %08x\n", result[56][0],
         result[56][1], result[56][2], result[56][3], result[56][4],
         result[56][5]);
  printf("var_P_1 %08x %08x %08x %08x %08x %08x\n", result[57][0],
         result[57][1], result[57][2], result[57][3], result[57][4],
         result[57][5]);
  printf("var_P_2 %08x %08x %08x %08x %08x %08x\n", result[58][0],
         result[58][1], result[58][2], result[58][3], result[58][4],
         result[58][5]);
  printf("var_P_4 %08x %08x %08x %08x %08x %08x\n", result[59][0],
         result[59][1], result[59][2], result[59][3], result[59][4],
         result[59][5]);
  printf("var_P_5 %08x %08x %08x %08x %08x %08x\n", result[60][0],
         result[60][1], result[60][2], result[60][3], result[60][4],
         result[60][5]);
  printf("var_P_6 %08x %08x %08x %08x %08x %08x\n", result[61][0],
         result[61][1], result[61][2], result[61][3], result[61][4],
         result[61][5]);
  printf("var_P_7 %08x %08x %08x %08x %08x %08x\n", result[62][0],
         result[62][1], result[62][2], result[62][3], result[62][4],
         result[62][5]);
  printf("var_P_8 %08x %08x %08x %08x %08x %08x\n", result[63][0],
         result[63][1], result[63][2], result[63][3], result[63][4],
         result[63][5]);
  printf("named_P_0 %08x %08x %08x %08x %08x %08x\n", result[64][0],
         result[64][1], result[64][2], result[64][3], result[64][4],
         result[64][5]);
  printf("named_P_1 %08x %08x %08x %08x %08x %08x\n", result[65][0],
         result[65][1], result[65][2], result[65][3], result[65][4],
         result[65][5]);
  printf("named_P_6 %08x %08x %08x %08x %08x %08x\n", result[66][0],
         result[66][1], result[66][2], result[66][3], result[66][4],
         result[66][5]);
  printf("named_P_7 %08x %08x %08x %08x %08x %08x\n", result[67][0],
         result[67][1], result[67][2], result[67][3], result[67][4],
         result[67][5]);
  printf("named_P_8 %08x %08x %08x %08x %08x %08x\n", result[68][0],
         result[68][1], result[68][2], result[68][3], result[68][4],
         result[68][5]);
  printf("return_P %08x %08x %08x %08x %08x %08x\n", result[69][0],
         result[69][1], result[69][2], result[69][3], result[69][4],
         result[69][5]);
  return 0;
}
