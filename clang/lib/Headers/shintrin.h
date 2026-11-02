/*===---- shintrin.h - Explicit SuperH graphics operations
 * -------------------===*
 *
 * Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===----------------------------------------------------------------------===*/
#ifndef __SHINTRIN_H
#define __SHINTRIN_H
#ifndef __sh__
#error "shintrin.h requires a SuperH target"
#endif

typedef float sh_float2 __attribute__((__vector_size__(8)));
typedef float sh_float4 __attribute__((__vector_size__(16)));
/* column[j][i] is row i, column j. No persistent hardware matrix state. */
typedef struct {
  sh_float4 column[4];
} sh_matrix4;
typedef char __sh_matrix_size_check[sizeof(sh_matrix4) == 64 ? 1 : -1];
typedef char __sh_matrix_column_check
    [__builtin_offsetof(sh_matrix4, column[1]) == 16 &&
             __builtin_offsetof(sh_matrix4, column[2]) == 32 &&
             __builtin_offsetof(sh_matrix4, column[3]) == 48
         ? 1
         : -1];

#define __SH_INLINE                                                            \
  static __inline__ __attribute__((__always_inline__, __nodebug__))
__SH_INLINE sh_float4 sh_load4(const float *__p) {
  sh_float4 __v;
  __builtin_memcpy(&__v, __p, sizeof(__v));
  return __v;
}
__SH_INLINE void sh_store4(float *__p, sh_float4 __v) {
  __builtin_memcpy(__p, &__v, sizeof(__v));
}
__SH_INLINE sh_matrix4 sh_matrix4_load(const float *__p) {
  sh_matrix4 __m;
  __builtin_memcpy(&__m, __p, sizeof(__m));
  return __m;
}
__SH_INLINE void sh_matrix4_store(float *__p, const sh_matrix4 *__m) {
  sh_matrix4 __copy = *__m;
  __builtin_memcpy(__p, &__copy, sizeof(__copy));
}
#ifdef __SH_GRAPHICS__
__SH_INLINE float sh_fipr(sh_float4 __a, sh_float4 __b) {
  return __builtin_sh_fipr(__a, __b);
}
__SH_INLINE sh_float4 sh_ftrv(const sh_matrix4 *__m, sh_float4 __v) {
  return __builtin_sh_ftrv(__m, __v);
}
/* Arrays must be disjoint or identical; the matrix must be disjoint from both.
 */
__SH_INLINE void sh_ftrv4(sh_float4 *__dst, const sh_float4 *__src,
                          const sh_matrix4 *__m) {
  __builtin_sh_ftrv4(__dst, __src, __m);
}
/* One matrix load for the whole array. Zero count accesses no memory and
 * performs no floating-point operation. The same overlap rules as ftrv4 apply.
 */
__SH_INLINE void sh_ftrv_array(sh_float4 *__dst, const sh_float4 *__src,
                               unsigned __count, const sh_matrix4 *__m) {
  __builtin_sh_ftrv_array(__dst, __src, __count, __m);
}
/* Form C.column[j] = FTRV(left, right.column[j]), then transform the array
 * with C without materializing C in memory. This preserves that instruction
 * sequence, including its rounding and status effects. Arrays must be disjoint
 * or identical. Both matrices are read before any output is written and may
 * overlap either array or each other. Zero count is a complete no-op.
 */
__SH_INLINE void sh_ftrv_compose(sh_float4 *__dst, const sh_float4 *__src,
                                 unsigned __count, const sh_matrix4 *__left,
                                 const sh_matrix4 *__right) {
  __builtin_sh_ftrv_compose(__dst, __src, __count, __left, __right);
}
#endif
#ifdef __SH_APPROX_MATH__
/* Low 16 phase bits represent a full turn; result lanes are sine, cosine. */
__SH_INLINE sh_float2 sh_fsca(unsigned __phase) {
  return __builtin_sh_fsca(__phase);
}
__SH_INLINE float sh_fsrra(float __x) { return __builtin_sh_fsrra(__x); }
#endif
#undef __SH_INLINE
#endif
