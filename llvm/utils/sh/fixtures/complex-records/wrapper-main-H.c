// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "wrappers.h"
#include <stdio.h>
#include <string.h>
unsigned result[256][8];
int main(void) {
  {
    struct H v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_H_0(0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct H v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_H_1(1.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct H v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_H_2(1.0f, 2.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct H v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_H_4(1.0f, 2.0f, 3.0f, 4.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct H v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_H_5(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct H v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_H_6(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 0, v, (struct SF){9.0f},
            0x31415926U);
  }
  {
    struct H v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_H_7(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 0, v, (struct SF){9.0f},
            0x31415926U);
  }
  {
    struct H v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    var_H_8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 0, v,
            (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct H v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_H_0(v, 0x31415926U);
  }
  {
    struct H v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_H_1(1.0f, v, 0x31415926U);
  }
  {
    struct H v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_H_6(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, v, 0x31415926U);
  }
  {
    struct H v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_H_7(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, v, 0x31415926U);
  }
  {
    struct H v;
    __real__ v.z = 1.25;
    __imag__ v.z = -3.5;
    named_H_8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, v, 0x31415926U);
  }
  {
    struct H v = return_H();
    union {
      float f;
      unsigned u;
    } a = {.f = __real__ v.z}, b = {.f = __imag__ v.z};
    result[83][0] = a.u;
    result[83][1] = b.u;
  }
  printf("var_H_0 %08x %08x %08x %08x %08x %08x\n", result[70][0],
         result[70][1], result[70][2], result[70][3], result[70][4],
         result[70][5]);
  printf("var_H_1 %08x %08x %08x %08x %08x %08x\n", result[71][0],
         result[71][1], result[71][2], result[71][3], result[71][4],
         result[71][5]);
  printf("var_H_2 %08x %08x %08x %08x %08x %08x\n", result[72][0],
         result[72][1], result[72][2], result[72][3], result[72][4],
         result[72][5]);
  printf("var_H_4 %08x %08x %08x %08x %08x %08x\n", result[73][0],
         result[73][1], result[73][2], result[73][3], result[73][4],
         result[73][5]);
  printf("var_H_5 %08x %08x %08x %08x %08x %08x\n", result[74][0],
         result[74][1], result[74][2], result[74][3], result[74][4],
         result[74][5]);
  printf("var_H_6 %08x %08x %08x %08x %08x %08x\n", result[75][0],
         result[75][1], result[75][2], result[75][3], result[75][4],
         result[75][5]);
  printf("var_H_7 %08x %08x %08x %08x %08x %08x\n", result[76][0],
         result[76][1], result[76][2], result[76][3], result[76][4],
         result[76][5]);
  printf("var_H_8 %08x %08x %08x %08x %08x %08x\n", result[77][0],
         result[77][1], result[77][2], result[77][3], result[77][4],
         result[77][5]);
  printf("named_H_0 %08x %08x %08x %08x %08x %08x\n", result[78][0],
         result[78][1], result[78][2], result[78][3], result[78][4],
         result[78][5]);
  printf("named_H_1 %08x %08x %08x %08x %08x %08x\n", result[79][0],
         result[79][1], result[79][2], result[79][3], result[79][4],
         result[79][5]);
  printf("named_H_6 %08x %08x %08x %08x %08x %08x\n", result[80][0],
         result[80][1], result[80][2], result[80][3], result[80][4],
         result[80][5]);
  printf("named_H_7 %08x %08x %08x %08x %08x %08x\n", result[81][0],
         result[81][1], result[81][2], result[81][3], result[81][4],
         result[81][5]);
  printf("named_H_8 %08x %08x %08x %08x %08x %08x\n", result[82][0],
         result[82][1], result[82][2], result[82][3], result[82][4],
         result[82][5]);
  printf("return_H %08x %08x %08x %08x %08x %08x\n", result[83][0],
         result[83][1], result[83][2], result[83][3], result[83][4],
         result[83][5]);
  return 0;
}
