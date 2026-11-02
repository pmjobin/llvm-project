// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// GNU caller for the actual CodeGen/SH stack, arithmetic and delay-slot tests.
// Keep the IR in those tests as the single implementation of each witness.
#include <stdint.h>
#include <stdio.h>

#define CHECK(C)                                                               \
  do {                                                                         \
    if (!(C)) {                                                                \
      fprintf(stderr, "scheduler-contracts failure at %d\n", __LINE__);        \
      return 1;                                                                \
    }                                                                          \
  } while (0)

extern int call_return_fifth(int, int, int, int, int);
extern int call_eight_mixed(int, int, int, int, int, int, int, int);
extern int stack_value_before_register_overwrite(int, int, int, int, int);
extern int register_cycle_with_stack(int, int, int, int);
extern int multiple_call_frame_sizes(int);
extern void *pointer_stack_argument(void *);
extern int start_recursive_stack_call(int);
extern int call6(int, int);
extern int call19(void);
extern int direct_fifth_object(void);
typedef int (*Call8)(int, int, int, int, int, int, int, int);
extern int indirect_eight_object(Call8);
extern void add_i8(uint8_t *);
extern void add_i16(uint16_t *);
extern int add_i8_signed_compare(uint8_t *);
extern int add_i16_unsigned_compare(uint16_t *);
extern _Bool compare_argument(int);
extern _Bool compare_size_argument(int);
extern _Bool compare_call_result(void);
extern int branch_on_call_result(void);
extern uint32_t udivrem_max(uint32_t);
extern uint32_t udiv_max(uint32_t);
extern uint32_t urem_max(uint32_t);
extern int32_t srem_min(int32_t);

static volatile int CallResult;

int callee(void) { return CallResult; }

static int weighted(int A0, int A1, int A2, int A3, int A4, int A5, int A6,
                    int A7) {
  return A0 + 2 * A1 + 4 * A2 + 8 * A3 + 16 * A4 + 32 * A5 + 64 * A6 + 128 * A7;
}

int main(void) {
  for (int I = -10; I <= 10; ++I) {
    CHECK(call_return_fifth(1, 2, 3, 4, I) == I);
    CHECK(call_eight_mixed(I, 2, 3, 4, 5, 6, 7, 8) == 2 * I + 29);
    CHECK(stack_value_before_register_overwrite(I, 2, 3, 4, 5) == I);
    CHECK(register_cycle_with_stack(I, 2, 3, 4) == 2 * I + 18);
    CHECK(multiple_call_frame_sizes(I) == 2 * I + 30);
    CHECK(call6(123, I) == I);
    CHECK(compare_argument(I) == (I == 7));
    CHECK(compare_size_argument(I) == (I == 7));
    CHECK(pointer_stack_argument(&I) == &I);
  }
  for (int N = 0; N != 10; ++N)
    CHECK(start_recursive_stack_call(N) == 4 * (N + 1) + N * (N + 1) / 2);
  CHECK(call19() == 1);
  CHECK(direct_fifth_object() == 5);
  CHECK(indirect_eight_object(weighted) == 1793);

  // The i8 addition wraps before the signed > 0 comparison. Express the
  // oracle without an implementation-defined unsigned-to-signed conversion.
  for (unsigned I = 0; I != 256; ++I) {
    uint8_t Byte = I;
    unsigned Sum = (I + 1) & 255;
    CHECK(add_i8_signed_compare(&Byte) == (Sum > 0 && Sum < 128));
    CHECK(Byte == I);
    add_i8(&Byte);
    CHECK(Byte == Sum);
  }
  for (unsigned I = 0; I != 65536; ++I) {
    uint16_t Half = I;
    unsigned Sum = (I + 1) & 65535;
    CHECK(add_i16_unsigned_compare(&Half) == (Sum > 1));
    CHECK(Half == I);
    add_i16(&Half);
    CHECK(Half == Sum);
  }

  const int32_t Signed[] = {INT32_MIN, INT32_MIN + 1, -7, -1, 0, 1,
                            42,        INT32_MAX};
  for (unsigned I = 0; I != sizeof(Signed) / sizeof(Signed[0]); ++I) {
    int32_t X = Signed[I];
    CallResult = X;
    CHECK(compare_call_result() == (X == -7));
    CHECK(branch_on_call_result() == (X == 42 ? 11 : 17));
    CHECK(srem_min(X) == (X == INT32_MIN ? 0 : X));
  }
  const uint32_t Edges[] = {0,          1,          0x7fffffff,
                            0x80000000, 0xfffffffe, UINT32_MAX};
  uint32_t Random = 0x31415926;
  for (unsigned I = 0; I != 4096; ++I) {
    uint32_t X = I < sizeof(Edges) / sizeof(Edges[0]) ? Edges[I] : Random;
    Random = Random * UINT32_C(1664525) + UINT32_C(1013904223);
    CHECK(udiv_max(X) == (X == UINT32_MAX));
    CHECK(urem_max(X) == (X == UINT32_MAX ? 0 : X));
    CHECK(udivrem_max(X) == (X == UINT32_MAX ? 1 : X));
  }
  puts("scheduler-contracts:PASS");
  return 0;
}
