// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "graphics.h"
#include <stdarg.h>
float graphics_dot(sh_float4 a, sh_float4 b) {
  return sh_fipr(a, b) + a[0] + b[3];
}
sh_float4 graphics_transform(const sh_matrix4 *m, sh_float4 v) {
  return sh_ftrv(m, v);
}
void graphics_batch(sh_float4 *d, const sh_float4 *s, const sh_matrix4 *m) {
  sh_ftrv4(d, s, m);
}
void graphics_unaligned(float *d, const float *s, const float *m) {
  sh_float4 v = sh_load4(s);
  sh_float4 r = __builtin_sh_ftrv(m, v);
  sh_store4(d, r);
}
sh_float4 graphics_pressure(const float *p, sh_float4 v) {
  sh_float4 a = sh_load4(p), b = sh_load4(p + 4), c = sh_load4(p + 8);
  sh_float4 d = sh_load4(p + 12), e = sh_load4(p + 16);
  sh_float4 f = sh_load4(p + 20), g = sh_load4(p + 24);
  graphics_barrier();
  float x = sh_fipr(a, v) + sh_fipr(b, v) + sh_fipr(c, v) + sh_fipr(d, v);
  float y = sh_fipr(e, v) + sh_fipr(f, v) + sh_fipr(g, v);
  return (sh_float4){x, y, sh_fipr(v, v), a[2] + g[1]};
}
sh_float4 graphics_index(sh_float4 v, unsigned i) {
  float x = v[i & 3];
  v[(i + 1) & 3] = x + 10;
  return v;
}
int graphics_abi(void) {
  sh_float4 a = graphics_gnu_v4(17, (sh_float4){1, 2, 3, 4}, 19);
  sh_float2 b = graphics_gnu_v2(23, 29, 31, (sh_float2){5, 6}, 37);
  sh_float4 c =
      graphics_gnu_varargs(7, (sh_float4){1, 2, 3, 4}, (sh_float2){5, 6}, 11U);
  return a[0] == 18 && a[1] == 21 && a[2] == 3 && a[3] == 4 && b[0] == 88 &&
         b[1] == 43 && c[0] == 6 && c[1] == 8 && c[2] == 10 && c[3] == 15;
}
float graphics_alias(sh_matrix4 *m, sh_float4 v) {
  sh_float4 a = sh_ftrv(m, v);
  m->column[0][0] += 1;
  sh_float4 b = sh_ftrv(m, v);
  return b[0] - a[0];
}
void graphics_snapshot(float *p, const sh_matrix4 *m) {
  sh_matrix4_store(p, m);
}
sh_float4 graphics_shuffle(sh_float4 a, sh_float4 b) {
  return a + __builtin_shufflevector(a, b, 7, 0, 5, 2);
}
void graphics_copy(float *d, const float *s) { sh_store4(d, sh_load4(s)); }
sh_float4 graphics_clang_varargs(unsigned tag, ...) {
  va_list ap;
  va_start(ap, tag);
  sh_float4 a = va_arg(ap, sh_float4);
  sh_float2 b = va_arg(ap, sh_float2);
  unsigned tail = va_arg(ap, unsigned);
  va_end(ap);
  return a + (sh_float4){b[0], b[1], tag, tail};
}
