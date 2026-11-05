// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern int helper_abi_test(void);
extern int platform_write(const char *, size_t);
extern int64_t signed_divide(int64_t, int64_t);
extern int64_t signed_remainder(int64_t, int64_t);
extern uint64_t unsigned_divide(uint64_t, uint64_t);
extern uint64_t unsigned_remainder(uint64_t, uint64_t);
extern uint64_t multiply(uint64_t, uint64_t);
extern uint64_t shift_left(uint64_t, int);
extern uint64_t shift_right(uint64_t, int);
extern int64_t shift_signed(int64_t, int);
extern double signed_to_double(int64_t);
extern float unsigned_to_float(uint64_t);
extern int64_t double_to_signed(double);
extern uint64_t float_to_unsigned(float);
extern double add_double(double, double);
extern double subtract_double(double, double);
extern double multiply_double(double, double);
extern double divide_double(double, double);
extern float add_float(float, float);
extern float divide_float(float, float);
extern int switch_test(unsigned, int);
extern volatile int switch_counter;

struct Big {
  int elements[67];
};
extern void copy_big(struct Big *, const struct Big *);

static const struct {
  uint64_t a, b, quotient, remainder;
} unsigned_cases[] = {
    {0, 1, 0, 0},
    {1, 1, 1, 0},
    {UINT64_MAX, 1, UINT64_MAX, 0},
    {UINT64_MAX, UINT64_MAX, 1, 0},
    {UINT64_MAX, 0x100000000ULL, 0xffffffffULL, 0xffffffffULL},
    {0x100000002ULL, 3, 0x55555556ULL, 0},
    {0x8000000000000000ULL, 2, 0x4000000000000000ULL, 0},
    {0x100000000ULL, 0xffffffffULL, 1, 1},
    {7, 0x100000000ULL, 0, 7},
    {UINT64_MAX, 0x8000000000000000ULL, 1, 0x7fffffffffffffffULL}};

static const struct {
  int64_t a, b, quotient, remainder;
} signed_cases[] = {{0, -1, 0, 0},
                    {-7, 3, -2, -1},
                    {7, -3, -2, 1},
                    {-7, -3, 2, -1},
                    {INT64_MIN, 1, INT64_MIN, 0},
                    {INT64_MIN, 2, -0x4000000000000000LL, 0},
                    {INT64_MAX, -1, -INT64_MAX, 0},
                    {-0x100000005LL, 7, -613566757LL, -2},
                    {INT64_MAX, INT64_MAX, 1, 0}};

static uint64_t double_bits(double value) {
  union {
    double f;
    uint64_t u;
  } bits = {value};
  return bits.u;
}
static uint32_t float_bits(float value) {
  union {
    float f;
    uint32_t u;
  } bits = {value};
  return bits.u;
}

int main(void) {
  if (helper_abi_test())
    return 10;
  for (unsigned i = 0; i < sizeof(unsigned_cases) / sizeof(unsigned_cases[0]);
       ++i)
    if (unsigned_divide(unsigned_cases[i].a, unsigned_cases[i].b) !=
            unsigned_cases[i].quotient ||
        unsigned_remainder(unsigned_cases[i].a, unsigned_cases[i].b) !=
            unsigned_cases[i].remainder)
      return 11;
  for (unsigned i = 0; i < sizeof(signed_cases) / sizeof(signed_cases[0]); ++i)
    if (signed_divide(signed_cases[i].a, signed_cases[i].b) !=
            signed_cases[i].quotient ||
        signed_remainder(signed_cases[i].a, signed_cases[i].b) !=
            signed_cases[i].remainder)
      return 12;
  if (multiply(0x123456789abcdef0ULL, 0x100000001ULL) !=
          0xacf135689abcdef0ULL ||
      shift_left(0x123456789abcdef0ULL, 36) != 0xabcdef0000000000ULL ||
      shift_right(0x123456789abcdef0ULL, 36) != 0x1234567ULL ||
      shift_signed(-0x123456789abcdefLL, 36) != -0x123457LL)
    return 13;
  if (double_bits(signed_to_double(-4294967297LL)) != 0xc1f0000000100000ULL ||
      float_bits(unsigned_to_float(0x100000001ULL)) != 0x4f800000U ||
      double_to_signed(-4294967297.75) != -4294967297LL ||
      float_to_unsigned(4294967296.0F) != 0x100000000ULL)
    return 14;
  if (double_bits(add_double(1.25, 2.5)) != 0x400e000000000000ULL ||
      double_bits(subtract_double(1.25, 2.5)) != 0xbff4000000000000ULL ||
      double_bits(multiply_double(1.25, 2.5)) != 0x4009000000000000ULL ||
      double_bits(divide_double(7.5, 2.5)) != 0x4008000000000000ULL ||
      float_bits(add_float(1.25F, 2.5F)) != 0x40700000U ||
      float_bits(divide_float(7.5F, 2.5F)) != 0x40400000U)
    return 15;

  unsigned char a[263], b[263];
  for (unsigned i = 0; i != sizeof(a); ++i)
    a[i] = b[i] = (unsigned char)(i * 17);
  for (unsigned n = 0; n != 258; ++n) {
    if (memset(a + 1, 0xa5, n) != a + 1 || memcpy(a + 1, b + 3, n) != a + 1 ||
        memcmp(a + 1, b + 3, n))
      return 16;
  }
  for (unsigned i = 0; i != sizeof(a); ++i)
    a[i] = (unsigned char)i;
  if (memmove(a + 3, a, 257) != a + 3)
    return 17;
  for (unsigned i = 0; i != 257; ++i)
    if (a[i + 3] != (unsigned char)i)
      return 17;
  memmove(a, a + 3, 257);
  for (unsigned i = 0; i != 257; ++i)
    if (a[i] != (unsigned char)i)
      return 18;
  a[0] = 0x80;
  b[0] = 0x7f;
  if (memcmp(a, b, 1) <= 0 || memcmp(b, a, 1) >= 0)
    return 19;
  struct Big original, copy;
  for (unsigned i = 0; i != 67; ++i)
    original.elements[i] = i * 97;
  copy_big(&copy, &original);
  for (unsigned i = 0; i != 67; ++i)
    if (copy.elements[i] != (int)i * 97)
      return 20;
  const int switch_expected[] = {38, 17, 25, 58, 23, 100, -6, 45, -1};
  for (unsigned i = 0; i != 9; ++i)
    if (switch_test(i, 5) != switch_expected[i])
      return 21;
  if (switch_counter != 0)
    return 22;
  static const char message[] = "SH4 runtime PASS\n";
  return platform_write(message, sizeof(message) - 1) == sizeof(message) - 1
             ? 0
             : 23;
}
