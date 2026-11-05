// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#define _GNU_SOURCE
#include <assert.h>
#include <complex.h>
#include <errno.h>
#include <math.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int library_transform(int);
static jmp_buf Jump;
static volatile int State;
static volatile double Input = 0.5;

static int compare(const void *A, const void *B) {
  return (*(const int *)A > *(const int *)B) -
         (*(const int *)A < *(const int *)B);
}

static void finish(void) {
  assert(State == 1);
  puts("SH4 PICOLIBC PASS");
}

static void jump(void) { longjmp(Jump, 7); }

#define DEFINE_COMPARE(Name, Type)                                             \
  __attribute__((noinline)) static unsigned Name(Type A, Type B) {             \
    return (A == B) | ((A != B) << 1) | ((A < B) << 2) | ((A <= B) << 3) |     \
           ((A > B) << 4) | ((A >= B) << 5) |                                  \
           ((!__builtin_isunordered(A, B)) << 6) |                             \
           (__builtin_isunordered(A, B) << 7);                                 \
  }
DEFINE_COMPARE(compareFloat, float)
DEFINE_COMPARE(compareDouble, double)
#undef DEFINE_COMPARE

int main(void) {
  assert(library_transform(7) == 52);
  unsigned char Source[128], Destination[128];
  for (unsigned Index = 0; Index != sizeof(Source); ++Index)
    Source[Index] = (unsigned char)(Index * 17 + 3);
  for (unsigned From = 0; From != 8; ++From)
    for (unsigned To = 0; To != 8; ++To)
      for (unsigned Count = 0; Count != 97; ++Count) {
        memset(Destination, 0x5a, sizeof(Destination));
        assert(memcpy(Destination + To, Source + From, Count) ==
               Destination + To);
        for (unsigned Index = 0; Index != sizeof(Destination); ++Index)
          assert(Destination[Index] == (Index >= To && Index < To + Count
                                            ? Source[From + Index - To]
                                            : 0x5a));
      }
  int Values[] = {8, -5, 2, 0, 19, -3};
  qsort(Values, 6, sizeof(int), compare);
  assert(Values[0] == -5 && Values[5] == 19);
  char *Buffer = calloc(64, 1);
  assert(Buffer && Buffer[63] == 0);
  strcpy(Buffer, "memory library");
  memmove(Buffer + 2, Buffer, 15);
  assert(strcmp(Buffer + 2, "memory library") == 0);
  Buffer = realloc(Buffer, 256);
  assert(Buffer && strcmp(Buffer + 2, "memory library") == 0);
  free(Buffer);

  char Text[96];
  assert(snprintf(Text, sizeof(Text), "%lld %.3f %x", 1234567890123LL, 1.25,
                  0xabcd) > 0);
  assert(strcmp(Text, "1234567890123 1.250 abcd") == 0);
  long long Number;
  double Decimal;
  unsigned Hex;
  assert(sscanf(Text, "%lld %lf %x", &Number, &Decimal, &Hex) == 3);
  assert(Number == 1234567890123LL && Decimal == 1.25 && Hex == 0xabcd);
  errno = 0;
  assert(strtoll("9223372036854775808", NULL, 10) == INT64_MAX);
  assert(errno == ERANGE);

  double X = Input;
  volatile float FloatQuiet = __builtin_nanf("");
  volatile double DoubleQuiet = __builtin_nan("");
  uint32_t FloatSignalingBits = 0x7f800001;
  uint64_t DoubleSignalingBits = UINT64_C(0x7ff0000000000001);
  float FloatSignaling;
  double DoubleSignaling;
  memcpy(&FloatSignaling, &FloatSignalingBits, sizeof(FloatSignaling));
  memcpy(&DoubleSignaling, &DoubleSignalingBits, sizeof(DoubleSignaling));
  assert(isnan(FloatQuiet) && !issignaling(FloatQuiet));
  assert(isnan(DoubleQuiet) && !issignaling(DoubleQuiet));
  assert(isnan(FloatSignaling) && issignaling(FloatSignaling));
  assert(isnan(DoubleSignaling) && issignaling(DoubleSignaling));
  assert(compareDouble(X, X) == 105 && compareFloat((float)X, (float)X) == 105);
  assert(compareDouble(X, X + 1) == 78 &&
         compareFloat((float)X, (float)X + 1) == 78);
  assert(compareDouble(X + 1, X) == 114 &&
         compareFloat((float)X + 1, (float)X) == 114);
  assert(compareDouble(DoubleQuiet, X) == 130 &&
         compareFloat(FloatQuiet, (float)X) == 130);
  assert(compareDouble(X, DoubleQuiet) == 130 &&
         compareFloat((float)X, FloatQuiet) == 130);
  assert(compareDouble(0.0, -0.0) == 105 && compareFloat(0.0f, -0.0f) == 105);
  assert(compareDouble(INFINITY, X) == 114 &&
         compareFloat(INFINITY, (float)X) == 114);
  assert(compareDouble(-INFINITY, X) == 78 &&
         compareFloat(-INFINITY, (float)X) == 78);
  double Invalid = sqrt(-X);
  assert(isnan(Invalid) && !issignaling(Invalid));
  float InvalidFloat = sqrtf(-(float)X);
  assert(isnan(InvalidFloat) && !issignaling(InvalidFloat));
  assert(fabs(sin(X) - 0.479425538604203) < 1e-12);
  assert(fabs(cos(X) - 0.877582561890373) < 1e-12);
  assert(fabs(sqrt(X + 1.75) - 1.5) < 1e-12);
  assert(fabs(pow(X + 1.5, 10) - 1024.0) < 1e-10);
  assert(fabs(log(exp(X)) - X) < 1e-12);
  double complex Z = X + 2.0 * I;
  double complex Product = Z * (3.0 + 4.0 * I);
  assert(creal(Product) == -6.5 && cimag(Product) == 8.0);
  double complex Quotient = Product / Z;
  assert(fabs(creal(Quotient) - 3.0) < 1e-12);
  assert(fabs(cimag(Quotient) - 4.0) < 1e-12);
  assert(fabs(cabs(Z) - sqrt(4.25)) < 1e-12);
  float complex F = (float)X + 2.0f * I;
  float complex FP = F * (3.0f + 4.0f * I);
  assert(crealf(FP) == -6.5f && cimagf(FP) == 8.0f);
  float complex FQ = FP / F;
  assert(fabsf(crealf(FQ) - 3.0f) < 1e-5f);
  assert(fabsf(cimagf(FQ) - 4.0f) < 1e-5f);

  char FileData[64] = {0};
  FILE *File = fmemopen(FileData, sizeof(FileData), "w+");
  assert(File && fprintf(File, "%s:%d", "ram", 42) == 6);
  assert(fflush(File) == 0 && fseek(File, 0, SEEK_SET) == 0);
  memset(Text, 0, sizeof(Text));
  assert(fread(Text, 1, 6, File) == 6 && strcmp(Text, "ram:42") == 0);
  assert(fclose(File) == 0);
  int Result = setjmp(Jump);
  if (Result == 0)
    jump();
  assert(Result == 7);
  assert(atexit(finish) == 0);
  State = 1;
  return 0;
}
