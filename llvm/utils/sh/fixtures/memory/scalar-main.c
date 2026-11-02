// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// GNU C oracle for the exact scalar and aggregate memory IR producers.
// Runtime scope is SH4 little-endian.
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

typedef uint32_t (*Load32)(void *);
typedef uint64_t (*Load64)(void *);
typedef void (*Store32)(void *, uint32_t);
typedef void (*Store64)(void *, uint64_t);

uint32_t load_u24(void *);
uint32_t load_s24(void *);
void store_u24(void *, uint32_t);
uint32_t load_u24_a4(void *);
uint32_t load_s24_a4(void *);
void store_u24_a4(void *, uint32_t);
uint64_t load_u40(void *);
uint64_t load_s40(void *);
void store_u40(void *, uint64_t);
uint64_t load_u40_a4(void *);
uint64_t load_s40_a4(void *);
void store_u40_a4(void *, uint64_t);
uint64_t load_u48(void *);
uint64_t load_s48(void *);
void store_u48(void *, uint64_t);
uint64_t load_u48_a4(void *);
uint64_t load_s48_a4(void *);
void store_u48_a4(void *, uint64_t);
uint64_t load_u56(void *);
uint64_t load_s56(void *);
void store_u56(void *, uint64_t);
uint64_t load_u56_a4(void *);
uint64_t load_s56_a4(void *);
void store_u56_a4(void *, uint64_t);
uint32_t load_i16_a1(void *);
uint32_t load_s16_a1(void *);
void store_i16_a1(void *, uint32_t);
uint32_t load_i16_aligned(void *);
void store_i16_aligned(void *, uint32_t);
uint32_t load_i32_a1(void *);
void store_i32_a1(void *, uint32_t);
uint32_t load_i32_a2(void *);
void store_i32_a2(void *, uint32_t);
uint32_t load_i32_aligned(void *);
void store_i32_aligned(void *, uint32_t);
uint64_t load_i64_a1(void *);
void store_i64_a1(void *, uint64_t);
uint64_t load_i64_a2(void *);
void store_i64_a2(void *, uint64_t);
uint64_t load_i64_aligned(void *);
void store_i64_aligned(void *, uint64_t);
uint32_t volatile_i16_rmw(void *);
uint32_t volatile_i32_rmw(void *);
uint64_t volatile_i64_rmw(void *);
void store_i16_pattern(void *);
void store_i32_pattern(void *);
void store_i16_zero(void *);
void store_i32_ones(void *);
void store_i16_high_bit(void *);
uint32_t load_i16_boundary(void);
uint32_t load_i32_boundary(void);
uint64_t load_i64_boundary(void);
void metadata_i32(void *, void *);
void copy_aggregate_i1(void *, void *);
void copy_two(void *, void *);
void copy_mixed(void *, void *);
void copy_nested(void *, void *);
void copy_packed(void *, void *);
void copy_padded(void *, void *);
void pointer_derived(void *, void *);
void select_store(uint32_t, void *);
uint32_t local_round_trip(uint32_t);
uint32_t global_round_trip(uint32_t);
extern unsigned char global_s1;

unsigned char two_bytes[2] = {0x34, 0x92};
unsigned char four_bytes[4] = {0x78, 0x56, 0x34, 0xf2};
unsigned char eight_bytes[8] = {0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x81};

static const uint64_t Masks[] = {
    0,
    UINT64_C(0xff),
    UINT64_C(0xffff),
    UINT64_C(0xffffff),
    UINT64_C(0xffffffff),
    UINT64_C(0xffffffffff),
    UINT64_C(0xffffffffffff),
    UINT64_C(0xffffffffffffff),
    UINT64_MAX,
};

static const struct Scalar {
  const char *Name;
  unsigned Bytes, Alignment;
  Load32 Unsigned32, Signed32;
  Store32 Write32;
  Load64 Unsigned64, Signed64;
  Store64 Write64;
} Scalars[] = {
    {"i24-a1", 3, 1, load_u24, load_s24, store_u24, 0, 0, 0},
    {"i24-a4", 3, 4, load_u24_a4, load_s24_a4, store_u24_a4, 0, 0, 0},
    {"i40-a1", 5, 1, 0, 0, 0, load_u40, load_s40, store_u40},
    {"i40-a4", 5, 4, 0, 0, 0, load_u40_a4, load_s40_a4, store_u40_a4},
    {"i48-a1", 6, 1, 0, 0, 0, load_u48, load_s48, store_u48},
    {"i48-a4", 6, 4, 0, 0, 0, load_u48_a4, load_s48_a4, store_u48_a4},
    {"i56-a1", 7, 1, 0, 0, 0, load_u56, load_s56, store_u56},
    {"i56-a4", 7, 4, 0, 0, 0, load_u56_a4, load_s56_a4, store_u56_a4},
    {"i16-a1", 2, 1, load_i16_a1, load_s16_a1, store_i16_a1, 0, 0, 0},
    {"i16-a2", 2, 2, load_i16_aligned, 0, store_i16_aligned, 0, 0, 0},
    {"i32-a1", 4, 1, load_i32_a1, 0, store_i32_a1, 0, 0, 0},
    {"i32-a2", 4, 2, load_i32_a2, 0, store_i32_a2, 0, 0, 0},
    {"i32-a4", 4, 4, load_i32_aligned, 0, store_i32_aligned, 0, 0, 0},
    {"i64-a1", 8, 1, 0, 0, 0, load_i64_a1, 0, store_i64_a1},
    {"i64-a2", 8, 2, 0, 0, 0, load_i64_a2, 0, store_i64_a2},
    {"i64-a4", 8, 4, 0, 0, 0, load_i64_aligned, 0, store_i64_aligned},
};

static const struct RMW {
  const char *Name;
  unsigned Bytes;
  Load32 Word;
  Load64 Wide;
} VolatileRMW[] = {
    {"volatile-i16", 2, volatile_i16_rmw, 0},
    {"volatile-i32", 4, volatile_i32_rmw, 0},
    {"volatile-i64", 8, 0, volatile_i64_rmw},
};

static const struct FixedStore {
  const char *Name;
  unsigned Bytes, Alignment;
  uint64_t Value;
  void (*Store)(void *);
} FixedStores[] = {
    {"i16-pattern", 2, 1, UINT64_C(0x1234), store_i16_pattern},
    {"i32-pattern", 4, 1, UINT64_C(0x12345678), store_i32_pattern},
    {"i16-zero", 2, 1, 0, store_i16_zero},
    {"i32-ones", 4, 2, UINT64_C(0xffffffff), store_i32_ones},
    {"i16-high-bit", 2, 1, UINT64_C(0x8000), store_i16_high_bit},
};

static int failure(const char *Name, const char *Operation, size_t Offset,
                   uint64_t Value) {
  fprintf(stderr, "scalar-memory:%s:%s:offset=%zu:value=%016" PRIx64 "\n", Name,
          Operation, Offset, Value);
  return 1;
}

static void initialize(unsigned char *Actual, unsigned char *Expected,
                       size_t Length) {
  for (size_t I = 0; I != Length; ++I)
    Actual[I] = Expected[I] = (unsigned char)(I * 37 + 0xa5);
}

static void put_bytes(unsigned char *Where, unsigned Bytes, uint64_t Value) {
  // Byte arithmetic in the GNU caller is independent of SH scalar lowering.
  for (unsigned I = 0; I != Bytes; ++I) {
    Where[I] = (unsigned char)Value;
    Value >>= 8;
  }
}

static uint64_t pattern(unsigned Index, unsigned Bytes) {
  uint64_t Sign = (Masks[Bytes] >> 1) + 1;
  const uint64_t Values[] = {0,
                             UINT64_MAX,
                             Sign,
                             Sign - 1,
                             UINT64_C(0x80ff7f0181234567),
                             UINT64_C(0x7f0180ff76543210)};
  return Values[Index];
}

static int scalar(const struct Scalar *S, unsigned char *Actual,
                  unsigned char *Expected, size_t Length, size_t Offset,
                  uint64_t Input) {
  uint64_t Value = Input & Masks[S->Bytes];
  uint64_t Sign = (Masks[S->Bytes] >> 1) + 1;
  uint64_t Signed = Value & Sign ? Value | ~Masks[S->Bytes] : Value;
  initialize(Actual, Expected, Length);
  put_bytes(Actual + Offset, S->Bytes, Value);
  memcpy(Expected, Actual, Length);
  if ((S->Unsigned32 && S->Unsigned32(Actual + Offset) != Value) ||
      (S->Unsigned64 && S->Unsigned64(Actual + Offset) != Value))
    return failure(S->Name, "zero-load", Offset, Input);
  if ((S->Signed32 && S->Signed32(Actual + Offset) != (uint32_t)Signed) ||
      (S->Signed64 && S->Signed64(Actual + Offset) != Signed))
    return failure(S->Name, "sign-load", Offset, Input);
  if (memcmp(Actual, Expected, Length))
    return failure(S->Name, "load-guards", Offset, Input);

  initialize(Actual, Expected, Length);
  put_bytes(Expected + Offset, S->Bytes, Value);
  if (S->Write32)
    S->Write32(Actual + Offset, (uint32_t)Input);
  else
    S->Write64(Actual + Offset, Input);
  if (memcmp(Actual, Expected, Length))
    return failure(S->Name, "store-and-guards", Offset, Input);
  return 0;
}

static int rmw(const struct RMW *R, unsigned char *Actual,
               unsigned char *Expected, size_t Length, size_t Offset,
               uint64_t Input) {
  uint64_t Value = Input & Masks[R->Bytes];
  uint64_t Next = Value + 1;
  if (R->Word)
    Next = (uint32_t)Next;
  initialize(Actual, Expected, Length);
  put_bytes(Actual + Offset, R->Bytes, Value);
  put_bytes(Expected + Offset, R->Bytes, Next);
  uint64_t Returned =
      R->Word ? R->Word(Actual + Offset) : R->Wide(Actual + Offset);
  if (Returned != Next)
    return failure(R->Name, "return", Offset, Input);
  if (memcmp(Actual, Expected, Length))
    return failure(R->Name, "store-and-guards", Offset, Input);
  return 0;
}

static int fixed_store(const struct FixedStore *S, unsigned char *Actual,
                       unsigned char *Expected, size_t Length, size_t Offset) {
  initialize(Actual, Expected, Length);
  put_bytes(Expected + Offset, S->Bytes, S->Value);
  S->Store(Actual + Offset);
  if (memcmp(Actual, Expected, Length))
    return failure(S->Name, "store-and-guards", Offset, S->Value);
  return 0;
}

static void copy_one(void *Destination, void *Source) {
  copy_aggregate_i1(Source, Destination);
}

static int aggregates(void) {
  const struct {
    const char *Name;
    unsigned Bytes, Alignment, BooleanBytes, CopiedBytes;
    void (*Copy)(void *, void *);
  } Operations[] = {
      {"one-i1", 1, 1, 0x001, 0x001, copy_one},
      {"two-i1", 2, 1, 0x003, 0x003, copy_two},
      {"mixed", 8, 4, 0x001, 0x0ff, copy_mixed},
      {"nested", 4, 1, 0x00f, 0x00f, copy_nested},
      {"packed", 3, 1, 0x001, 0x007, copy_packed},
      {"padded", 12, 4, 0x000, 0xff1, copy_padded},
  };
  unsigned char Actual[64] __attribute__((aligned(8)));
  unsigned char Source[64] __attribute__((aligned(8)));
  unsigned char Expected[64], Snapshot[64];
  for (unsigned O = 0; O != sizeof(Operations) / sizeof(Operations[0]); ++O)
    for (unsigned Offset = 17; Offset != 25; ++Offset) {
      if (Offset % Operations[O].Alignment)
        continue;
      for (unsigned P = 0; P != 6; ++P) {
        initialize(Actual, Expected, sizeof(Actual));
        initialize(Source, Snapshot, sizeof(Source));
        for (unsigned I = 0; I != Operations[O].Bytes; ++I) {
          unsigned char Byte = (unsigned char)(P * 43 + I * 37 + 0x91);
          // A load of i1 is poison if discarded bits are nonzero. Supply
          // canonical bytes and verify the complete bytes written by stores.
          if (Operations[O].BooleanBytes & (1u << I))
            Byte &= 1;
          Source[Offset + I] = Byte;
          if (Operations[O].CopiedBytes & (1u << I))
            Expected[Offset + I] = Byte;
        }
        memcpy(Snapshot, Source, sizeof(Source));
        Operations[O].Copy(Actual + Offset, Source + Offset);
        // The padded case also checks the current native field-store shape:
        // bytes 1..3 stay untouched. General LLVM aggregate stores may leave
        // padding undefined; this is a field-access regression contract.
        if (memcmp(Actual, Expected, sizeof(Actual)) ||
            memcmp(Source, Snapshot, sizeof(Source)))
          return failure(Operations[O].Name, "copy-fields-and-guards", Offset,
                         P);
      }
    }
  for (unsigned Offset = 17; Offset != 25; ++Offset)
    for (unsigned P = 0; P != 6; ++P) {
      initialize(Actual, Expected, sizeof(Actual));
      initialize(Source, Snapshot, sizeof(Source));
      Source[Offset + 3] = P & 1;
      Expected[Offset + 5] = P & 1;
      memcpy(Snapshot, Source, sizeof(Source));
      pointer_derived(Actual + Offset, Source + Offset);
      if (memcmp(Actual, Expected, sizeof(Actual)) ||
          memcmp(Source, Snapshot, sizeof(Source)))
        return failure("pointer-derived-i1", "offsets-and-guards", Offset, P);
    }
  const uint32_t Choices[] = {0, 1, UINT32_MAX, UINT32_C(0x80000000)};
  for (unsigned C = 0; C != sizeof(Choices) / sizeof(Choices[0]); ++C) {
    uint32_t ExpectedBoolean = Choices[C] != 0;
    if (local_round_trip(Choices[C]) != ExpectedBoolean ||
        global_round_trip(Choices[C]) != ExpectedBoolean ||
        global_s1 != ExpectedBoolean)
      return failure("i1-round-trip", "normalized-result", 0, Choices[C]);
    for (unsigned Offset = 17; Offset != 25; ++Offset) {
      initialize(Actual, Expected, sizeof(Actual));
      Expected[Offset] = ExpectedBoolean;
      Expected[Offset + 1] = !ExpectedBoolean;
      select_store(Choices[C], Actual + Offset);
      if (memcmp(Actual, Expected, sizeof(Actual)))
        return failure("i1-select", "normalized-fields-and-guards", Offset,
                       Choices[C]);
    }
  }
  return 0;
}

int main(void) {
  unsigned char Actual[64] __attribute__((aligned(8)));
  unsigned char Expected[64];
  // Exercise every high-byte value through the actual native align-1 load.
  // Distinct lower bytes detect accidental changes to the other pieces.
  for (unsigned High = 0; High != 256; ++High)
    for (unsigned Offset = 17; Offset != 25; ++Offset) {
      uint32_t Value = ((uint32_t)High << 24) | UINT32_C(0x006ca735);
      initialize(Actual, Expected, sizeof(Actual));
      put_bytes(Actual + Offset, 4, Value);
      memcpy(Expected, Actual, sizeof(Actual));
      if (load_i32_a1(Actual + Offset) != Value)
        return failure("i32-high-byte", "load", Offset, Value);
      if (memcmp(Actual, Expected, sizeof(Actual)))
        return failure("i32-high-byte", "load-guards", Offset, Value);
    }
  for (unsigned S = 0; S != sizeof(Scalars) / sizeof(Scalars[0]); ++S)
    for (unsigned Offset = 17; Offset != 25; ++Offset) {
      if (Offset % Scalars[S].Alignment)
        continue;
      for (unsigned P = 0; P != 6; ++P)
        if (scalar(&Scalars[S], Actual, Expected, sizeof(Actual), Offset,
                   pattern(P, Scalars[S].Bytes)))
          return 1;
    }
  for (unsigned R = 0; R != sizeof(VolatileRMW) / sizeof(VolatileRMW[0]); ++R)
    for (unsigned Offset = 17; Offset != 25; ++Offset)
      for (unsigned P = 0; P != 6; ++P)
        if (rmw(&VolatileRMW[R], Actual, Expected, sizeof(Actual), Offset,
                pattern(P, VolatileRMW[R].Bytes)))
          return 1;
  for (unsigned S = 0; S != sizeof(FixedStores) / sizeof(FixedStores[0]); ++S)
    for (unsigned Offset = 17; Offset != 25; ++Offset)
      if (Offset % FixedStores[S].Alignment == 0 &&
          fixed_store(&FixedStores[S], Actual, Expected, sizeof(Actual),
                      Offset))
        return 1;

  // Only align-1 scalar functions are called at exact page ends: placing an
  // odd-width value there cannot satisfy an align-4 IR promise. Both adjacent
  // pages stay inaccessible, detecting reads and writes outside the object.
  long PageSize = sysconf(_SC_PAGESIZE);
  if (PageSize < 64)
    return failure("mapping", "page-size", 0, PageSize);
  unsigned char *Mapping = mmap(0, 3 * (size_t)PageSize, PROT_NONE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (Mapping == MAP_FAILED) {
    perror("scalar-memory:mmap");
    return 1;
  }
  unsigned char *Page = Mapping + PageSize;
  unsigned char *PageExpected = malloc(PageSize);
  if (!PageExpected || mprotect(Page, PageSize, PROT_READ | PROT_WRITE)) {
    perror("scalar-memory:page-setup");
    return 1;
  }
  for (unsigned S = 0; S != sizeof(Scalars) / sizeof(Scalars[0]); ++S) {
    if (Scalars[S].Alignment != 1)
      continue;
    for (unsigned End = 0; End != 2; ++End)
      for (unsigned P = 0; P != 6; ++P) {
        size_t Offset = End ? PageSize - Scalars[S].Bytes : 0;
        if (scalar(&Scalars[S], Page, PageExpected, PageSize, Offset,
                   pattern(P, Scalars[S].Bytes)))
          return 1;
      }
  }
  for (unsigned R = 0; R != sizeof(VolatileRMW) / sizeof(VolatileRMW[0]); ++R)
    for (unsigned End = 0; End != 2; ++End)
      for (unsigned P = 0; P != 6; ++P) {
        size_t Offset = End ? PageSize - VolatileRMW[R].Bytes : 0;
        if (rmw(&VolatileRMW[R], Page, PageExpected, PageSize, Offset,
                pattern(P, VolatileRMW[R].Bytes)))
          return 1;
      }
  for (unsigned S = 0; S != sizeof(FixedStores) / sizeof(FixedStores[0]); ++S)
    for (unsigned End = 0; End != 2; ++End) {
      size_t Offset = End ? PageSize - FixedStores[S].Bytes : 0;
      if (fixed_store(&FixedStores[S], Page, PageExpected, PageSize, Offset))
        return 1;
    }
  if (munmap(Mapping, 3 * (size_t)PageSize)) {
    perror("scalar-memory:munmap");
    return 1;
  }
  free(PageExpected);

  if (load_i16_boundary() != UINT32_C(0x9234) ||
      load_i32_boundary() != UINT32_C(0xf2345678) ||
      load_i64_boundary() != UINT64_C(0x8123456789abcdef))
    return failure("global", "load", 0, 0);

  unsigned char Source[64], Snapshot[64];
  for (unsigned Offset = 17; Offset != 25; ++Offset)
    for (unsigned P = 0; P != 6; ++P) {
      initialize(Actual, Expected, sizeof(Actual));
      initialize(Source, Snapshot, sizeof(Source));
      put_bytes(Source + Offset + 1, 4, pattern(P, 4));
      memcpy(Snapshot, Source, sizeof(Source));
      memcpy(Expected + Offset, Source + Offset + 1, 4);
      // Distinct arrays honor the producer's alias.scope/noalias metadata.
      metadata_i32(Actual + Offset, Source + Offset + 1);
      if (memcmp(Actual, Expected, sizeof(Actual)) ||
          memcmp(Source, Snapshot, sizeof(Source)))
        return failure("metadata", "copy-and-guards", Offset, pattern(P, 4));
    }
  if (aggregates())
    return 1;
  puts("scalar-memory:PASS");
  return 0;
}
