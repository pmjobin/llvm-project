// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern int first_stack_integer(int, int, int, int, ...);
extern uint64_t pair_at_boundary(int, int, int, ...);
extern void *copied_pointer(int, ...);
extern double double_after_float(float, ...);
extern double stack_double(double, double, double, double, ...);
extern void packed_copy(void *, const void *);
extern void packed_volatile_copy(void *, const void *);
extern int nest_caller(int *);
extern int returnaddress_matches_pr(void);
extern int returnaddress_cfi(void);

int main(void) {
  int value = 37;
  if (first_stack_integer(1, 2, 3, 4, 55) != 55 ||
      pair_at_boundary(1, 2, 3, UINT64_C(0x1122334455667788), 9) !=
          UINT64_C(0x1122334455667791) ||
      copied_pointer(1, &value) != &value ||
      double_after_float(1.25f, 2.5) != 2.5 ||
      stack_double(1.0, 2.0, 3.0, 4.0, 5.5) != 5.5)
    return 1;

  // Compare the representation, including signed zero, quiet NaN payloads and
  // infinities, without performing floating arithmetic on the packed members.
  _Alignas(8) unsigned char source[35] = {
      0x5a, 0x19, 0, 0, 0, 0x80, 0x34, 0x12, 0,    0,    0,   0,
      0xf8, 0x7f, 0, 0, 0, 0,    0,    0,    0,    0,    0,   0,
      0xf0, 0x7f, 0, 0, 0, 0,    0,    0,    0xf0, 0xff, 0xa5};
  _Alignas(8) unsigned char output[35];
  void *pointer = &value;
  memcpy(source + 14, &pointer, sizeof pointer);
  for (unsigned is_volatile = 0; is_volatile != 2; ++is_volatile) {
    memset(output, 0xcc, sizeof output);
    (is_volatile ? packed_volatile_copy : packed_copy)(output + 1, source + 1);
    if (output[0] != 0xcc || output[34] != 0xcc ||
        memcmp(source + 1, output + 1, 33))
      return 2;
  }
  if (nest_caller(&value) != 3 * value + 10)
    return 3;

  register unsigned sentinel __asm__("r8") = 0x12345678;
  __asm__ volatile("" : "+r"(sentinel));
  if (returnaddress_matches_pr() != 1 || returnaddress_cfi() != 1)
    return 4;
  __asm__ volatile("" : "+r"(sentinel));
  if (sentinel != 0x12345678)
    return 5;
  puts("abi-boundaries:PASS");
  return 0;
}
