// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "wrappers.h"
#include <stdarg.h>
static unsigned bits(float x) {
  union {
    float f;
    unsigned u;
  } t = {.f = x};
  return t.u;
}
void var_W_0(int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct W v = va_arg(ap, struct W);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct W v2 = va_arg(copy, struct W);
  va_end(copy);
  va_end(ap);
  result[0][0] = bits(__real__ v.z);
  result[0][1] = bits(__imag__ v.z);
  result[0][2] = bits(__real__ v2.z);
  result[0][3] = bits(__imag__ v2.z);
  result[0][4] = bits(later.x);
  result[0][5] = tail;
}
void var_W_1(float f0, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct W v = va_arg(ap, struct W);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct W v2 = va_arg(copy, struct W);
  va_end(copy);
  va_end(ap);
  result[1][0] = bits(__real__ v.z);
  result[1][1] = bits(__imag__ v.z);
  result[1][2] = bits(__real__ v2.z);
  result[1][3] = bits(__imag__ v2.z);
  result[1][4] = bits(later.x);
  result[1][5] = tail;
}
void var_W_2(float f0, float f1, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct W v = va_arg(ap, struct W);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct W v2 = va_arg(copy, struct W);
  va_end(copy);
  va_end(ap);
  result[2][0] = bits(__real__ v.z);
  result[2][1] = bits(__imag__ v.z);
  result[2][2] = bits(__real__ v2.z);
  result[2][3] = bits(__imag__ v2.z);
  result[2][4] = bits(later.x);
  result[2][5] = tail;
}
void var_W_4(float f0, float f1, float f2, float f3, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct W v = va_arg(ap, struct W);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct W v2 = va_arg(copy, struct W);
  va_end(copy);
  va_end(ap);
  result[3][0] = bits(__real__ v.z);
  result[3][1] = bits(__imag__ v.z);
  result[3][2] = bits(__real__ v2.z);
  result[3][3] = bits(__imag__ v2.z);
  result[3][4] = bits(later.x);
  result[3][5] = tail;
}
void var_W_5(float f0, float f1, float f2, float f3, float f4, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct W v = va_arg(ap, struct W);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct W v2 = va_arg(copy, struct W);
  va_end(copy);
  va_end(ap);
  result[4][0] = bits(__real__ v.z);
  result[4][1] = bits(__imag__ v.z);
  result[4][2] = bits(__real__ v2.z);
  result[4][3] = bits(__imag__ v2.z);
  result[4][4] = bits(later.x);
  result[4][5] = tail;
}
void var_W_6(float f0, float f1, float f2, float f3, float f4, float f5,
             int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct W v = va_arg(ap, struct W);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct W v2 = va_arg(copy, struct W);
  va_end(copy);
  va_end(ap);
  result[5][0] = bits(__real__ v.z);
  result[5][1] = bits(__imag__ v.z);
  result[5][2] = bits(__real__ v2.z);
  result[5][3] = bits(__imag__ v2.z);
  result[5][4] = bits(later.x);
  result[5][5] = tail;
}
void var_W_7(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct W v = va_arg(ap, struct W);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct W v2 = va_arg(copy, struct W);
  va_end(copy);
  va_end(ap);
  result[6][0] = bits(__real__ v.z);
  result[6][1] = bits(__imag__ v.z);
  result[6][2] = bits(__real__ v2.z);
  result[6][3] = bits(__imag__ v2.z);
  result[6][4] = bits(later.x);
  result[6][5] = tail;
}
void var_W_8(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, float f7, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct W v = va_arg(ap, struct W);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct W v2 = va_arg(copy, struct W);
  va_end(copy);
  va_end(ap);
  result[7][0] = bits(__real__ v.z);
  result[7][1] = bits(__imag__ v.z);
  result[7][2] = bits(__real__ v2.z);
  result[7][3] = bits(__imag__ v2.z);
  result[7][4] = bits(later.x);
  result[7][5] = tail;
}
void named_W_0(struct W v, unsigned tail) {
  result[8][0] = bits(__real__ v.z);
  result[8][1] = bits(__imag__ v.z);
  result[8][5] = tail;
}
void named_W_1(float f0, struct W v, unsigned tail) {
  result[9][0] = bits(__real__ v.z);
  result[9][1] = bits(__imag__ v.z);
  result[9][5] = tail;
}
void named_W_6(float f0, float f1, float f2, float f3, float f4, float f5,
               struct W v, unsigned tail) {
  result[10][0] = bits(__real__ v.z);
  result[10][1] = bits(__imag__ v.z);
  result[10][5] = tail;
}
void named_W_7(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, struct W v, unsigned tail) {
  result[11][0] = bits(__real__ v.z);
  result[11][1] = bits(__imag__ v.z);
  result[11][5] = tail;
}
void named_W_8(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, float f7, struct W v, unsigned tail) {
  result[12][0] = bits(__real__ v.z);
  result[12][1] = bits(__imag__ v.z);
  result[12][5] = tail;
}
struct W return_W(void) {
  struct W v;
  __real__ v.z = 1.25;
  __imag__ v.z = -3.5;
  return v;
}
void var_N_0(int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct N v = va_arg(ap, struct N);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct N v2 = va_arg(copy, struct N);
  va_end(copy);
  va_end(ap);
  result[14][0] = bits(__real__ v.w.z);
  result[14][1] = bits(__imag__ v.w.z);
  result[14][2] = bits(__real__ v2.w.z);
  result[14][3] = bits(__imag__ v2.w.z);
  result[14][4] = bits(later.x);
  result[14][5] = tail;
}
void var_N_1(float f0, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct N v = va_arg(ap, struct N);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct N v2 = va_arg(copy, struct N);
  va_end(copy);
  va_end(ap);
  result[15][0] = bits(__real__ v.w.z);
  result[15][1] = bits(__imag__ v.w.z);
  result[15][2] = bits(__real__ v2.w.z);
  result[15][3] = bits(__imag__ v2.w.z);
  result[15][4] = bits(later.x);
  result[15][5] = tail;
}
void var_N_2(float f0, float f1, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct N v = va_arg(ap, struct N);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct N v2 = va_arg(copy, struct N);
  va_end(copy);
  va_end(ap);
  result[16][0] = bits(__real__ v.w.z);
  result[16][1] = bits(__imag__ v.w.z);
  result[16][2] = bits(__real__ v2.w.z);
  result[16][3] = bits(__imag__ v2.w.z);
  result[16][4] = bits(later.x);
  result[16][5] = tail;
}
void var_N_4(float f0, float f1, float f2, float f3, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct N v = va_arg(ap, struct N);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct N v2 = va_arg(copy, struct N);
  va_end(copy);
  va_end(ap);
  result[17][0] = bits(__real__ v.w.z);
  result[17][1] = bits(__imag__ v.w.z);
  result[17][2] = bits(__real__ v2.w.z);
  result[17][3] = bits(__imag__ v2.w.z);
  result[17][4] = bits(later.x);
  result[17][5] = tail;
}
void var_N_5(float f0, float f1, float f2, float f3, float f4, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct N v = va_arg(ap, struct N);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct N v2 = va_arg(copy, struct N);
  va_end(copy);
  va_end(ap);
  result[18][0] = bits(__real__ v.w.z);
  result[18][1] = bits(__imag__ v.w.z);
  result[18][2] = bits(__real__ v2.w.z);
  result[18][3] = bits(__imag__ v2.w.z);
  result[18][4] = bits(later.x);
  result[18][5] = tail;
}
void var_N_6(float f0, float f1, float f2, float f3, float f4, float f5,
             int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct N v = va_arg(ap, struct N);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct N v2 = va_arg(copy, struct N);
  va_end(copy);
  va_end(ap);
  result[19][0] = bits(__real__ v.w.z);
  result[19][1] = bits(__imag__ v.w.z);
  result[19][2] = bits(__real__ v2.w.z);
  result[19][3] = bits(__imag__ v2.w.z);
  result[19][4] = bits(later.x);
  result[19][5] = tail;
}
void var_N_7(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct N v = va_arg(ap, struct N);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct N v2 = va_arg(copy, struct N);
  va_end(copy);
  va_end(ap);
  result[20][0] = bits(__real__ v.w.z);
  result[20][1] = bits(__imag__ v.w.z);
  result[20][2] = bits(__real__ v2.w.z);
  result[20][3] = bits(__imag__ v2.w.z);
  result[20][4] = bits(later.x);
  result[20][5] = tail;
}
void var_N_8(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, float f7, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct N v = va_arg(ap, struct N);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct N v2 = va_arg(copy, struct N);
  va_end(copy);
  va_end(ap);
  result[21][0] = bits(__real__ v.w.z);
  result[21][1] = bits(__imag__ v.w.z);
  result[21][2] = bits(__real__ v2.w.z);
  result[21][3] = bits(__imag__ v2.w.z);
  result[21][4] = bits(later.x);
  result[21][5] = tail;
}
void named_N_0(struct N v, unsigned tail) {
  result[22][0] = bits(__real__ v.w.z);
  result[22][1] = bits(__imag__ v.w.z);
  result[22][5] = tail;
}
void named_N_1(float f0, struct N v, unsigned tail) {
  result[23][0] = bits(__real__ v.w.z);
  result[23][1] = bits(__imag__ v.w.z);
  result[23][5] = tail;
}
void named_N_6(float f0, float f1, float f2, float f3, float f4, float f5,
               struct N v, unsigned tail) {
  result[24][0] = bits(__real__ v.w.z);
  result[24][1] = bits(__imag__ v.w.z);
  result[24][5] = tail;
}
void named_N_7(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, struct N v, unsigned tail) {
  result[25][0] = bits(__real__ v.w.z);
  result[25][1] = bits(__imag__ v.w.z);
  result[25][5] = tail;
}
void named_N_8(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, float f7, struct N v, unsigned tail) {
  result[26][0] = bits(__real__ v.w.z);
  result[26][1] = bits(__imag__ v.w.z);
  result[26][5] = tail;
}
struct N return_N(void) {
  struct N v;
  __real__ v.w.z = 1.25;
  __imag__ v.w.z = -3.5;
  return v;
}
void var_D_0(int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct D v = va_arg(ap, struct D);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct D v2 = va_arg(copy, struct D);
  va_end(copy);
  va_end(ap);
  result[28][0] = bits(__real__ v.z);
  result[28][1] = bits(__imag__ v.z);
  result[28][2] = bits(__real__ v2.z);
  result[28][3] = bits(__imag__ v2.z);
  result[28][4] = bits(later.x);
  result[28][5] = tail;
}
void var_D_1(float f0, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct D v = va_arg(ap, struct D);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct D v2 = va_arg(copy, struct D);
  va_end(copy);
  va_end(ap);
  result[29][0] = bits(__real__ v.z);
  result[29][1] = bits(__imag__ v.z);
  result[29][2] = bits(__real__ v2.z);
  result[29][3] = bits(__imag__ v2.z);
  result[29][4] = bits(later.x);
  result[29][5] = tail;
}
void var_D_2(float f0, float f1, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct D v = va_arg(ap, struct D);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct D v2 = va_arg(copy, struct D);
  va_end(copy);
  va_end(ap);
  result[30][0] = bits(__real__ v.z);
  result[30][1] = bits(__imag__ v.z);
  result[30][2] = bits(__real__ v2.z);
  result[30][3] = bits(__imag__ v2.z);
  result[30][4] = bits(later.x);
  result[30][5] = tail;
}
void var_D_4(float f0, float f1, float f2, float f3, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct D v = va_arg(ap, struct D);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct D v2 = va_arg(copy, struct D);
  va_end(copy);
  va_end(ap);
  result[31][0] = bits(__real__ v.z);
  result[31][1] = bits(__imag__ v.z);
  result[31][2] = bits(__real__ v2.z);
  result[31][3] = bits(__imag__ v2.z);
  result[31][4] = bits(later.x);
  result[31][5] = tail;
}
void var_D_5(float f0, float f1, float f2, float f3, float f4, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct D v = va_arg(ap, struct D);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct D v2 = va_arg(copy, struct D);
  va_end(copy);
  va_end(ap);
  result[32][0] = bits(__real__ v.z);
  result[32][1] = bits(__imag__ v.z);
  result[32][2] = bits(__real__ v2.z);
  result[32][3] = bits(__imag__ v2.z);
  result[32][4] = bits(later.x);
  result[32][5] = tail;
}
void var_D_6(float f0, float f1, float f2, float f3, float f4, float f5,
             int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct D v = va_arg(ap, struct D);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct D v2 = va_arg(copy, struct D);
  va_end(copy);
  va_end(ap);
  result[33][0] = bits(__real__ v.z);
  result[33][1] = bits(__imag__ v.z);
  result[33][2] = bits(__real__ v2.z);
  result[33][3] = bits(__imag__ v2.z);
  result[33][4] = bits(later.x);
  result[33][5] = tail;
}
void var_D_7(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct D v = va_arg(ap, struct D);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct D v2 = va_arg(copy, struct D);
  va_end(copy);
  va_end(ap);
  result[34][0] = bits(__real__ v.z);
  result[34][1] = bits(__imag__ v.z);
  result[34][2] = bits(__real__ v2.z);
  result[34][3] = bits(__imag__ v2.z);
  result[34][4] = bits(later.x);
  result[34][5] = tail;
}
void var_D_8(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, float f7, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct D v = va_arg(ap, struct D);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct D v2 = va_arg(copy, struct D);
  va_end(copy);
  va_end(ap);
  result[35][0] = bits(__real__ v.z);
  result[35][1] = bits(__imag__ v.z);
  result[35][2] = bits(__real__ v2.z);
  result[35][3] = bits(__imag__ v2.z);
  result[35][4] = bits(later.x);
  result[35][5] = tail;
}
void named_D_0(struct D v, unsigned tail) {
  result[36][0] = bits(__real__ v.z);
  result[36][1] = bits(__imag__ v.z);
  result[36][5] = tail;
}
void named_D_1(float f0, struct D v, unsigned tail) {
  result[37][0] = bits(__real__ v.z);
  result[37][1] = bits(__imag__ v.z);
  result[37][5] = tail;
}
void named_D_6(float f0, float f1, float f2, float f3, float f4, float f5,
               struct D v, unsigned tail) {
  result[38][0] = bits(__real__ v.z);
  result[38][1] = bits(__imag__ v.z);
  result[38][5] = tail;
}
void named_D_7(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, struct D v, unsigned tail) {
  result[39][0] = bits(__real__ v.z);
  result[39][1] = bits(__imag__ v.z);
  result[39][5] = tail;
}
void named_D_8(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, float f7, struct D v, unsigned tail) {
  result[40][0] = bits(__real__ v.z);
  result[40][1] = bits(__imag__ v.z);
  result[40][5] = tail;
}
struct D return_D(void) {
  struct D v;
  __real__ v.z = 1.25;
  __imag__ v.z = -3.5;
  return v;
}
void var_ND_0(int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct ND v = va_arg(ap, struct ND);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct ND v2 = va_arg(copy, struct ND);
  va_end(copy);
  va_end(ap);
  result[42][0] = bits(__real__ v.d.z);
  result[42][1] = bits(__imag__ v.d.z);
  result[42][2] = bits(__real__ v2.d.z);
  result[42][3] = bits(__imag__ v2.d.z);
  result[42][4] = bits(later.x);
  result[42][5] = tail;
}
void var_ND_1(float f0, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct ND v = va_arg(ap, struct ND);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct ND v2 = va_arg(copy, struct ND);
  va_end(copy);
  va_end(ap);
  result[43][0] = bits(__real__ v.d.z);
  result[43][1] = bits(__imag__ v.d.z);
  result[43][2] = bits(__real__ v2.d.z);
  result[43][3] = bits(__imag__ v2.d.z);
  result[43][4] = bits(later.x);
  result[43][5] = tail;
}
void var_ND_2(float f0, float f1, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct ND v = va_arg(ap, struct ND);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct ND v2 = va_arg(copy, struct ND);
  va_end(copy);
  va_end(ap);
  result[44][0] = bits(__real__ v.d.z);
  result[44][1] = bits(__imag__ v.d.z);
  result[44][2] = bits(__real__ v2.d.z);
  result[44][3] = bits(__imag__ v2.d.z);
  result[44][4] = bits(later.x);
  result[44][5] = tail;
}
void var_ND_4(float f0, float f1, float f2, float f3, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct ND v = va_arg(ap, struct ND);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct ND v2 = va_arg(copy, struct ND);
  va_end(copy);
  va_end(ap);
  result[45][0] = bits(__real__ v.d.z);
  result[45][1] = bits(__imag__ v.d.z);
  result[45][2] = bits(__real__ v2.d.z);
  result[45][3] = bits(__imag__ v2.d.z);
  result[45][4] = bits(later.x);
  result[45][5] = tail;
}
void var_ND_5(float f0, float f1, float f2, float f3, float f4, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct ND v = va_arg(ap, struct ND);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct ND v2 = va_arg(copy, struct ND);
  va_end(copy);
  va_end(ap);
  result[46][0] = bits(__real__ v.d.z);
  result[46][1] = bits(__imag__ v.d.z);
  result[46][2] = bits(__real__ v2.d.z);
  result[46][3] = bits(__imag__ v2.d.z);
  result[46][4] = bits(later.x);
  result[46][5] = tail;
}
void var_ND_6(float f0, float f1, float f2, float f3, float f4, float f5,
              int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct ND v = va_arg(ap, struct ND);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct ND v2 = va_arg(copy, struct ND);
  va_end(copy);
  va_end(ap);
  result[47][0] = bits(__real__ v.d.z);
  result[47][1] = bits(__imag__ v.d.z);
  result[47][2] = bits(__real__ v2.d.z);
  result[47][3] = bits(__imag__ v2.d.z);
  result[47][4] = bits(later.x);
  result[47][5] = tail;
}
void var_ND_7(float f0, float f1, float f2, float f3, float f4, float f5,
              float f6, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct ND v = va_arg(ap, struct ND);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct ND v2 = va_arg(copy, struct ND);
  va_end(copy);
  va_end(ap);
  result[48][0] = bits(__real__ v.d.z);
  result[48][1] = bits(__imag__ v.d.z);
  result[48][2] = bits(__real__ v2.d.z);
  result[48][3] = bits(__imag__ v2.d.z);
  result[48][4] = bits(later.x);
  result[48][5] = tail;
}
void var_ND_8(float f0, float f1, float f2, float f3, float f4, float f5,
              float f6, float f7, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct ND v = va_arg(ap, struct ND);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct ND v2 = va_arg(copy, struct ND);
  va_end(copy);
  va_end(ap);
  result[49][0] = bits(__real__ v.d.z);
  result[49][1] = bits(__imag__ v.d.z);
  result[49][2] = bits(__real__ v2.d.z);
  result[49][3] = bits(__imag__ v2.d.z);
  result[49][4] = bits(later.x);
  result[49][5] = tail;
}
void named_ND_0(struct ND v, unsigned tail) {
  result[50][0] = bits(__real__ v.d.z);
  result[50][1] = bits(__imag__ v.d.z);
  result[50][5] = tail;
}
void named_ND_1(float f0, struct ND v, unsigned tail) {
  result[51][0] = bits(__real__ v.d.z);
  result[51][1] = bits(__imag__ v.d.z);
  result[51][5] = tail;
}
void named_ND_6(float f0, float f1, float f2, float f3, float f4, float f5,
                struct ND v, unsigned tail) {
  result[52][0] = bits(__real__ v.d.z);
  result[52][1] = bits(__imag__ v.d.z);
  result[52][5] = tail;
}
void named_ND_7(float f0, float f1, float f2, float f3, float f4, float f5,
                float f6, struct ND v, unsigned tail) {
  result[53][0] = bits(__real__ v.d.z);
  result[53][1] = bits(__imag__ v.d.z);
  result[53][5] = tail;
}
void named_ND_8(float f0, float f1, float f2, float f3, float f4, float f5,
                float f6, float f7, struct ND v, unsigned tail) {
  result[54][0] = bits(__real__ v.d.z);
  result[54][1] = bits(__imag__ v.d.z);
  result[54][5] = tail;
}
struct ND return_ND(void) {
  struct ND v;
  __real__ v.d.z = 1.25;
  __imag__ v.d.z = -3.5;
  return v;
}
void var_P_0(int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct P v = va_arg(ap, struct P);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct P v2 = va_arg(copy, struct P);
  va_end(copy);
  va_end(ap);
  result[56][0] = bits(__real__ v.z);
  result[56][1] = bits(__imag__ v.z);
  result[56][2] = bits(__real__ v2.z);
  result[56][3] = bits(__imag__ v2.z);
  result[56][4] = bits(later.x);
  result[56][5] = tail;
}
void var_P_1(float f0, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct P v = va_arg(ap, struct P);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct P v2 = va_arg(copy, struct P);
  va_end(copy);
  va_end(ap);
  result[57][0] = bits(__real__ v.z);
  result[57][1] = bits(__imag__ v.z);
  result[57][2] = bits(__real__ v2.z);
  result[57][3] = bits(__imag__ v2.z);
  result[57][4] = bits(later.x);
  result[57][5] = tail;
}
void var_P_2(float f0, float f1, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct P v = va_arg(ap, struct P);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct P v2 = va_arg(copy, struct P);
  va_end(copy);
  va_end(ap);
  result[58][0] = bits(__real__ v.z);
  result[58][1] = bits(__imag__ v.z);
  result[58][2] = bits(__real__ v2.z);
  result[58][3] = bits(__imag__ v2.z);
  result[58][4] = bits(later.x);
  result[58][5] = tail;
}
void var_P_4(float f0, float f1, float f2, float f3, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct P v = va_arg(ap, struct P);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct P v2 = va_arg(copy, struct P);
  va_end(copy);
  va_end(ap);
  result[59][0] = bits(__real__ v.z);
  result[59][1] = bits(__imag__ v.z);
  result[59][2] = bits(__real__ v2.z);
  result[59][3] = bits(__imag__ v2.z);
  result[59][4] = bits(later.x);
  result[59][5] = tail;
}
void var_P_5(float f0, float f1, float f2, float f3, float f4, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct P v = va_arg(ap, struct P);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct P v2 = va_arg(copy, struct P);
  va_end(copy);
  va_end(ap);
  result[60][0] = bits(__real__ v.z);
  result[60][1] = bits(__imag__ v.z);
  result[60][2] = bits(__real__ v2.z);
  result[60][3] = bits(__imag__ v2.z);
  result[60][4] = bits(later.x);
  result[60][5] = tail;
}
void var_P_6(float f0, float f1, float f2, float f3, float f4, float f5,
             int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct P v = va_arg(ap, struct P);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct P v2 = va_arg(copy, struct P);
  va_end(copy);
  va_end(ap);
  result[61][0] = bits(__real__ v.z);
  result[61][1] = bits(__imag__ v.z);
  result[61][2] = bits(__real__ v2.z);
  result[61][3] = bits(__imag__ v2.z);
  result[61][4] = bits(later.x);
  result[61][5] = tail;
}
void var_P_7(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct P v = va_arg(ap, struct P);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct P v2 = va_arg(copy, struct P);
  va_end(copy);
  va_end(ap);
  result[62][0] = bits(__real__ v.z);
  result[62][1] = bits(__imag__ v.z);
  result[62][2] = bits(__real__ v2.z);
  result[62][3] = bits(__imag__ v2.z);
  result[62][4] = bits(later.x);
  result[62][5] = tail;
}
void var_P_8(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, float f7, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct P v = va_arg(ap, struct P);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct P v2 = va_arg(copy, struct P);
  va_end(copy);
  va_end(ap);
  result[63][0] = bits(__real__ v.z);
  result[63][1] = bits(__imag__ v.z);
  result[63][2] = bits(__real__ v2.z);
  result[63][3] = bits(__imag__ v2.z);
  result[63][4] = bits(later.x);
  result[63][5] = tail;
}
void named_P_0(struct P v, unsigned tail) {
  result[64][0] = bits(__real__ v.z);
  result[64][1] = bits(__imag__ v.z);
  result[64][5] = tail;
}
void named_P_1(float f0, struct P v, unsigned tail) {
  result[65][0] = bits(__real__ v.z);
  result[65][1] = bits(__imag__ v.z);
  result[65][5] = tail;
}
void named_P_6(float f0, float f1, float f2, float f3, float f4, float f5,
               struct P v, unsigned tail) {
  result[66][0] = bits(__real__ v.z);
  result[66][1] = bits(__imag__ v.z);
  result[66][5] = tail;
}
void named_P_7(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, struct P v, unsigned tail) {
  result[67][0] = bits(__real__ v.z);
  result[67][1] = bits(__imag__ v.z);
  result[67][5] = tail;
}
void named_P_8(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, float f7, struct P v, unsigned tail) {
  result[68][0] = bits(__real__ v.z);
  result[68][1] = bits(__imag__ v.z);
  result[68][5] = tail;
}
struct P return_P(void) {
  struct P v;
  __real__ v.z = 1.25;
  __imag__ v.z = -3.5;
  return v;
}
void var_H_0(int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct H v = va_arg(ap, struct H);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct H v2 = va_arg(copy, struct H);
  va_end(copy);
  va_end(ap);
  result[70][0] = bits(__real__ v.z);
  result[70][1] = bits(__imag__ v.z);
  result[70][2] = bits(__real__ v2.z);
  result[70][3] = bits(__imag__ v2.z);
  result[70][4] = bits(later.x);
  result[70][5] = tail;
}
void var_H_1(float f0, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct H v = va_arg(ap, struct H);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct H v2 = va_arg(copy, struct H);
  va_end(copy);
  va_end(ap);
  result[71][0] = bits(__real__ v.z);
  result[71][1] = bits(__imag__ v.z);
  result[71][2] = bits(__real__ v2.z);
  result[71][3] = bits(__imag__ v2.z);
  result[71][4] = bits(later.x);
  result[71][5] = tail;
}
void var_H_2(float f0, float f1, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct H v = va_arg(ap, struct H);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct H v2 = va_arg(copy, struct H);
  va_end(copy);
  va_end(ap);
  result[72][0] = bits(__real__ v.z);
  result[72][1] = bits(__imag__ v.z);
  result[72][2] = bits(__real__ v2.z);
  result[72][3] = bits(__imag__ v2.z);
  result[72][4] = bits(later.x);
  result[72][5] = tail;
}
void var_H_4(float f0, float f1, float f2, float f3, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct H v = va_arg(ap, struct H);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct H v2 = va_arg(copy, struct H);
  va_end(copy);
  va_end(ap);
  result[73][0] = bits(__real__ v.z);
  result[73][1] = bits(__imag__ v.z);
  result[73][2] = bits(__real__ v2.z);
  result[73][3] = bits(__imag__ v2.z);
  result[73][4] = bits(later.x);
  result[73][5] = tail;
}
void var_H_5(float f0, float f1, float f2, float f3, float f4, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct H v = va_arg(ap, struct H);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct H v2 = va_arg(copy, struct H);
  va_end(copy);
  va_end(ap);
  result[74][0] = bits(__real__ v.z);
  result[74][1] = bits(__imag__ v.z);
  result[74][2] = bits(__real__ v2.z);
  result[74][3] = bits(__imag__ v2.z);
  result[74][4] = bits(later.x);
  result[74][5] = tail;
}
void var_H_6(float f0, float f1, float f2, float f3, float f4, float f5,
             int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct H v = va_arg(ap, struct H);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct H v2 = va_arg(copy, struct H);
  va_end(copy);
  va_end(ap);
  result[75][0] = bits(__real__ v.z);
  result[75][1] = bits(__imag__ v.z);
  result[75][2] = bits(__real__ v2.z);
  result[75][3] = bits(__imag__ v2.z);
  result[75][4] = bits(later.x);
  result[75][5] = tail;
}
void var_H_7(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct H v = va_arg(ap, struct H);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct H v2 = va_arg(copy, struct H);
  va_end(copy);
  va_end(ap);
  result[76][0] = bits(__real__ v.z);
  result[76][1] = bits(__imag__ v.z);
  result[76][2] = bits(__real__ v2.z);
  result[76][3] = bits(__imag__ v2.z);
  result[76][4] = bits(later.x);
  result[76][5] = tail;
}
void var_H_8(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, float f7, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct H v = va_arg(ap, struct H);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct H v2 = va_arg(copy, struct H);
  va_end(copy);
  va_end(ap);
  result[77][0] = bits(__real__ v.z);
  result[77][1] = bits(__imag__ v.z);
  result[77][2] = bits(__real__ v2.z);
  result[77][3] = bits(__imag__ v2.z);
  result[77][4] = bits(later.x);
  result[77][5] = tail;
}
void named_H_0(struct H v, unsigned tail) {
  result[78][0] = bits(__real__ v.z);
  result[78][1] = bits(__imag__ v.z);
  result[78][5] = tail;
}
void named_H_1(float f0, struct H v, unsigned tail) {
  result[79][0] = bits(__real__ v.z);
  result[79][1] = bits(__imag__ v.z);
  result[79][5] = tail;
}
void named_H_6(float f0, float f1, float f2, float f3, float f4, float f5,
               struct H v, unsigned tail) {
  result[80][0] = bits(__real__ v.z);
  result[80][1] = bits(__imag__ v.z);
  result[80][5] = tail;
}
void named_H_7(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, struct H v, unsigned tail) {
  result[81][0] = bits(__real__ v.z);
  result[81][1] = bits(__imag__ v.z);
  result[81][5] = tail;
}
void named_H_8(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, float f7, struct H v, unsigned tail) {
  result[82][0] = bits(__real__ v.z);
  result[82][1] = bits(__imag__ v.z);
  result[82][5] = tail;
}
struct H return_H(void) {
  struct H v;
  __real__ v.z = 1.25;
  __imag__ v.z = -3.5;
  return v;
}
void var_M_0(int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct M v = va_arg(ap, struct M);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct M v2 = va_arg(copy, struct M);
  va_end(copy);
  va_end(ap);
  result[84][0] = bits(v.a);
  result[84][1] = bits(v.b);
  result[84][2] = bits(v2.a);
  result[84][3] = bits(v2.b);
  result[84][4] = bits(later.x);
  result[84][5] = tail;
}
void var_M_1(float f0, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct M v = va_arg(ap, struct M);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct M v2 = va_arg(copy, struct M);
  va_end(copy);
  va_end(ap);
  result[85][0] = bits(v.a);
  result[85][1] = bits(v.b);
  result[85][2] = bits(v2.a);
  result[85][3] = bits(v2.b);
  result[85][4] = bits(later.x);
  result[85][5] = tail;
}
void var_M_2(float f0, float f1, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct M v = va_arg(ap, struct M);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct M v2 = va_arg(copy, struct M);
  va_end(copy);
  va_end(ap);
  result[86][0] = bits(v.a);
  result[86][1] = bits(v.b);
  result[86][2] = bits(v2.a);
  result[86][3] = bits(v2.b);
  result[86][4] = bits(later.x);
  result[86][5] = tail;
}
void var_M_4(float f0, float f1, float f2, float f3, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct M v = va_arg(ap, struct M);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct M v2 = va_arg(copy, struct M);
  va_end(copy);
  va_end(ap);
  result[87][0] = bits(v.a);
  result[87][1] = bits(v.b);
  result[87][2] = bits(v2.a);
  result[87][3] = bits(v2.b);
  result[87][4] = bits(later.x);
  result[87][5] = tail;
}
void var_M_5(float f0, float f1, float f2, float f3, float f4, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct M v = va_arg(ap, struct M);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct M v2 = va_arg(copy, struct M);
  va_end(copy);
  va_end(ap);
  result[88][0] = bits(v.a);
  result[88][1] = bits(v.b);
  result[88][2] = bits(v2.a);
  result[88][3] = bits(v2.b);
  result[88][4] = bits(later.x);
  result[88][5] = tail;
}
void var_M_6(float f0, float f1, float f2, float f3, float f4, float f5,
             int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct M v = va_arg(ap, struct M);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct M v2 = va_arg(copy, struct M);
  va_end(copy);
  va_end(ap);
  result[89][0] = bits(v.a);
  result[89][1] = bits(v.b);
  result[89][2] = bits(v2.a);
  result[89][3] = bits(v2.b);
  result[89][4] = bits(later.x);
  result[89][5] = tail;
}
void var_M_7(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct M v = va_arg(ap, struct M);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct M v2 = va_arg(copy, struct M);
  va_end(copy);
  va_end(ap);
  result[90][0] = bits(v.a);
  result[90][1] = bits(v.b);
  result[90][2] = bits(v2.a);
  result[90][3] = bits(v2.b);
  result[90][4] = bits(later.x);
  result[90][5] = tail;
}
void var_M_8(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, float f7, int tag, ...) {
  va_list ap, copy;
  va_start(ap, tag);
  va_copy(copy, ap);
  struct M v = va_arg(ap, struct M);
  struct SF later = va_arg(ap, struct SF);
  unsigned tail = va_arg(ap, unsigned);
  struct M v2 = va_arg(copy, struct M);
  va_end(copy);
  va_end(ap);
  result[91][0] = bits(v.a);
  result[91][1] = bits(v.b);
  result[91][2] = bits(v2.a);
  result[91][3] = bits(v2.b);
  result[91][4] = bits(later.x);
  result[91][5] = tail;
}
void named_M_0(struct M v, unsigned tail) {
  result[92][0] = bits(v.a);
  result[92][1] = bits(v.b);
  result[92][5] = tail;
}
void named_M_1(float f0, struct M v, unsigned tail) {
  result[93][0] = bits(v.a);
  result[93][1] = bits(v.b);
  result[93][5] = tail;
}
void named_M_6(float f0, float f1, float f2, float f3, float f4, float f5,
               struct M v, unsigned tail) {
  result[94][0] = bits(v.a);
  result[94][1] = bits(v.b);
  result[94][5] = tail;
}
void named_M_7(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, struct M v, unsigned tail) {
  result[95][0] = bits(v.a);
  result[95][1] = bits(v.b);
  result[95][5] = tail;
}
void named_M_8(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, float f7, struct M v, unsigned tail) {
  result[96][0] = bits(v.a);
  result[96][1] = bits(v.b);
  result[96][5] = tail;
}
struct M return_M(void) {
  struct M v;
  v.a = 1.25f;
  v.b = -3.5f;
  return v;
}
