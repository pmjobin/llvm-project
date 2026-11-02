// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "operations.h"
#include <stdarg.h>
#include <string.h>

uint32_t unsigned_divide(uint32_t A, uint32_t B) { return A / B; }
uint32_t unsigned_remainder(uint32_t A, uint32_t B) { return A % B; }
int32_t signed_divide(int32_t A, int32_t B) { return A / B; }
int32_t signed_remainder(int32_t A, int32_t B) { return A % B; }
uint32_t narrow_product(uint8_t A, uint8_t B) { return (uint16_t)(A * B); }
uint32_t low_byte_product(uint32_t A, uint32_t B) {
  return (uint8_t)((uint8_t)A * (uint8_t)B);
}
uint64_t shift_left(uint64_t A, unsigned N) { return A << (N & 63); }
uint64_t shift_right(uint64_t A, unsigned N) { return A >> (N & 63); }
int64_t shift_signed(int64_t A, unsigned N) { return A >> (N & 63); }
uint32_t constant_divide(uint32_t A) { return A / 17; }
int32_t constant_signed_divide(int32_t A) { return A / 251; }

void constant_edges(uint32_t U, int32_t S, uint32_t *Results) {
  Results[0] = U / UINT32_MAX;
  Results[1] = U % UINT32_MAX;
  Results[2] = U / 17;
  Results[3] = S / 2;
  Results[4] = S / -2;
  Results[5] = S / INT32_MIN;
  Results[6] = S % INT32_MIN;
  Results[7] = S / 251;
}

uint64_t packed_memory(uint8_t *Bytes, uint64_t Value) {
  struct __attribute__((packed)) Packed {
    uint8_t Tag;
    uint32_t Word;
    uint16_t Half;
    uint64_t Wide;
  };
  // Caller provides storage with no alignment promise. Volatile accesses must
  // preserve widths/order; the following memcpy also exercises ordinary loads.
  volatile struct Packed *P = (volatile struct Packed *)Bytes;
  P->Tag = 17;
  P->Word = UINT32_C(0x89abcdef);
  P->Half = UINT16_C(0xfedc);
  P->Wide = Value;
  uint64_t Copy;
  memcpy(&Copy, Bytes + 7, sizeof(Copy));
  return Copy ^ P->Wide ^ P->Word ^ P->Half ^ P->Tag;
}

struct Aggregate aggregate_call(struct Aggregate A, int B, int C, int D, int E,
                                int F, int G) {
  A.Tag += B + C + D + E + F + G;
  A.Wide ^= UINT64_C(0x8123456789abcdef);
  A.Value += (double)A.Tag;
  return A;
}

double variadic_sum(unsigned Count, ...) {
  va_list Args;
  va_start(Args, Count);
  double Sum = 0;
  for (unsigned I = 0; I != Count; ++I) {
    Sum += va_arg(Args, int);
    Sum += va_arg(Args, double);
  }
  va_end(Args);
  return Sum;
}

double floating_mix(double A, float B, int N) { return A + B * N; }

#define PREDICATES(A, B)                                                       \
  ((A == B) | ((A != B) << 1) | ((A < B) << 2) | ((A <= B) << 3) |             \
   ((A > B) << 4) | ((A >= B) << 5) | (__builtin_isunordered(A, B) << 6) |     \
   (__builtin_islessgreater(A, B) << 7))

unsigned predicates_float(float A, float B) { return PREDICATES(A, B); }
unsigned predicates_double(double A, double B) { return PREDICATES(A, B); }

uint32_t rotate_left(uint32_t A, unsigned N) {
  return (A << (N & 31)) | (A >> ((-N) & 31));
}
uint32_t rotate_right(uint32_t A, unsigned N) {
  return (A >> (N & 31)) | (A << ((-N) & 31));
}

int variadic_packed(unsigned Count, ...) {
  va_list Args;
  va_start(Args, Count);
  double Sum = 0;
  for (unsigned I = 0; I != Count; ++I) {
    Sum += va_arg(Args, int);
    struct Packed5 P = va_arg(Args, struct Packed5);
    for (unsigned J = 0; J != 5; ++J)
      Sum += P.Bytes[J];
    Sum += 2 * va_arg(Args, double);
  }
  va_end(Args);
  return (int)Sum;
}

static __attribute__((noinline)) int
many_arguments(int A, int B, int C, int D, int E, int F, int G, int H) {
  return A - B + C - D + E - F + G - H;
}

int adjacent_calls(int X) {
  int A = many_arguments(X, 2, 3, 4, 5, 6, 7, 8);
  int B = many_arguments(9, X, 11, 12, 13, 14, 15, 16);
  return A * 3 + B;
}

uint32_t conditional_flow(uint32_t A, unsigned Selector) {
  switch (Selector % 7) {
  case 0:
    return A + 17;
  case 1:
    return A ^ UINT32_C(0x89abcdef);
  case 2:
    return A * 251;
  case 3:
    return A / 17;
  case 4:
    return A & 255;
  case 5:
    return A | UINT32_C(0xf0000000);
  default:
    return ~A;
  }
}
