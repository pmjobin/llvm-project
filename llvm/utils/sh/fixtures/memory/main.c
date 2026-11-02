// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void copy1(void *, const void *, unsigned);
void copy2(void *, const void *, unsigned);
void copy4(void *, const void *, unsigned);
void move1(void *, const void *, unsigned);
void move2(void *, const void *, unsigned);
void move4(void *, const void *, unsigned);
void set1(void *, const void *, unsigned);
void set2(void *, const void *, unsigned);
void set4(void *, const void *, unsigned);
void copy_mixed(void *, const void *, unsigned);
void volatile_copy(void *, const void *, unsigned);
void volatile_move(void *, const void *, unsigned);
void volatile_set(void *, const void *, unsigned);
void link_nodes(unsigned char *);

static unsigned char Source[320] __attribute__((aligned(4)));
static unsigned char Actual[320] __attribute__((aligned(4)));
static unsigned char Expected[320] __attribute__((aligned(4)));

int main(void) {
  void (*Operations[3][3])(void *, const void *, unsigned) = {
      {copy1, copy2, copy4}, {move1, move2, move4}, {set1, set2, set4}};
  const unsigned Lengths[] = {0, 1, 4, 5, 8, 9, 16, 17, 31, 32, 33,
                             56, 60, 61, 128, 139};
  for (unsigned K = 0; K != 3; ++K)
    for (unsigned A = 0; A != 3; ++A)
      for (unsigned L = 0; L != sizeof(Lengths) / sizeof(Lengths[0]); ++L)
        for (unsigned Direction = 0; Direction != 2; ++Direction) {
          unsigned Alignment = 1u << A;
          unsigned D = 16 + (Direction ? Alignment : 2 * Alignment);
          unsigned S = 16 + (Direction ? 2 * Alignment : Alignment);
          unsigned N = Lengths[L];
          for (unsigned I = 0; I != sizeof(Source); ++I) {
            Source[I] = (I * 37 + 13) & 255;
            Actual[I] = Expected[I] = (I * 17 + 29) & 255;
          }
          // Snapshot the source before overlapping movement. The oracle uses
          // only the GNU-compiled caller and includes both surrounding guards.
          unsigned char Snapshot[320];
          for (unsigned I = 0; I != N; ++I)
            Snapshot[I] = K == 1 ? Actual[S + I] : Source[S + I];
          for (unsigned I = 0; I != N; ++I)
            Expected[D + I] = K == 2 ? 0xa5 : Snapshot[I];
          Operations[K][A](Actual + D, K == 1 ? Actual + S : Source + S, N);
          if (memcmp(Actual, Expected, sizeof(Actual))) {
            fprintf(stderr, "memory:%u:%u:%u:%u\n", K, A, N, Direction);
            return 1;
          }
        }
  memset(Actual, 0, sizeof(Actual));
  memset(Expected, 0, sizeof(Expected));
  for (unsigned I = 0; I != 56; ++I)
    Expected[16 + I] = Source[17 + I];
  copy_mixed(Actual + 16, Source + 17, 56);
  if (memcmp(Actual, Expected, sizeof(Actual)))
    return 2;
  void (*Volatile[3])(void *, const void *, unsigned) = {
      volatile_copy, volatile_move, volatile_set};
  for (unsigned K = 0; K != 3; ++K)
    for (unsigned Direction = 0; Direction != 2; ++Direction) {
      unsigned D = Direction ? 17 : 19;
      unsigned S = Direction ? 19 : 17;
      unsigned N = K == 1 ? 16 : 60;
      unsigned char Snapshot[60];
      for (unsigned I = 0; I != sizeof(Actual); ++I)
        Actual[I] = Expected[I] = (I * 17 + 29) & 255;
      for (unsigned I = 0; I != N; ++I)
        Snapshot[I] = K == 1 ? Actual[S + I] : Source[S + I];
      for (unsigned I = 0; I != N; ++I)
        Expected[D + I] = K == 2 ? 0xa5 : Snapshot[I];
      Volatile[K](Actual + D, K == 1 ? Actual + S : Source + S, N);
      if (memcmp(Actual, Expected, sizeof(Actual)))
        return 3;
    }
  // malloc storage has no declared type; pointer stores may establish it.
  unsigned char *Nodes = malloc(73 * 56);
  if (!Nodes)
    return 4;
  memset(Nodes, 0x5a, 73 * 56);
  link_nodes(Nodes);
  for (unsigned I = 0; I != 73; ++I) {
    void *Next;
    memcpy(&Next, Nodes + 56 * I, sizeof(Next));
    if (Next != (I == 72 ? 0 : Nodes + 56 * (I + 1)))
      return 5;
    for (unsigned J = sizeof(Next); J != 56; ++J)
      if (Nodes[56 * I + J] != 0x5a)
        return 6;
  }
  free(Nodes);
  puts("memory-boundaries:PASS");
  return 0;
}
