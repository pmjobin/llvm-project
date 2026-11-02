// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "graphics.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
void graphics_barrier(void) { __asm__ volatile("" ::: "memory"); }
sh_float4 graphics_gnu_v4(unsigned a, sh_float4 v, unsigned tail) {
  return v + (sh_float4){a, tail, 0, 0};
}
sh_float2 graphics_gnu_v2(unsigned a, unsigned b, unsigned c, sh_float2 v,
                          unsigned tail) {
  return v + (sh_float2){a + b + c, tail};
}
sh_float4 graphics_gnu_varargs(unsigned tag, ...) {
  va_list ap;
  va_start(ap, tag);
  sh_float4 a = va_arg(ap, sh_float4);
  sh_float2 b = va_arg(ap, sh_float2);
  unsigned tail = va_arg(ap, unsigned);
  va_end(ap);
  return a + (sh_float4){b[0], b[1], tag, tail};
}
static int equal(sh_float4 a, sh_float4 b) {
  for (int i = 0; i < 4; ++i)
    if (a[i] != b[i])
      return 0;
  return 1;
}
int main(void) {
  sh_matrix4 m = {
      {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}, {13, 14, 15, 16}}};
  sh_float4 v = {1, 2, 3, 4};
  sh_float4 want = {90, 100, 110, 120};
  if (graphics_dot(v, v) != 35)
    return 1;
  if (!equal(graphics_transform(&m, v), want))
    return 2;
  sh_float4 in[4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}},
            out[4];
  graphics_batch(out, in, &m);
  for (int i = 0; i < 4; ++i)
    if (!equal(out[i], m.column[i]))
      return 3;
  graphics_batch(in, in, &m);
  for (int i = 0; i < 4; ++i)
    if (!equal(in[i], m.column[i]))
      return 4;
  float src[5] __attribute__((aligned(8))), dst[5] __attribute__((aligned(8)));
  float mat[17] __attribute__((aligned(8)));
  memcpy(src + 1, &v, 16);
  memcpy(mat + 1, &m, 64);
  graphics_unaligned(dst + 1, src + 1, mat + 1);
  sh_float4 r;
  memcpy(&r, dst + 1, 16);
  if (!equal(r, want))
    return 5;
  float p[28];
  for (int i = 0; i < 28; ++i)
    p[i] = (float)(i + 1);
  if (!equal(graphics_pressure(p, v), (sh_float4){360, 690, 30, 29}))
    return 6;
  if (!graphics_abi())
    return 7;
  if (!equal(graphics_clang_varargs(7, (sh_float4){1, 2, 3, 4},
                                    (sh_float2){5, 6}, 11U),
             (sh_float4){6, 8, 10, 15}))
    return 13;
  for (unsigned i = 0; i < 4; ++i) {
    sh_float4 expected = v;
    expected[(i + 1) & 3] = v[i] + 10;
    if (!equal(graphics_index(v, i), expected))
      return 8;
  }
  if (graphics_alias(&m, v) != 1)
    return 9;
  float overlap[20] __attribute__((aligned(16)));
  memcpy(overlap, &m, 64);
  graphics_snapshot(overlap + 1, (const sh_matrix4 *)overlap);
  if (memcmp(overlap + 1, &m, 64))
    return 10;
  if (!equal(graphics_shuffle(v, (sh_float4){5, 6, 7, 8}),
             (sh_float4){9, 3, 9, 7}))
    return 11;
  const unsigned bits[4] = {0x80000000, 0x7fc12345, 0x7f812345, 0x12345678};
  for (int offset = 0; offset < 2; ++offset) {
    memcpy(src + offset, bits, 16);
    graphics_copy(dst + offset, src + offset);
    if (memcmp(dst + offset, bits, 16))
      return 12;
  }
  puts("graphics:PASS");
  return 0;
}
