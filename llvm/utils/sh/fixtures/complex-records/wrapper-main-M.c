// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "wrappers.h"
#include <stdio.h>
#include <string.h>
unsigned result[256][8];
int main(void) {
  {
    struct M v;
    v.a = 1.25f;
    v.b = -3.5f;
    var_M_0(0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct M v;
    v.a = 1.25f;
    v.b = -3.5f;
    var_M_1(1.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct M v;
    v.a = 1.25f;
    v.b = -3.5f;
    var_M_2(1.0f, 2.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct M v;
    v.a = 1.25f;
    v.b = -3.5f;
    var_M_4(1.0f, 2.0f, 3.0f, 4.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct M v;
    v.a = 1.25f;
    v.b = -3.5f;
    var_M_5(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0, v, (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct M v;
    v.a = 1.25f;
    v.b = -3.5f;
    var_M_6(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 0, v, (struct SF){9.0f},
            0x31415926U);
  }
  {
    struct M v;
    v.a = 1.25f;
    v.b = -3.5f;
    var_M_7(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 0, v, (struct SF){9.0f},
            0x31415926U);
  }
  {
    struct M v;
    v.a = 1.25f;
    v.b = -3.5f;
    var_M_8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 0, v,
            (struct SF){9.0f}, 0x31415926U);
  }
  {
    struct M v;
    v.a = 1.25f;
    v.b = -3.5f;
    named_M_0(v, 0x31415926U);
  }
  {
    struct M v;
    v.a = 1.25f;
    v.b = -3.5f;
    named_M_1(1.0f, v, 0x31415926U);
  }
  {
    struct M v;
    v.a = 1.25f;
    v.b = -3.5f;
    named_M_6(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, v, 0x31415926U);
  }
  {
    struct M v;
    v.a = 1.25f;
    v.b = -3.5f;
    named_M_7(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, v, 0x31415926U);
  }
  {
    struct M v;
    v.a = 1.25f;
    v.b = -3.5f;
    named_M_8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, v, 0x31415926U);
  }
  {
    struct M v = return_M();
    union {
      float f;
      unsigned u;
    } a = {.f = v.a}, b = {.f = v.b};
    result[97][0] = a.u;
    result[97][1] = b.u;
  }
  printf("var_M_0 %08x %08x %08x %08x %08x %08x\n", result[84][0],
         result[84][1], result[84][2], result[84][3], result[84][4],
         result[84][5]);
  printf("var_M_1 %08x %08x %08x %08x %08x %08x\n", result[85][0],
         result[85][1], result[85][2], result[85][3], result[85][4],
         result[85][5]);
  printf("var_M_2 %08x %08x %08x %08x %08x %08x\n", result[86][0],
         result[86][1], result[86][2], result[86][3], result[86][4],
         result[86][5]);
  printf("var_M_4 %08x %08x %08x %08x %08x %08x\n", result[87][0],
         result[87][1], result[87][2], result[87][3], result[87][4],
         result[87][5]);
  printf("var_M_5 %08x %08x %08x %08x %08x %08x\n", result[88][0],
         result[88][1], result[88][2], result[88][3], result[88][4],
         result[88][5]);
  printf("var_M_6 %08x %08x %08x %08x %08x %08x\n", result[89][0],
         result[89][1], result[89][2], result[89][3], result[89][4],
         result[89][5]);
  printf("var_M_7 %08x %08x %08x %08x %08x %08x\n", result[90][0],
         result[90][1], result[90][2], result[90][3], result[90][4],
         result[90][5]);
  printf("var_M_8 %08x %08x %08x %08x %08x %08x\n", result[91][0],
         result[91][1], result[91][2], result[91][3], result[91][4],
         result[91][5]);
  printf("named_M_0 %08x %08x %08x %08x %08x %08x\n", result[92][0],
         result[92][1], result[92][2], result[92][3], result[92][4],
         result[92][5]);
  printf("named_M_1 %08x %08x %08x %08x %08x %08x\n", result[93][0],
         result[93][1], result[93][2], result[93][3], result[93][4],
         result[93][5]);
  printf("named_M_6 %08x %08x %08x %08x %08x %08x\n", result[94][0],
         result[94][1], result[94][2], result[94][3], result[94][4],
         result[94][5]);
  printf("named_M_7 %08x %08x %08x %08x %08x %08x\n", result[95][0],
         result[95][1], result[95][2], result[95][3], result[95][4],
         result[95][5]);
  printf("named_M_8 %08x %08x %08x %08x %08x %08x\n", result[96][0],
         result[96][1], result[96][2], result[96][3], result[96][4],
         result[96][5]);
  printf("return_M %08x %08x %08x %08x %08x %08x\n", result[97][0],
         result[97][1], result[97][2], result[97][3], result[97][4],
         result[97][5]);
  return 0;
}
