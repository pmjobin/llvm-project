// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef LLVM_UTILS_SH_INTEGER_OPERATIONS_H
#define LLVM_UTILS_SH_INTEGER_OPERATIONS_H

#include <stdint.h>

struct Aggregate {
  int32_t Tag;
  uint64_t Wide;
  double Value;
};

struct __attribute__((packed)) Packed5 {
  uint8_t Bytes[5];
};

uint32_t unsigned_divide(uint32_t, uint32_t);
uint32_t unsigned_remainder(uint32_t, uint32_t);
int32_t signed_divide(int32_t, int32_t);
int32_t signed_remainder(int32_t, int32_t);
uint32_t narrow_product(uint8_t, uint8_t);
uint32_t low_byte_product(uint32_t, uint32_t);
uint64_t shift_left(uint64_t, unsigned);
uint64_t shift_right(uint64_t, unsigned);
int64_t shift_signed(int64_t, unsigned);
uint32_t constant_divide(uint32_t);
int32_t constant_signed_divide(int32_t);
void constant_edges(uint32_t, int32_t, uint32_t *);
uint64_t packed_memory(uint8_t *, uint64_t);
struct Aggregate aggregate_call(struct Aggregate, int, int, int, int, int, int);
double variadic_sum(unsigned, ...);
double floating_mix(double, float, int);
int adjacent_calls(int);
uint32_t conditional_flow(uint32_t, unsigned);
unsigned predicates_float(float, float);
unsigned predicates_double(double, double);
uint32_t rotate_left(uint32_t, unsigned);
uint32_t rotate_right(uint32_t, unsigned);
int variadic_packed(unsigned, ...);

#endif
