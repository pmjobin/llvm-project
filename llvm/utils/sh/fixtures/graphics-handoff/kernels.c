/* Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */
#include <shintrin.h>

float dot4(const sh_float4 *a, const sh_float4 *b) { return sh_fipr(*a, *b); }

void dot4_sequence(sh_float4 *out, const sh_float4 a[4], const sh_float4 *b) {
  const sh_float4 v = *b;
  const float d0 = sh_fipr(a[0], v);
  const float d1 = sh_fipr(a[1], v);
  const float d2 = sh_fipr(a[2], v);
  const float d3 = sh_fipr(a[3], v);
  *out = (sh_float4){d0, d1, d2, d3};
}

/* An alternative output contract: out must not overlap either input. */
void dot4_scalar_outputs(float *restrict out, const sh_float4 a[4],
                         const sh_float4 *b) {
  const sh_float4 v = *b;
  out[0] = sh_fipr(a[0], v);
  out[1] = sh_fipr(a[1], v);
  out[2] = sh_fipr(a[2], v);
  out[3] = sh_fipr(a[3], v);
}

float dot4_sum(const sh_float4 a[4], const sh_float4 *b) {
  const sh_float4 v = *b;
  const float d0 = sh_fipr(a[0], v);
  const float d1 = sh_fipr(a[1], v);
  const float d2 = sh_fipr(a[2], v);
  const float d3 = sh_fipr(a[3], v);
  return ((d0 + d1) + d2) + d3;
}

void dot4_store_align4(float *out, const sh_float4 a[4], const sh_float4 *b) {
  const sh_float4 v = *b;
  const float d0 = sh_fipr(a[0], v);
  const float d1 = sh_fipr(a[1], v);
  const float d2 = sh_fipr(a[2], v);
  const float d3 = sh_fipr(a[3], v);
  sh_store4(out, (sh_float4){d0, d1, d2, d3});
}

/* This function requires an actually eight-byte-aligned destination. */
void dot4_store_align8(float *out, const sh_float4 a[4], const sh_float4 *b) {
  out = __builtin_assume_aligned(out, 8);
  const sh_float4 v = *b;
  const float d0 = sh_fipr(a[0], v);
  const float d1 = sh_fipr(a[1], v);
  const float d2 = sh_fipr(a[2], v);
  const float d3 = sh_fipr(a[3], v);
  sh_store4(out, (sh_float4){d0, d1, d2, d3});
}

/* Integer transfers can be cheaper for a stand-alone bitwise copy. */
void copy_float4(sh_float4 *out, const sh_float4 *in) { *out = *in; }

void copy_header(float *out, const float *in) { sh_store4(out, sh_load4(in)); }

/* restrict makes the matrix disjoint from the output stores. */
void same_matrix_twice(sh_float4 *restrict out, const sh_float4 *restrict in,
                       const sh_matrix4 *restrict matrix) {
  out[0] = sh_ftrv(matrix, in[0]);
  out[1] = sh_ftrv(matrix, in[1]);
}

/* A must-reload case for future matrix-residency optimizations. */
void changed_matrix(sh_float4 *out, const sh_float4 *in, sh_matrix4 *matrix) {
  out[0] = sh_ftrv(matrix, in[0]);
  matrix->column[0][0] = 2.0f;
  out[1] = sh_ftrv(matrix, in[1]);
}

extern void transform_other_tu(sh_float4 *, const sh_float4 *,
                               const sh_matrix4 *);

void across_translation_units(sh_float4 *out, const sh_float4 *in,
                              const sh_matrix4 *matrix) {
  out[0] = sh_ftrv(matrix, in[0]);
  transform_other_tu(out + 1, in + 1, matrix);
}
