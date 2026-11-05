// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <stddef.h>
#include <stdint.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
  unsigned char *d = dest;
  const unsigned char *s = src;
  for (size_t i = 0; i != n; ++i)
    d[i] = s[i];
  return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
  unsigned char *d = dest;
  const unsigned char *s = src;
  if ((uintptr_t)d < (uintptr_t)s) {
    for (size_t i = 0; i != n; ++i)
      d[i] = s[i];
  } else {
    while (n) {
      --n;
      d[n] = s[n];
    }
  }
  return dest;
}

void *memset(void *dest, int value, size_t n) {
  unsigned char *d = dest;
  for (size_t i = 0; i != n; ++i)
    d[i] = (unsigned char)value;
  return dest;
}

int memcmp(const void *lhs, const void *rhs, size_t n) {
  const unsigned char *a = lhs;
  const unsigned char *b = rhs;
  for (size_t i = 0; i != n; ++i)
    if (a[i] != b[i])
      return a[i] - b[i];
  return 0;
}

// SH code generation currently lowers llvm.trap to abort. A freestanding
// abort must not depend on a hosted C library. The application owns the trap
// vector; if its handler returns, remain stopped rather than return to C.
__attribute__((noreturn)) void abort(void) {
  __asm__ volatile("trapa #255");
  for (;;)
    __asm__ volatile("nop");
}
