// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <stddef.h>

extern int archive_function(int);
extern int platform_write(const char *, size_t);

const int read_only[] = {2, 3, 5, 7};
int initialized = 17;
volatile int zero_initialized;
int (*volatile function_pointer)(int) = archive_function;

int main(void) {
  if (zero_initialized != 0 || initialized != 17)
    return 1;
  zero_initialized = function_pointer(read_only[2]) + initialized;
  if (zero_initialized != 55 || archive_function(read_only[3]) != 52)
    return 2;
  static const char message[] = "SH4 C link PASS\n";
  return platform_write(message, sizeof(message) - 1) == sizeof(message) - 1
             ? 0
             : 3;
}
