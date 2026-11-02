/* Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */
#include <shintrin.h>

void transform_other_tu(sh_float4 *out, const sh_float4 *in,
                        const sh_matrix4 *matrix) {
  *out = sh_ftrv(matrix, *in);
}
