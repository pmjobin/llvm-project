// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Executes the existing generate-i64-compare-slot-consumers.py witnesses.
// Predicate masks are literal unsigned/signed mathematical comparisons,
// calculated outside the compiler under test. Operations use a separate TU.
#include <stdint.h>
#include <stdio.h>

typedef uint64_t (*Wide)(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                         void *);
typedef unsigned (*Word)(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                         void *);
extern uint64_t select_ult(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern uint64_t branch_ult(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern unsigned boolean_ult(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern unsigned accumulate_ult(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                               void *);
extern uint64_t multiple_ult(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                             void *);
extern uint64_t phi_ult(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                        void *);
extern uint64_t clobber_ult(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern uint64_t select_ule(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern uint64_t branch_ule(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern unsigned boolean_ule(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern unsigned accumulate_ule(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                               void *);
extern uint64_t multiple_ule(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                             void *);
extern uint64_t phi_ule(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                        void *);
extern uint64_t clobber_ule(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern uint64_t select_ugt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern uint64_t branch_ugt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern unsigned boolean_ugt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern unsigned accumulate_ugt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                               void *);
extern uint64_t multiple_ugt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                             void *);
extern uint64_t phi_ugt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                        void *);
extern uint64_t clobber_ugt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern uint64_t select_uge(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern uint64_t branch_uge(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern unsigned boolean_uge(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern unsigned accumulate_uge(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                               void *);
extern uint64_t multiple_uge(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                             void *);
extern uint64_t phi_uge(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                        void *);
extern uint64_t clobber_uge(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern uint64_t select_slt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern uint64_t branch_slt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern unsigned boolean_slt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern unsigned accumulate_slt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                               void *);
extern uint64_t multiple_slt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                             void *);
extern uint64_t phi_slt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                        void *);
extern uint64_t clobber_slt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern uint64_t select_sle(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern uint64_t branch_sle(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern unsigned boolean_sle(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern unsigned accumulate_sle(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                               void *);
extern uint64_t multiple_sle(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                             void *);
extern uint64_t phi_sle(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                        void *);
extern uint64_t clobber_sle(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern uint64_t select_sgt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern uint64_t branch_sgt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern unsigned boolean_sgt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern unsigned accumulate_sgt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                               void *);
extern uint64_t multiple_sgt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                             void *);
extern uint64_t phi_sgt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                        void *);
extern uint64_t clobber_sgt(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern uint64_t select_sge(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern uint64_t branch_sge(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern unsigned boolean_sge(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern unsigned accumulate_sge(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                               void *);
extern uint64_t multiple_sge(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                             void *);
extern uint64_t phi_sge(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                        void *);
extern uint64_t clobber_sge(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern uint64_t select_eq(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                          void *);
extern uint64_t branch_eq(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                          void *);
extern unsigned boolean_eq(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern unsigned accumulate_eq(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                              void *);
extern uint64_t multiple_eq(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern uint64_t phi_eq(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                       void *);
extern uint64_t clobber_eq(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern uint64_t select_ne(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                          void *);
extern uint64_t branch_ne(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                          void *);
extern unsigned boolean_ne(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);
extern unsigned accumulate_ne(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                              void *);
extern uint64_t multiple_ne(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                            void *);
extern uint64_t phi_ne(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                       void *);
extern uint64_t clobber_ne(uint64_t, uint64_t, uint64_t, uint64_t, unsigned,
                           void *);

static const struct {
  const char *Name;
  Wide Select, Branch, Multiple, Phi, Clobber;
  Word Boolean, Accumulate;
} Operations[] = {
    {"ult", select_ult, branch_ult, multiple_ult, phi_ult, clobber_ult,
     boolean_ult, accumulate_ult},
    {"ule", select_ule, branch_ule, multiple_ule, phi_ule, clobber_ule,
     boolean_ule, accumulate_ule},
    {"ugt", select_ugt, branch_ugt, multiple_ugt, phi_ugt, clobber_ugt,
     boolean_ugt, accumulate_ugt},
    {"uge", select_uge, branch_uge, multiple_uge, phi_uge, clobber_uge,
     boolean_uge, accumulate_uge},
    {"slt", select_slt, branch_slt, multiple_slt, phi_slt, clobber_slt,
     boolean_slt, accumulate_slt},
    {"sle", select_sle, branch_sle, multiple_sle, phi_sle, clobber_sle,
     boolean_sle, accumulate_sle},
    {"sgt", select_sgt, branch_sgt, multiple_sgt, phi_sgt, clobber_sgt,
     boolean_sgt, accumulate_sgt},
    {"sge", select_sge, branch_sge, multiple_sge, phi_sge, clobber_sge,
     boolean_sge, accumulate_sge},
    {"eq", select_eq, branch_eq, multiple_eq, phi_eq, clobber_eq, boolean_eq,
     accumulate_eq},
    {"ne", select_ne, branch_ne, multiple_ne, phi_ne, clobber_ne, boolean_ne,
     accumulate_ne},
};
static const struct {
  uint64_t A, B;
  unsigned Predicates;
} Pairs[] = {
    {UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), 0x1aa},
    {UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001), 0x1aa},
    {UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff), 0x1aa},
    {UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000001), 0x233},
    {UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000000), 0x2cc},
    {UINT64_C(0x00000000ffffffff), UINT64_C(0x0000000100000000), 0x233},
    {UINT64_C(0x0000000100000000), UINT64_C(0x00000000ffffffff), 0x2cc},
    {UINT64_C(0x0000000100000000), UINT64_C(0x0000000100000001), 0x233},
    {UINT64_C(0x0000000100000001), UINT64_C(0x0000000100000000), 0x2cc},
    {UINT64_C(0x123456787fffffff), UINT64_C(0x1234567880000000), 0x233},
    {UINT64_C(0x1234567880000000), UINT64_C(0x123456787fffffff), 0x2cc},
    {UINT64_C(0x12345678ffffffff), UINT64_C(0x1234567900000000), 0x233},
    {UINT64_C(0x1234567900000000), UINT64_C(0x12345678ffffffff), 0x2cc},
    {UINT64_C(0x7fffffffffffffff), UINT64_C(0x8000000000000000), 0x2c3},
    {UINT64_C(0x8000000000000000), UINT64_C(0x7fffffffffffffff), 0x23c},
    {UINT64_C(0x8000000000000000), UINT64_C(0x0000000000000000), 0x23c},
    {UINT64_C(0x0000000000000000), UINT64_C(0x8000000000000000), 0x2c3},
    {UINT64_C(0xffffffffffffffff), UINT64_C(0x0000000000000000), 0x23c},
    {UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff), 0x2c3},
    {UINT64_C(0x800000007fffffff), UINT64_C(0x8000000080000000), 0x233},
    {UINT64_C(0x8000000080000000), UINT64_C(0x800000007fffffff), 0x2cc},
    {UINT64_C(0xffffffff00000000), UINT64_C(0xfffffffeffffffff), 0x2cc},
    {UINT64_C(0xfffffffeffffffff), UINT64_C(0xffffffff00000000), 0x233},
    {UINT64_C(0xfffffffffffffffe), UINT64_C(0xffffffffffffffff), 0x233},
    {UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffffffffffe), 0x2cc},
    {UINT64_C(0x8000000000000000), UINT64_C(0x8000000000000000), 0x1aa},
};
extern unsigned equal_high_low_boundary(void *);
extern unsigned signed_min_max(void *);
extern unsigned unsigned_max(void *);
extern unsigned threshold_equal(void *);

#define CHECK(C, Form)                                                         \
  do {                                                                         \
    if (!(C)) {                                                                \
      fprintf(stderr, "i64 relation %s/%s pair %u failed at %d\n",             \
              Operations[P].Name, Form, I, __LINE__);                          \
      return 1;                                                                \
    }                                                                          \
  } while (0)

int main(void) {
  const uint64_t X = UINT64_C(0x0123456789abcdef);
  const uint64_t Y = UINT64_C(0xfedcba9876543210);
  for (unsigned P = 0; P != sizeof(Operations) / sizeof(Operations[0]); ++P) {
    for (unsigned I = 0; I != sizeof(Pairs) / sizeof(Pairs[0]); ++I) {
      const uint64_t A = Pairs[I].A, B = Pairs[I].B;
      const unsigned Bit = (Pairs[I].Predicates >> P) & 1;
      const uint64_t Expected = Bit ? X : Y;
      const unsigned Accumulated = Bit ? 0 : UINT32_MAX;
      uint64_t WideStore = UINT64_C(0xa5a5a5a55a5a5a5a);
      unsigned WordStore = 0xc001face;
      CHECK(Operations[P].Select(A, B, X, Y, UINT32_MAX, &WideStore) ==
                Expected,
            "select");
      CHECK(Operations[P].Branch(A, B, X, Y, UINT32_MAX, &WideStore) ==
                Expected,
            "branch");
      CHECK(Operations[P].Boolean(A, B, X, Y, UINT32_MAX, &WideStore) == Bit,
            "boolean");
      CHECK(Operations[P].Accumulate(A, B, X, Y, UINT32_MAX, &WideStore) ==
                Accumulated,
            "accumulate");
      CHECK(Operations[P].Multiple(A, B, X, Y, UINT32_MAX, &WordStore) ==
                Expected,
            "multiple-value");
      CHECK(WordStore == Bit, "multiple-store");
      CHECK(Operations[P].Phi(A, B, X, Y, UINT32_MAX, &WideStore) == Expected,
            "phi-value");
      CHECK(WideStore == Expected, "phi-store");
      CHECK(Operations[P].Clobber(A, B, X, Y, UINT32_MAX, &WideStore) ==
                Expected,
            "T-bit-clobber");
    }
  }
  uint64_t Storage = 0;
  if (equal_high_low_boundary(&Storage) != 1 || signed_min_max(&Storage) != 1 ||
      unsigned_max(&Storage) != 1 || threshold_equal(&Storage) != 1) {
    fputs("i64 literal boundary failed\n", stderr);
    return 1;
  }
  puts("i64-relations:PASS");
  return 0;
}
