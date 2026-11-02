// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "wrappers.h"
#include <stdio.h>
#include <string.h>
unsigned result[256][8];
int main(void) {
  {
    struct ND v;
    __real__ v.d.z = 1.25;
    __imag__ v.d.z = -3.5;
    var_ND_0(0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct ND v;
    __real__ v.d.z = 1.25;
    __imag__ v.d.z = -3.5;
    var_ND_1(1.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct ND v;
    __real__ v.d.z = 1.25;
    __imag__ v.d.z = -3.5;
    var_ND_2(1.0f, 2.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct ND v;
    __real__ v.d.z = 1.25;
    __imag__ v.d.z = -3.5;
    var_ND_4(1.0f, 2.0f, 3.0f, 4.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct ND v;
    __real__ v.d.z = 1.25;
    __imag__ v.d.z = -3.5;
    var_ND_5(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0, v, (struct SF){9.0f},
             0x31415926U);
  }
  {
    struct ND v;
    __real__ v.d.z = 1.25;
    __imag__ v.d.z = -3.5;
    var_ND_6(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 0, v, (struct SF){9.0f},
             0x31415926U);
  }
  {
    struct ND v;
    __real__ v.d.z = 1.25;
    __imag__ v.d.z = -3.5;
    var_ND_7(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 0, v, (struct SF){9.0f},
             0x31415926U);
  }
  {
    struct ND v;
    __real__ v.d.z = 1.25;
    __imag__ v.d.z = -3.5;
    var_ND_8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 0, v,
             (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct ND v;
    __real__ v.d.z = 1.25;
    __imag__ v.d.z = -3.5;
    named_ND_0(v, 0x31415926U);
  }
  {
    struct ND v;
    __real__ v.d.z = 1.25;
    __imag__ v.d.z = -3.5;
    named_ND_1(1.0f, v, 0x31415926U);
  }
  {
    struct ND v;
    __real__ v.d.z = 1.25;
    __imag__ v.d.z = -3.5;
    named_ND_6(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, v, 0x31415926U);
  }
  {
    struct ND v;
    __real__ v.d.z = 1.25;
    __imag__ v.d.z = -3.5;
    named_ND_7(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, v, 0x31415926U);
  }
  {
    struct ND v;
    __real__ v.d.z = 1.25;
    __imag__ v.d.z = -3.5;
    named_ND_8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, v, 0x31415926U);
  }
  {
    struct ND v = return_ND();
    union {
      float f;
      unsigned u;
    } a = {.f = __real__ v.d.z}, b = {.f = __imag__ v.d.z};
    result[55][0] = a.u;
    result[55][1] = b.u;
  }
  printf("var_ND_0 %08x %08x %08x %08x %08x %08x\n", result[42][0],
         result[42][1], result[42][2], result[42][3], result[42][4],
         result[42][5]);
  printf("var_ND_1 %08x %08x %08x %08x %08x %08x\n", result[43][0],
         result[43][1], result[43][2], result[43][3], result[43][4],
         result[43][5]);
  printf("var_ND_2 %08x %08x %08x %08x %08x %08x\n", result[44][0],
         result[44][1], result[44][2], result[44][3], result[44][4],
         result[44][5]);
  printf("var_ND_4 %08x %08x %08x %08x %08x %08x\n", result[45][0],
         result[45][1], result[45][2], result[45][3], result[45][4],
         result[45][5]);
  printf("var_ND_5 %08x %08x %08x %08x %08x %08x\n", result[46][0],
         result[46][1], result[46][2], result[46][3], result[46][4],
         result[46][5]);
  printf("var_ND_6 %08x %08x %08x %08x %08x %08x\n", result[47][0],
         result[47][1], result[47][2], result[47][3], result[47][4],
         result[47][5]);
  printf("var_ND_7 %08x %08x %08x %08x %08x %08x\n", result[48][0],
         result[48][1], result[48][2], result[48][3], result[48][4],
         result[48][5]);
  printf("var_ND_8 %08x %08x %08x %08x %08x %08x\n", result[49][0],
         result[49][1], result[49][2], result[49][3], result[49][4],
         result[49][5]);
  printf("named_ND_0 %08x %08x %08x %08x %08x %08x\n", result[50][0],
         result[50][1], result[50][2], result[50][3], result[50][4],
         result[50][5]);
  printf("named_ND_1 %08x %08x %08x %08x %08x %08x\n", result[51][0],
         result[51][1], result[51][2], result[51][3], result[51][4],
         result[51][5]);
  printf("named_ND_6 %08x %08x %08x %08x %08x %08x\n", result[52][0],
         result[52][1], result[52][2], result[52][3], result[52][4],
         result[52][5]);
  printf("named_ND_7 %08x %08x %08x %08x %08x %08x\n", result[53][0],
         result[53][1], result[53][2], result[53][3], result[53][4],
         result[53][5]);
  printf("named_ND_8 %08x %08x %08x %08x %08x %08x\n", result[54][0],
         result[54][1], result[54][2], result[54][3], result[54][4],
         result[54][5]);
  printf("return_ND %08x %08x %08x %08x %08x %08x\n", result[55][0],
         result[55][1], result[55][2], result[55][3], result[55][4],
         result[55][5]);
  return 0;
}
