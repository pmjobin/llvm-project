// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <stddef.h>

typedef void (*Initializer)(void);
extern Initializer __preinit_array_start[], __preinit_array_end[];
extern Initializer __init_array_start[], __init_array_end[];
extern Initializer __fini_array_start[], __fini_array_end[];
extern int main(void);
extern void __cxa_finalize(void *) __attribute__((weak));
extern int after_fini(int) __attribute__((weak));

int sh4_start(void) {
  for (Initializer *p = __preinit_array_start; p != __preinit_array_end; ++p)
    (*p)();
  for (Initializer *p = __init_array_start; p != __init_array_end; ++p)
    (*p)();
  int result = main();
  if (__cxa_finalize)
    __cxa_finalize(NULL);
  for (Initializer *p = __fini_array_end; p != __fini_array_start;)
    (*--p)();
  return after_fini ? after_fini(result) : result;
}
