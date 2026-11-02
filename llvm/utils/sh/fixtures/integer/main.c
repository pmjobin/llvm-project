// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "operations.h"
#include "vectors.h"
#include <stdio.h>

#define CHECK(C)                                                               \
  do {                                                                         \
    if (!(C)) {                                                                \
      fprintf(stderr, "integer-abi failure at %d\n", __LINE__);                \
      return 1;                                                                \
    }                                                                          \
  } while (0)

int main(void) {
  for (unsigned I = 0; I != sizeof(Unsigned) / sizeof(Unsigned[0]); ++I) {
    CHECK(unsigned_divide(Unsigned[I].A, Unsigned[I].B) == Unsigned[I].Q);
    CHECK(unsigned_remainder(Unsigned[I].A, Unsigned[I].B) == Unsigned[I].R);
  }
  for (unsigned I = 0; I != sizeof(Signed) / sizeof(Signed[0]); ++I) {
    CHECK(signed_divide(Signed[I].A, Signed[I].B) == Signed[I].Q);
    CHECK(signed_remainder(Signed[I].A, Signed[I].B) == Signed[I].R);
  }
  for (unsigned I = 0; I != sizeof(Shifts) / sizeof(Shifts[0]); ++I) {
    CHECK(shift_left(Shifts[I].A, Shifts[I].N) == Shifts[I].L);
    CHECK(shift_right(Shifts[I].A, Shifts[I].N) == Shifts[I].R);
    CHECK((uint64_t)shift_signed((int64_t)Shifts[I].A, Shifts[I].N) ==
          Shifts[I].S);
  }
  CHECK(narrow_product(255, 255) == 65025);
  CHECK(narrow_product(129, 255) == 32895);
  CHECK(narrow_product(128, 127) == 16256);
  CHECK(low_byte_product(255, 255) == 1);
  CHECK(low_byte_product(129, 255) == 127);
  CHECK(constant_divide(UINT32_C(0xffffffff)) == UINT32_C(252645135));
  CHECK(constant_signed_divide(-2147483647) == -8555711);
  // Literal results use mathematical truncation toward zero, generated without
  // the compiler under test. Each operation is in a separate translation unit.
  const struct {
    uint32_t U;
    int32_t S;
    uint32_t Results[8];
  } Edges[] = {
      {UINT32_C(0x00000000),
       0,
       {UINT32_C(0x00000000), UINT32_C(0x00000000), UINT32_C(0x00000000),
        UINT32_C(0x00000000), UINT32_C(0x00000000), UINT32_C(0x00000000),
        UINT32_C(0x00000000), UINT32_C(0x00000000)}},
      {UINT32_C(0x00000001),
       1,
       {UINT32_C(0x00000000), UINT32_C(0x00000001), UINT32_C(0x00000000),
        UINT32_C(0x00000000), UINT32_C(0x00000000), UINT32_C(0x00000000),
        UINT32_C(0x00000001), UINT32_C(0x00000000)}},
      {UINT32_C(0xffffffff),
       -1,
       {UINT32_C(0x00000001), UINT32_C(0x00000000), UINT32_C(0x0f0f0f0f),
        UINT32_C(0x00000000), UINT32_C(0x00000000), UINT32_C(0x00000000),
        UINT32_C(0xffffffff), UINT32_C(0x00000000)}},
      {UINT32_C(0xfffffffe),
       INT32_MIN,
       {UINT32_C(0x00000000), UINT32_C(0xfffffffe), UINT32_C(0x0f0f0f0e),
        UINT32_C(0xc0000000), UINT32_C(0x40000000), UINT32_C(0x00000001),
        UINT32_C(0x00000000), UINT32_C(0xff7d7341)}},
      {UINT32_C(0x80000000),
       -2147483647,
       {UINT32_C(0x00000000), UINT32_C(0x80000000), UINT32_C(0x07878787),
        UINT32_C(0xc0000001), UINT32_C(0x3fffffff), UINT32_C(0x00000000),
        UINT32_C(0x80000001), UINT32_C(0xff7d7341)}},
      {UINT32_C(0x7fffffff),
       2147483647,
       {UINT32_C(0x00000000), UINT32_C(0x7fffffff), UINT32_C(0x07878787),
        UINT32_C(0x3fffffff), UINT32_C(0xc0000001), UINT32_C(0x00000000),
        UINT32_C(0x7fffffff), UINT32_C(0x00828cbf)}},
      {UINT32_C(0x00000003),
       -3,
       {UINT32_C(0x00000000), UINT32_C(0x00000003), UINT32_C(0x00000000),
        UINT32_C(0xffffffff), UINT32_C(0x00000001), UINT32_C(0x00000000),
        UINT32_C(0xfffffffd), UINT32_C(0x00000000)}},
      {UINT32_C(0x00000011),
       3,
       {UINT32_C(0x00000000), UINT32_C(0x00000011), UINT32_C(0x00000001),
        UINT32_C(0x00000001), UINT32_C(0xffffffff), UINT32_C(0x00000000),
        UINT32_C(0x00000003), UINT32_C(0x00000000)}},
  };
  for (unsigned I = 0; I != sizeof(Edges) / sizeof(Edges[0]); ++I) {
    uint32_t Results[8];
    constant_edges(Edges[I].U, Edges[I].S, Results);
    for (unsigned J = 0; J != 8; ++J)
      CHECK(Results[J] == Edges[I].Results[J]);
  }
  uint8_t Storage[32] = {0};
  CHECK(packed_memory(Storage + 1, UINT64_C(0x8123456789abcdef)) ==
        UINT64_C(0x89ab3322));
  struct Aggregate A = {7, UINT64_C(0xfedcba9876543210), 1.25};
  A = aggregate_call(A, 1, 2, 3, 4, 5, 6);
  CHECK(A.Tag == 28 && A.Wide == UINT64_C(0x7fffffffffffffff) &&
        A.Value == 29.25);
  CHECK(variadic_sum(6, 1, 0.25, 2, 0.5, 3, 0.75, 4, 1.0, 5, 1.25, 6, 1.5) ==
        26.25);
  CHECK(floating_mix(1.25, 0.5f, 7) == 4.75);
  const double Pairs[][3] = {
      {0.0, -0.0, 41},
      {1, 2, 142},
      {2, 1, 178},
      {__builtin_nan(""), 1, 66},
      {1, __builtin_nan(""), 66},
      {__builtin_inf(), __builtin_inf(), 41},
      {-__builtin_inf(), __builtin_inf(), 142},
  };
  for (unsigned I = 0; I != sizeof(Pairs) / sizeof(Pairs[0]); ++I) {
    CHECK(predicates_float(Pairs[I][0], Pairs[I][1]) == (unsigned)Pairs[I][2]);
    CHECK(predicates_double(Pairs[I][0], Pairs[I][1]) == (unsigned)Pairs[I][2]);
  }
  const unsigned Counts[] = {0, 1, 31, 32, 63};
  const uint32_t Left[] = {0x81234567, 0x02468acf, 0xc091a2b3, 0x81234567,
                           0xc091a2b3};
  const uint32_t Right[] = {0x81234567, 0xc091a2b3, 0x02468acf, 0x81234567,
                            0x02468acf};
  for (unsigned I = 0; I != 5; ++I) {
    CHECK(rotate_left(0x81234567, Counts[I]) == Left[I]);
    CHECK(rotate_right(0x81234567, Counts[I]) == Right[I]);
  }
  const struct Packed5 P1 = {{1, 2, 3, 4, 5}};
  const struct Packed5 P2 = {{6, 7, 8, 9, 10}};
  const struct Packed5 P3 = {{11, 12, 13, 14, 15}};
  CHECK(variadic_packed(3, 7, P1, 1.5, 11, P2, 2.5, 13, P3, 3.5) == 166);
  CHECK(adjacent_calls(19) == 29);
  const uint32_t Flow[] = {272,
                           UINT32_C(0x89abcd10),
                           64005,
                           15,
                           255,
                           UINT32_C(0xf00000ff),
                           UINT32_C(0xffffff00)};
  for (unsigned I = 0; I != 7; ++I)
    CHECK(conditional_flow(255, I) == Flow[I]);
  puts("integer-abi:PASS");
  return 0;
}
