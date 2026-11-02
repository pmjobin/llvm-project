// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "direct.h"
#include <stdio.h>
struct observation observations[9];
unsigned captured[9];
extern void capture_odd(float, int, ...);
extern void asm_call_odd(void);
int main(void) {
  CF z;
  __real__ z = 1.25f;
  __imag__ z = -3.5f;
  struct SF f = {9.0f};
  c0(0, z, f, 0x31415926U);
  c1(7.0f, 0, z, f, 0x31415926U);
  c2(7.0f, 8.0f, 0, z, f, 0x31415926U);
  c6(7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 0, z, f, 0x31415926U);
  c7(7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 0, z, f, 0x31415926U);
  c8(7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 0, z, f, 0x31415926U);
  capture_odd(7.0f, 0, z, f, 0x31415926U);
  for (unsigned k = 0; k < 9; k++) {
    if (k != 0 && k != 1 && k != 2 && k != 6 && k != 7 && k != 8)
      continue;
    struct observation *o = &observations[k];
    printf("n=%u real=%08x imag=%08x later=%08x tail=%08x named=%08x delta=%u "
           "save=",
           k, o->real, o->imag, o->later, o->tail, o->named, o->cursor_delta);
    for (unsigned j = 0; j < o->saved; j++)
      printf("%s%08x", j ? "," : "", o->save[j]);
    puts("");
  }
  printf("capture FR4..11/R5:");
  for (unsigned i = 0; i < 9; i++)
    printf(" %08x", captured[i]);
  puts("");
  asm_call_odd();
  printf("asm caller odd real=%08x imag=%08x later=%08x tail=%08x\n",
         observations[1].real, observations[1].imag, observations[1].later,
         observations[1].tail);
  return observations[1].real != 0x3fa00000U ||
         observations[1].imag != 0xc0600000U;
}
