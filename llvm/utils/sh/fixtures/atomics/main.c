// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// GNU compiles this oracle; Clang compiles the exact CodeGen IR providers.
// Each row lists old value, operand, and the 14 resulting stored values in
// operation-table order. Constants use modular integer arithmetic and signed
// mathematical ordering; no result is calculated by the compiler under test.
// Sequential execution checks values, ABI, guards and libatomic calls. It does
// not establish interthread ordering or the number of volatile memory accesses.

extern uint32_t add4(uint32_t *, uint32_t);
extern uint32_t sub4(uint32_t *, uint32_t);
extern uint32_t and4(uint32_t *, uint32_t);
extern uint32_t nand4(uint32_t *, uint32_t);
extern uint32_t or4(uint32_t *, uint32_t);
extern uint32_t xor4(uint32_t *, uint32_t);
extern uint32_t max4(uint32_t *, uint32_t);
extern uint32_t min4(uint32_t *, uint32_t);
extern uint32_t umax4(uint32_t *, uint32_t);
extern uint32_t umin4(uint32_t *, uint32_t);
extern uint32_t uinc4(uint32_t *, uint32_t);
extern uint32_t udec4(uint32_t *, uint32_t);
extern uint32_t usub_cond4(uint32_t *, uint32_t);
extern uint32_t usub_sat4(uint32_t *, uint32_t);

extern uint64_t add8(uint64_t *, uint64_t);
extern uint64_t sub8(uint64_t *, uint64_t);
extern uint64_t and8(uint64_t *, uint64_t);
extern uint64_t nand8(uint64_t *, uint64_t);
extern uint64_t or8(uint64_t *, uint64_t);
extern uint64_t xor8(uint64_t *, uint64_t);
extern uint64_t max8(uint64_t *, uint64_t);
extern uint64_t min8(uint64_t *, uint64_t);
extern uint64_t umax8(uint64_t *, uint64_t);
extern uint64_t umin8(uint64_t *, uint64_t);
extern uint64_t uinc8(uint64_t *, uint64_t);
extern uint64_t udec8(uint64_t *, uint64_t);
extern uint64_t usub_cond8(uint64_t *, uint64_t);
extern uint64_t usub_sat8(uint64_t *, uint64_t);

extern unsigned cas_logic32(void *, void *, uint32_t, uint32_t, unsigned);
extern unsigned cas_float32(void *, void *, float, float, unsigned);
extern unsigned cas_double64(void *, void *, double, double, unsigned);

static uint32_t (*const operations32[])(uint32_t *, uint32_t) = {
    add4, sub4,  and4,  nand4, or4,   xor4,       max4,
    min4, umax4, umin4, uinc4, udec4, usub_cond4, usub_sat4};

static const struct {
  uint32_t old, operand, stored[14];
} vectors32[] = {
    {UINT32_C(0x00000000),
     UINT32_C(0x00000000),
     {UINT32_C(0x00000000), UINT32_C(0x00000000), UINT32_C(0x00000000),
      UINT32_C(0xffffffff), UINT32_C(0x00000000), UINT32_C(0x00000000),
      UINT32_C(0x00000000), UINT32_C(0x00000000), UINT32_C(0x00000000),
      UINT32_C(0x00000000), UINT32_C(0x00000000), UINT32_C(0x00000000),
      UINT32_C(0x00000000), UINT32_C(0x00000000)}},
    {UINT32_C(0x00000000),
     UINT32_C(0x00000001),
     {UINT32_C(0x00000001), UINT32_C(0xffffffff), UINT32_C(0x00000000),
      UINT32_C(0xffffffff), UINT32_C(0x00000001), UINT32_C(0x00000001),
      UINT32_C(0x00000001), UINT32_C(0x00000000), UINT32_C(0x00000001),
      UINT32_C(0x00000000), UINT32_C(0x00000001), UINT32_C(0x00000001),
      UINT32_C(0x00000000), UINT32_C(0x00000000)}},
    {UINT32_C(0x00000001),
     UINT32_C(0x00000000),
     {UINT32_C(0x00000001), UINT32_C(0x00000001), UINT32_C(0x00000000),
      UINT32_C(0xffffffff), UINT32_C(0x00000001), UINT32_C(0x00000001),
      UINT32_C(0x00000001), UINT32_C(0x00000000), UINT32_C(0x00000001),
      UINT32_C(0x00000000), UINT32_C(0x00000000), UINT32_C(0x00000000),
      UINT32_C(0x00000001), UINT32_C(0x00000001)}},
    {UINT32_C(0x00000001),
     UINT32_C(0x00000001),
     {UINT32_C(0x00000002), UINT32_C(0x00000000), UINT32_C(0x00000001),
      UINT32_C(0xfffffffe), UINT32_C(0x00000001), UINT32_C(0x00000000),
      UINT32_C(0x00000001), UINT32_C(0x00000001), UINT32_C(0x00000001),
      UINT32_C(0x00000001), UINT32_C(0x00000000), UINT32_C(0x00000000),
      UINT32_C(0x00000000), UINT32_C(0x00000000)}},
    {UINT32_C(0x00000007),
     UINT32_C(0x00000007),
     {UINT32_C(0x0000000e), UINT32_C(0x00000000), UINT32_C(0x00000007),
      UINT32_C(0xfffffff8), UINT32_C(0x00000007), UINT32_C(0x00000000),
      UINT32_C(0x00000007), UINT32_C(0x00000007), UINT32_C(0x00000007),
      UINT32_C(0x00000007), UINT32_C(0x00000000), UINT32_C(0x00000006),
      UINT32_C(0x00000000), UINT32_C(0x00000000)}},
    {UINT32_C(0x00000006),
     UINT32_C(0x00000007),
     {UINT32_C(0x0000000d), UINT32_C(0xffffffff), UINT32_C(0x00000006),
      UINT32_C(0xfffffff9), UINT32_C(0x00000007), UINT32_C(0x00000001),
      UINT32_C(0x00000007), UINT32_C(0x00000006), UINT32_C(0x00000007),
      UINT32_C(0x00000006), UINT32_C(0x00000007), UINT32_C(0x00000005),
      UINT32_C(0x00000006), UINT32_C(0x00000000)}},
    {UINT32_C(0x00000008),
     UINT32_C(0x00000007),
     {UINT32_C(0x0000000f), UINT32_C(0x00000001), UINT32_C(0x00000000),
      UINT32_C(0xffffffff), UINT32_C(0x0000000f), UINT32_C(0x0000000f),
      UINT32_C(0x00000008), UINT32_C(0x00000007), UINT32_C(0x00000008),
      UINT32_C(0x00000007), UINT32_C(0x00000000), UINT32_C(0x00000007),
      UINT32_C(0x00000001), UINT32_C(0x00000001)}},
    {UINT32_C(0xffffffff),
     UINT32_C(0x00000000),
     {UINT32_C(0xffffffff), UINT32_C(0xffffffff), UINT32_C(0x00000000),
      UINT32_C(0xffffffff), UINT32_C(0xffffffff), UINT32_C(0xffffffff),
      UINT32_C(0x00000000), UINT32_C(0xffffffff), UINT32_C(0xffffffff),
      UINT32_C(0x00000000), UINT32_C(0x00000000), UINT32_C(0x00000000),
      UINT32_C(0xffffffff), UINT32_C(0xffffffff)}},
    {UINT32_C(0x00000000),
     UINT32_C(0xffffffff),
     {UINT32_C(0xffffffff), UINT32_C(0x00000001), UINT32_C(0x00000000),
      UINT32_C(0xffffffff), UINT32_C(0xffffffff), UINT32_C(0xffffffff),
      UINT32_C(0x00000000), UINT32_C(0xffffffff), UINT32_C(0xffffffff),
      UINT32_C(0x00000000), UINT32_C(0x00000001), UINT32_C(0xffffffff),
      UINT32_C(0x00000000), UINT32_C(0x00000000)}},
    {UINT32_C(0xffffffff),
     UINT32_C(0xffffffff),
     {UINT32_C(0xfffffffe), UINT32_C(0x00000000), UINT32_C(0xffffffff),
      UINT32_C(0x00000000), UINT32_C(0xffffffff), UINT32_C(0x00000000),
      UINT32_C(0xffffffff), UINT32_C(0xffffffff), UINT32_C(0xffffffff),
      UINT32_C(0xffffffff), UINT32_C(0x00000000), UINT32_C(0xfffffffe),
      UINT32_C(0x00000000), UINT32_C(0x00000000)}},
    {UINT32_C(0x80000000),
     UINT32_C(0x7fffffff),
     {UINT32_C(0xffffffff), UINT32_C(0x00000001), UINT32_C(0x00000000),
      UINT32_C(0xffffffff), UINT32_C(0xffffffff), UINT32_C(0xffffffff),
      UINT32_C(0x7fffffff), UINT32_C(0x80000000), UINT32_C(0x80000000),
      UINT32_C(0x7fffffff), UINT32_C(0x00000000), UINT32_C(0x7fffffff),
      UINT32_C(0x00000001), UINT32_C(0x00000001)}},
    {UINT32_C(0x7fffffff),
     UINT32_C(0x80000000),
     {UINT32_C(0xffffffff), UINT32_C(0xffffffff), UINT32_C(0x00000000),
      UINT32_C(0xffffffff), UINT32_C(0xffffffff), UINT32_C(0xffffffff),
      UINT32_C(0x7fffffff), UINT32_C(0x80000000), UINT32_C(0x80000000),
      UINT32_C(0x7fffffff), UINT32_C(0x80000000), UINT32_C(0x7ffffffe),
      UINT32_C(0x7fffffff), UINT32_C(0x00000000)}},
    {UINT32_C(0x80000000),
     UINT32_C(0x00000001),
     {UINT32_C(0x80000001), UINT32_C(0x7fffffff), UINT32_C(0x00000000),
      UINT32_C(0xffffffff), UINT32_C(0x80000001), UINT32_C(0x80000001),
      UINT32_C(0x00000001), UINT32_C(0x80000000), UINT32_C(0x80000000),
      UINT32_C(0x00000001), UINT32_C(0x00000000), UINT32_C(0x00000001),
      UINT32_C(0x7fffffff), UINT32_C(0x7fffffff)}},
    {UINT32_C(0x7fffffff),
     UINT32_C(0x00000001),
     {UINT32_C(0x80000000), UINT32_C(0x7ffffffe), UINT32_C(0x00000001),
      UINT32_C(0xfffffffe), UINT32_C(0x7fffffff), UINT32_C(0x7ffffffe),
      UINT32_C(0x7fffffff), UINT32_C(0x00000001), UINT32_C(0x7fffffff),
      UINT32_C(0x00000001), UINT32_C(0x00000000), UINT32_C(0x00000001),
      UINT32_C(0x7ffffffe), UINT32_C(0x7ffffffe)}},
    {UINT32_C(0xfffffffe),
     UINT32_C(0xffffffff),
     {UINT32_C(0xfffffffd), UINT32_C(0xffffffff), UINT32_C(0xfffffffe),
      UINT32_C(0x00000001), UINT32_C(0xffffffff), UINT32_C(0x00000001),
      UINT32_C(0xffffffff), UINT32_C(0xfffffffe), UINT32_C(0xffffffff),
      UINT32_C(0xfffffffe), UINT32_C(0xffffffff), UINT32_C(0xfffffffd),
      UINT32_C(0xfffffffe), UINT32_C(0x00000000)}},
    {UINT32_C(0x89abcdef),
     UINT32_C(0x76543210),
     {UINT32_C(0xffffffff), UINT32_C(0x13579bdf), UINT32_C(0x00000000),
      UINT32_C(0xffffffff), UINT32_C(0xffffffff), UINT32_C(0xffffffff),
      UINT32_C(0x76543210), UINT32_C(0x89abcdef), UINT32_C(0x89abcdef),
      UINT32_C(0x76543210), UINT32_C(0x00000000), UINT32_C(0x76543210),
      UINT32_C(0x13579bdf), UINT32_C(0x13579bdf)}},
};

static uint64_t (*const operations64[])(uint64_t *, uint64_t) = {
    add8, sub8,  and8,  nand8, or8,   xor8,       max8,
    min8, umax8, umin8, uinc8, udec8, usub_cond8, usub_sat8};

static const struct {
  uint64_t old, operand, stored[14];
} vectors64[] = {
    {UINT64_C(0x0000000000000000),
     UINT64_C(0x0000000000000000),
     {UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)}},
    {UINT64_C(0x0000000000000000),
     UINT64_C(0x0000000000000001),
     {UINT64_C(0x0000000000000001), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001),
      UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)}},
    {UINT64_C(0x0000000000000001),
     UINT64_C(0x0000000000000000),
     {UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001),
      UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001),
      UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001)}},
    {UINT64_C(0x0000000000000001),
     UINT64_C(0x0000000000000001),
     {UINT64_C(0x0000000000000002), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000001), UINT64_C(0xfffffffffffffffe),
      UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001),
      UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)}},
    {UINT64_C(0x0000000000000007),
     UINT64_C(0x0000000000000007),
     {UINT64_C(0x000000000000000e), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000007), UINT64_C(0xfffffffffffffff8),
      UINT64_C(0x0000000000000007), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000007), UINT64_C(0x0000000000000007),
      UINT64_C(0x0000000000000007), UINT64_C(0x0000000000000007),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000006),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)}},
    {UINT64_C(0x0000000000000006),
     UINT64_C(0x0000000000000007),
     {UINT64_C(0x000000000000000d), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x0000000000000006), UINT64_C(0xfffffffffffffff9),
      UINT64_C(0x0000000000000007), UINT64_C(0x0000000000000001),
      UINT64_C(0x0000000000000007), UINT64_C(0x0000000000000006),
      UINT64_C(0x0000000000000007), UINT64_C(0x0000000000000006),
      UINT64_C(0x0000000000000007), UINT64_C(0x0000000000000005),
      UINT64_C(0x0000000000000006), UINT64_C(0x0000000000000000)}},
    {UINT64_C(0x0000000000000008),
     UINT64_C(0x0000000000000007),
     {UINT64_C(0x000000000000000f), UINT64_C(0x0000000000000001),
      UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x000000000000000f), UINT64_C(0x000000000000000f),
      UINT64_C(0x0000000000000008), UINT64_C(0x0000000000000007),
      UINT64_C(0x0000000000000008), UINT64_C(0x0000000000000007),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000007),
      UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001)}},
    {UINT64_C(0xffffffffffffffff),
     UINT64_C(0x0000000000000000),
     {UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff)}},
    {UINT64_C(0x0000000000000000),
     UINT64_C(0xffffffffffffffff),
     {UINT64_C(0xffffffffffffffff), UINT64_C(0x0000000000000001),
      UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0x0000000000000000),
      UINT64_C(0x0000000000000001), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)}},
    {UINT64_C(0xffffffffffffffff),
     UINT64_C(0xffffffffffffffff),
     {UINT64_C(0xfffffffffffffffe), UINT64_C(0x0000000000000000),
      UINT64_C(0xffffffffffffffff), UINT64_C(0x0000000000000000),
      UINT64_C(0xffffffffffffffff), UINT64_C(0x0000000000000000),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x0000000000000000), UINT64_C(0xfffffffffffffffe),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)}},
    {UINT64_C(0x8000000000000000),
     UINT64_C(0x7fffffffffffffff),
     {UINT64_C(0xffffffffffffffff), UINT64_C(0x0000000000000001),
      UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x7fffffffffffffff), UINT64_C(0x8000000000000000),
      UINT64_C(0x8000000000000000), UINT64_C(0x7fffffffffffffff),
      UINT64_C(0x0000000000000000), UINT64_C(0x7fffffffffffffff),
      UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001)}},
    {UINT64_C(0x7fffffffffffffff),
     UINT64_C(0x8000000000000000),
     {UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x7fffffffffffffff), UINT64_C(0x8000000000000000),
      UINT64_C(0x8000000000000000), UINT64_C(0x7fffffffffffffff),
      UINT64_C(0x8000000000000000), UINT64_C(0x7ffffffffffffffe),
      UINT64_C(0x7fffffffffffffff), UINT64_C(0x0000000000000000)}},
    {UINT64_C(0x8000000000000000),
     UINT64_C(0x0000000000000001),
     {UINT64_C(0x8000000000000001), UINT64_C(0x7fffffffffffffff),
      UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x8000000000000001), UINT64_C(0x8000000000000001),
      UINT64_C(0x0000000000000001), UINT64_C(0x8000000000000000),
      UINT64_C(0x8000000000000000), UINT64_C(0x0000000000000001),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000001),
      UINT64_C(0x7fffffffffffffff), UINT64_C(0x7fffffffffffffff)}},
    {UINT64_C(0x7fffffffffffffff),
     UINT64_C(0x0000000000000001),
     {UINT64_C(0x8000000000000000), UINT64_C(0x7ffffffffffffffe),
      UINT64_C(0x0000000000000001), UINT64_C(0xfffffffffffffffe),
      UINT64_C(0x7fffffffffffffff), UINT64_C(0x7ffffffffffffffe),
      UINT64_C(0x7fffffffffffffff), UINT64_C(0x0000000000000001),
      UINT64_C(0x7fffffffffffffff), UINT64_C(0x0000000000000001),
      UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000001),
      UINT64_C(0x7ffffffffffffffe), UINT64_C(0x7ffffffffffffffe)}},
    {UINT64_C(0xfffffffffffffffe),
     UINT64_C(0xffffffffffffffff),
     {UINT64_C(0xfffffffffffffffd), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xfffffffffffffffe), UINT64_C(0x0000000000000001),
      UINT64_C(0xffffffffffffffff), UINT64_C(0x0000000000000001),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffffffffffe),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffffffffffe),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffffffffffd),
      UINT64_C(0xfffffffffffffffe), UINT64_C(0x0000000000000000)}},
    {UINT64_C(0x0123456789abcdef),
     UINT64_C(0xfedcba9876543210),
     {UINT64_C(0xffffffffffffffff), UINT64_C(0x02468acf13579bdf),
      UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x0123456789abcdef), UINT64_C(0xfedcba9876543210),
      UINT64_C(0xfedcba9876543210), UINT64_C(0x0123456789abcdef),
      UINT64_C(0x0123456789abcdf0), UINT64_C(0x0123456789abcdee),
      UINT64_C(0x0123456789abcdef), UINT64_C(0x0000000000000000)}},
    {UINT64_C(0x00000000ffffffff),
     UINT64_C(0x0000000100000000),
     {UINT64_C(0x00000001ffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x00000001ffffffff), UINT64_C(0x00000001ffffffff),
      UINT64_C(0x0000000100000000), UINT64_C(0x00000000ffffffff),
      UINT64_C(0x0000000100000000), UINT64_C(0x00000000ffffffff),
      UINT64_C(0x0000000100000000), UINT64_C(0x00000000fffffffe),
      UINT64_C(0x00000000ffffffff), UINT64_C(0x0000000000000000)}},
    {UINT64_C(0x0000000100000000),
     UINT64_C(0x00000000ffffffff),
     {UINT64_C(0x00000001ffffffff), UINT64_C(0x0000000000000001),
      UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x00000001ffffffff), UINT64_C(0x00000001ffffffff),
      UINT64_C(0x0000000100000000), UINT64_C(0x00000000ffffffff),
      UINT64_C(0x0000000100000000), UINT64_C(0x00000000ffffffff),
      UINT64_C(0x0000000000000000), UINT64_C(0x00000000ffffffff),
      UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001)}},
    {UINT64_C(0x7fffffffffffffff),
     UINT64_C(0x8000000000000000),
     {UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0x7fffffffffffffff), UINT64_C(0x8000000000000000),
      UINT64_C(0x8000000000000000), UINT64_C(0x7fffffffffffffff),
      UINT64_C(0x8000000000000000), UINT64_C(0x7ffffffffffffffe),
      UINT64_C(0x7fffffffffffffff), UINT64_C(0x0000000000000000)}},
    {UINT64_C(0x8000000000000001),
     UINT64_C(0x8000000000000000),
     {UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001),
      UINT64_C(0x8000000000000000), UINT64_C(0x7fffffffffffffff),
      UINT64_C(0x8000000000000001), UINT64_C(0x0000000000000001),
      UINT64_C(0x8000000000000001), UINT64_C(0x8000000000000000),
      UINT64_C(0x8000000000000001), UINT64_C(0x8000000000000000),
      UINT64_C(0x0000000000000000), UINT64_C(0x8000000000000000),
      UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001)}},
};

static unsigned check_rmw(void) {
  for (unsigned i = 0; i != sizeof vectors32 / sizeof *vectors32; ++i)
    for (unsigned op = 0; op != 14; ++op) {
      struct {
        uint32_t before, value, after;
      } cell = {0x2468ace0, vectors32[i].old, 0x13579bdf};
      uint32_t old = operations32[op](&cell.value, vectors32[i].operand);
      if (old != vectors32[i].old || cell.value != vectors32[i].stored[op] ||
          cell.before != 0x2468ace0 || cell.after != 0x13579bdf) {
        printf("atomics:FAIL:rmw32:%u:%u\n", i, op);
        return 1;
      }
    }
  for (unsigned i = 0; i != sizeof vectors64 / sizeof *vectors64; ++i)
    for (unsigned op = 0; op != 14; ++op) {
      struct {
        uint64_t before, value, after;
      } __attribute__((aligned(8))) cell = {UINT64_C(0x2468ace013579bdf),
                                            vectors64[i].old,
                                            UINT64_C(0x1020304050607080)};
      uint64_t old = operations64[op](&cell.value, vectors64[i].operand);
      if (old != vectors64[i].old || cell.value != vectors64[i].stored[op] ||
          cell.before != UINT64_C(0x2468ace013579bdf) ||
          cell.after != UINT64_C(0x1020304050607080)) {
        printf("atomics:FAIL:rmw64:%u:%u\n", i, op);
        return 1;
      }
    }
  return 0;
}

static unsigned check_logic(void) {
  const unsigned choices[] = {0, 1, 2, 3, 4, 0xffffffff};
  const uint32_t desired[] = {0x12345678, 0x12345678, 0xabcdef01,
                              0x12345678, 0xabcdef01, 0xabcdef01};
  for (unsigned i = 0; i != sizeof choices / sizeof *choices; ++i)
    for (unsigned match = 0; match != 2; ++match) {
      struct {
        uint32_t before, value, after;
      } cell = {0x2468ace0, 0x31415926, 0x13579bdf};
      uint32_t expected = match ? 0x31415926 : 0x31415927;
      unsigned success = cas_logic32(&cell.value, &expected, 0x12345678,
                                     0xabcdef01, choices[i]);
      if (success != match || expected != 0x31415926 ||
          cell.value != (match ? desired[i] : 0x31415926) ||
          cell.before != 0x2468ace0 || cell.after != 0x13579bdf) {
        printf("atomics:FAIL:logic:%u:%u\n", i, match);
        return 1;
      }
    }
  return 0;
}

static unsigned check_floating_bits(void) {
  const uint32_t single[][2] = {{0x00000000, 0x80000000},
                                {0x7fc12345, 0xffc54321},
                                {0x3fa00000, 0xc0200000},
                                {0x7f800000, 0xff800000},
                                {0x00000001, 0x80000001}};
  const uint64_t wide[][2] = {
      {UINT64_C(0x0000000000000000), UINT64_C(0x8000000000000000)},
      {UINT64_C(0x7ff8123456789abc), UINT64_C(0xfff8abcdef012345)},
      {UINT64_C(0x3ff4000000000000), UINT64_C(0xc004000000000000)},
      {UINT64_C(0x7ff0000000000000), UINT64_C(0xfff0000000000000)},
      {UINT64_C(0x0000000000000001), UINT64_C(0x8000000000000001)}};
  for (unsigned i = 0; i != 5; ++i)
    for (unsigned choice = 0; choice != 2; ++choice)
      for (unsigned match = 0; match != 2; ++match) {
        float a, b;
        memcpy(&a, &single[i][0], sizeof a);
        memcpy(&b, &single[i][1], sizeof b);
        struct {
          uint32_t before, value, after;
        } cell = {0x2468ace0, 0x31415926, 0x13579bdf};
        uint32_t expected = match ? 0x31415926 : 0x31415927;
        unsigned success = cas_float32(&cell.value, &expected, a, b, choice);
        if (success != match || expected != 0x31415926 ||
            cell.value != (match ? single[i][choice] : 0x31415926) ||
            cell.before != 0x2468ace0 || cell.after != 0x13579bdf) {
          printf("atomics:FAIL:float:%u:%u:%u\n", i, choice, match);
          return 1;
        }

        double da, db;
        memcpy(&da, &wide[i][0], sizeof da);
        memcpy(&db, &wide[i][1], sizeof db);
        // Deliberately four-byte aligned but not eight-byte aligned; this uses
        // the generic libatomic interface rather than its sized fast path.
        unsigned char storage[24] __attribute__((aligned(8)));
        memset(storage, 0xa5, sizeof storage);
        uint64_t initial = UINT64_C(0x3141592653589793);
        uint64_t want = match ? wide[i][choice] : initial;
        uint64_t expected64 = match ? initial : initial + 1;
        memcpy(storage + 4, &initial, 8);
        success = cas_double64(storage + 4, &expected64, da, db, choice);
        uint64_t actual;
        memcpy(&actual, storage + 4, 8);
        unsigned guards = 0;
        for (unsigned j = 0; j != sizeof storage; ++j)
          if ((j < 4 || j >= 12) && storage[j] != 0xa5)
            guards = 1;
        if (success != match || expected64 != initial || actual != want ||
            guards) {
          printf("atomics:FAIL:double:%u:%u:%u\n", i, choice, match);
          return 1;
        }
      }
  return 0;
}

int main(void) {
  if (check_rmw() || check_logic() || check_floating_bits())
    return 1;
  puts("atomics:PASS");
  return 0;
}
