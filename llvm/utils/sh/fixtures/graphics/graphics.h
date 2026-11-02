// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifdef __clang__
#include <shintrin.h>
#else
typedef float sh_float2 __attribute__((vector_size(8)));
typedef float sh_float4 __attribute__((vector_size(16)));
typedef struct {
  sh_float4 column[4];
} sh_matrix4;
#endif
float graphics_dot(sh_float4, sh_float4);
sh_float4 graphics_transform(const sh_matrix4 *, sh_float4);
void graphics_batch(sh_float4 *, const sh_float4 *, const sh_matrix4 *);
void graphics_unaligned(float *, const float *, const float *);
sh_float4 graphics_pressure(const float *, sh_float4);
void graphics_barrier(void);
sh_float4 graphics_index(sh_float4, unsigned);
int graphics_abi(void);
sh_float4 graphics_gnu_v4(unsigned, sh_float4, unsigned);
sh_float2 graphics_gnu_v2(unsigned, unsigned, unsigned, sh_float2, unsigned);
float graphics_alias(sh_matrix4 *, sh_float4);
void graphics_snapshot(float *, const sh_matrix4 *);
sh_float4 graphics_shuffle(sh_float4, sh_float4);
void graphics_copy(float *, const float *);
sh_float4 graphics_gnu_varargs(unsigned, ...);
sh_float4 graphics_clang_varargs(unsigned, ...);
