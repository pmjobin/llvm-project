// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "direct.h"
#include <stdarg.h>
#include <string.h>
static unsigned bits(float x) {
  union {
    float f;
    unsigned u;
  } t = {.f = x};
  return t.u;
}
void c0(int fixed, ...) {
  va_list ap;
  va_start(ap, fixed);
  void *v[5];
  memcpy(v, &ap, sizeof(v));
  struct observation *o = &observations[0];
  o->saved = ((char *)v[3] - (char *)v[2]) / 4;
  for (unsigned i = 0; i < o->saved; i++)
    memcpy(&o->save[i], (char *)v[2] + 4 * i, 4);
  CF x = va_arg(ap, CF);
  struct SF f = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  o->real = bits(__real__ x);
  o->imag = bits(__imag__ x);
  o->later = bits(f.x);
  o->tail = tail;
  o->named = bits(0.0f);
  void *after[5];
  memcpy(after, &ap, sizeof(after));
  o->cursor_delta = (char *)after[2] - (char *)v[2];
  va_end(ap);
}
void c1(float f0, int fixed, ...) {
  va_list ap;
  va_start(ap, fixed);
  void *v[5];
  memcpy(v, &ap, sizeof(v));
  struct observation *o = &observations[1];
  o->saved = ((char *)v[3] - (char *)v[2]) / 4;
  for (unsigned i = 0; i < o->saved; i++)
    memcpy(&o->save[i], (char *)v[2] + 4 * i, 4);
  CF x = va_arg(ap, CF);
  struct SF f = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  o->real = bits(__real__ x);
  o->imag = bits(__imag__ x);
  o->later = bits(f.x);
  o->tail = tail;
  o->named = bits(f0);
  void *after[5];
  memcpy(after, &ap, sizeof(after));
  o->cursor_delta = (char *)after[2] - (char *)v[2];
  va_end(ap);
}
void c2(float f0, float f1, int fixed, ...) {
  va_list ap;
  va_start(ap, fixed);
  void *v[5];
  memcpy(v, &ap, sizeof(v));
  struct observation *o = &observations[2];
  o->saved = ((char *)v[3] - (char *)v[2]) / 4;
  for (unsigned i = 0; i < o->saved; i++)
    memcpy(&o->save[i], (char *)v[2] + 4 * i, 4);
  CF x = va_arg(ap, CF);
  struct SF f = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  o->real = bits(__real__ x);
  o->imag = bits(__imag__ x);
  o->later = bits(f.x);
  o->tail = tail;
  o->named = bits(f0);
  void *after[5];
  memcpy(after, &ap, sizeof(after));
  o->cursor_delta = (char *)after[2] - (char *)v[2];
  va_end(ap);
}
void c6(float f0, float f1, float f2, float f3, float f4, float f5, int fixed,
        ...) {
  va_list ap;
  va_start(ap, fixed);
  void *v[5];
  memcpy(v, &ap, sizeof(v));
  struct observation *o = &observations[6];
  o->saved = ((char *)v[3] - (char *)v[2]) / 4;
  for (unsigned i = 0; i < o->saved; i++)
    memcpy(&o->save[i], (char *)v[2] + 4 * i, 4);
  CF x = va_arg(ap, CF);
  struct SF f = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  o->real = bits(__real__ x);
  o->imag = bits(__imag__ x);
  o->later = bits(f.x);
  o->tail = tail;
  o->named = bits(f0);
  void *after[5];
  memcpy(after, &ap, sizeof(after));
  o->cursor_delta = (char *)after[2] - (char *)v[2];
  va_end(ap);
}
void c7(float f0, float f1, float f2, float f3, float f4, float f5, float f6,
        int fixed, ...) {
  va_list ap;
  va_start(ap, fixed);
  void *v[5];
  memcpy(v, &ap, sizeof(v));
  struct observation *o = &observations[7];
  o->saved = ((char *)v[3] - (char *)v[2]) / 4;
  for (unsigned i = 0; i < o->saved; i++)
    memcpy(&o->save[i], (char *)v[2] + 4 * i, 4);
  CF x = va_arg(ap, CF);
  struct SF f = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  o->real = bits(__real__ x);
  o->imag = bits(__imag__ x);
  o->later = bits(f.x);
  o->tail = tail;
  o->named = bits(f0);
  void *after[5];
  memcpy(after, &ap, sizeof(after));
  o->cursor_delta = (char *)after[2] - (char *)v[2];
  va_end(ap);
}
void c8(float f0, float f1, float f2, float f3, float f4, float f5, float f6,
        float f7, int fixed, ...) {
  va_list ap;
  va_start(ap, fixed);
  void *v[5];
  memcpy(v, &ap, sizeof(v));
  struct observation *o = &observations[8];
  o->saved = ((char *)v[3] - (char *)v[2]) / 4;
  for (unsigned i = 0; i < o->saved; i++)
    memcpy(&o->save[i], (char *)v[2] + 4 * i, 4);
  CF x = va_arg(ap, CF);
  struct SF f = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  o->real = bits(__real__ x);
  o->imag = bits(__imag__ x);
  o->later = bits(f.x);
  o->tail = tail;
  o->named = bits(f0);
  void *after[5];
  memcpy(after, &ap, sizeof(after));
  o->cursor_delta = (char *)after[2] - (char *)v[2];
  va_end(ap);
}
