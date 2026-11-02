// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <stddef.h>
#include <string.h>

// Each call has a constant length or the explicit dynamic fallback.
// Alignment is a valid caller contract, independent for source and destination.
void copy1(void *D, const void *S, unsigned N) {
  switch (N) {
  case 0: __builtin_memcpy(D, S, 0); return;
  case 1: __builtin_memcpy(D, S, 1); return;
  case 4: __builtin_memcpy(D, S, 4); return;
  case 5: __builtin_memcpy(D, S, 5); return;
  case 8: __builtin_memcpy(D, S, 8); return;
  case 9: __builtin_memcpy(D, S, 9); return;
  case 16: __builtin_memcpy(D, S, 16); return;
  case 17: __builtin_memcpy(D, S, 17); return;
  case 31: __builtin_memcpy(D, S, 31); return;
  case 32: __builtin_memcpy(D, S, 32); return;
  case 33: __builtin_memcpy(D, S, 33); return;
  case 56: __builtin_memcpy(D, S, 56); return;
  case 60: __builtin_memcpy(D, S, 60); return;
  case 61: __builtin_memcpy(D, S, 61); return;
  case 128: __builtin_memcpy(D, S, 128); return;
  default: __builtin_memcpy(D, S, N); return;
  }
}

void copy2(void *D, const void *S, unsigned N) {
  D = __builtin_assume_aligned(D, 2);
  S = __builtin_assume_aligned(S, 2);
  switch (N) {
  case 0: __builtin_memcpy(D, S, 0); return;
  case 1: __builtin_memcpy(D, S, 1); return;
  case 4: __builtin_memcpy(D, S, 4); return;
  case 5: __builtin_memcpy(D, S, 5); return;
  case 8: __builtin_memcpy(D, S, 8); return;
  case 9: __builtin_memcpy(D, S, 9); return;
  case 16: __builtin_memcpy(D, S, 16); return;
  case 17: __builtin_memcpy(D, S, 17); return;
  case 31: __builtin_memcpy(D, S, 31); return;
  case 32: __builtin_memcpy(D, S, 32); return;
  case 33: __builtin_memcpy(D, S, 33); return;
  case 56: __builtin_memcpy(D, S, 56); return;
  case 60: __builtin_memcpy(D, S, 60); return;
  case 61: __builtin_memcpy(D, S, 61); return;
  case 128: __builtin_memcpy(D, S, 128); return;
  default: __builtin_memcpy(D, S, N); return;
  }
}

void copy4(void *D, const void *S, unsigned N) {
  D = __builtin_assume_aligned(D, 4);
  S = __builtin_assume_aligned(S, 4);
  switch (N) {
  case 0: __builtin_memcpy(D, S, 0); return;
  case 1: __builtin_memcpy(D, S, 1); return;
  case 4: __builtin_memcpy(D, S, 4); return;
  case 5: __builtin_memcpy(D, S, 5); return;
  case 8: __builtin_memcpy(D, S, 8); return;
  case 9: __builtin_memcpy(D, S, 9); return;
  case 16: __builtin_memcpy(D, S, 16); return;
  case 17: __builtin_memcpy(D, S, 17); return;
  case 31: __builtin_memcpy(D, S, 31); return;
  case 32: __builtin_memcpy(D, S, 32); return;
  case 33: __builtin_memcpy(D, S, 33); return;
  case 56: __builtin_memcpy(D, S, 56); return;
  case 60: __builtin_memcpy(D, S, 60); return;
  case 61: __builtin_memcpy(D, S, 61); return;
  case 128: __builtin_memcpy(D, S, 128); return;
  default: __builtin_memcpy(D, S, N); return;
  }
}

void move1(void *D, const void *S, unsigned N) {
  switch (N) {
  case 0: __builtin_memmove(D, S, 0); return;
  case 1: __builtin_memmove(D, S, 1); return;
  case 4: __builtin_memmove(D, S, 4); return;
  case 5: __builtin_memmove(D, S, 5); return;
  case 8: __builtin_memmove(D, S, 8); return;
  case 9: __builtin_memmove(D, S, 9); return;
  case 16: __builtin_memmove(D, S, 16); return;
  case 17: __builtin_memmove(D, S, 17); return;
  case 31: __builtin_memmove(D, S, 31); return;
  case 32: __builtin_memmove(D, S, 32); return;
  case 33: __builtin_memmove(D, S, 33); return;
  case 56: __builtin_memmove(D, S, 56); return;
  case 60: __builtin_memmove(D, S, 60); return;
  case 61: __builtin_memmove(D, S, 61); return;
  case 128: __builtin_memmove(D, S, 128); return;
  default: __builtin_memmove(D, S, N); return;
  }
}

void move2(void *D, const void *S, unsigned N) {
  D = __builtin_assume_aligned(D, 2);
  S = __builtin_assume_aligned(S, 2);
  switch (N) {
  case 0: __builtin_memmove(D, S, 0); return;
  case 1: __builtin_memmove(D, S, 1); return;
  case 4: __builtin_memmove(D, S, 4); return;
  case 5: __builtin_memmove(D, S, 5); return;
  case 8: __builtin_memmove(D, S, 8); return;
  case 9: __builtin_memmove(D, S, 9); return;
  case 16: __builtin_memmove(D, S, 16); return;
  case 17: __builtin_memmove(D, S, 17); return;
  case 31: __builtin_memmove(D, S, 31); return;
  case 32: __builtin_memmove(D, S, 32); return;
  case 33: __builtin_memmove(D, S, 33); return;
  case 56: __builtin_memmove(D, S, 56); return;
  case 60: __builtin_memmove(D, S, 60); return;
  case 61: __builtin_memmove(D, S, 61); return;
  case 128: __builtin_memmove(D, S, 128); return;
  default: __builtin_memmove(D, S, N); return;
  }
}

void move4(void *D, const void *S, unsigned N) {
  D = __builtin_assume_aligned(D, 4);
  S = __builtin_assume_aligned(S, 4);
  switch (N) {
  case 0: __builtin_memmove(D, S, 0); return;
  case 1: __builtin_memmove(D, S, 1); return;
  case 4: __builtin_memmove(D, S, 4); return;
  case 5: __builtin_memmove(D, S, 5); return;
  case 8: __builtin_memmove(D, S, 8); return;
  case 9: __builtin_memmove(D, S, 9); return;
  case 16: __builtin_memmove(D, S, 16); return;
  case 17: __builtin_memmove(D, S, 17); return;
  case 31: __builtin_memmove(D, S, 31); return;
  case 32: __builtin_memmove(D, S, 32); return;
  case 33: __builtin_memmove(D, S, 33); return;
  case 56: __builtin_memmove(D, S, 56); return;
  case 60: __builtin_memmove(D, S, 60); return;
  case 61: __builtin_memmove(D, S, 61); return;
  case 128: __builtin_memmove(D, S, 128); return;
  default: __builtin_memmove(D, S, N); return;
  }
}

void set1(void *D, const void *S, unsigned N) {
  switch (N) {
  case 0: __builtin_memset(D, 0xa5, 0); return;
  case 1: __builtin_memset(D, 0xa5, 1); return;
  case 4: __builtin_memset(D, 0xa5, 4); return;
  case 5: __builtin_memset(D, 0xa5, 5); return;
  case 8: __builtin_memset(D, 0xa5, 8); return;
  case 9: __builtin_memset(D, 0xa5, 9); return;
  case 16: __builtin_memset(D, 0xa5, 16); return;
  case 17: __builtin_memset(D, 0xa5, 17); return;
  case 31: __builtin_memset(D, 0xa5, 31); return;
  case 32: __builtin_memset(D, 0xa5, 32); return;
  case 33: __builtin_memset(D, 0xa5, 33); return;
  case 56: __builtin_memset(D, 0xa5, 56); return;
  case 60: __builtin_memset(D, 0xa5, 60); return;
  case 61: __builtin_memset(D, 0xa5, 61); return;
  case 128: __builtin_memset(D, 0xa5, 128); return;
  default: __builtin_memset(D, 0xa5, N); return;
  }
}

void set2(void *D, const void *S, unsigned N) {
  D = __builtin_assume_aligned(D, 2);
  S = __builtin_assume_aligned(S, 2);
  switch (N) {
  case 0: __builtin_memset(D, 0xa5, 0); return;
  case 1: __builtin_memset(D, 0xa5, 1); return;
  case 4: __builtin_memset(D, 0xa5, 4); return;
  case 5: __builtin_memset(D, 0xa5, 5); return;
  case 8: __builtin_memset(D, 0xa5, 8); return;
  case 9: __builtin_memset(D, 0xa5, 9); return;
  case 16: __builtin_memset(D, 0xa5, 16); return;
  case 17: __builtin_memset(D, 0xa5, 17); return;
  case 31: __builtin_memset(D, 0xa5, 31); return;
  case 32: __builtin_memset(D, 0xa5, 32); return;
  case 33: __builtin_memset(D, 0xa5, 33); return;
  case 56: __builtin_memset(D, 0xa5, 56); return;
  case 60: __builtin_memset(D, 0xa5, 60); return;
  case 61: __builtin_memset(D, 0xa5, 61); return;
  case 128: __builtin_memset(D, 0xa5, 128); return;
  default: __builtin_memset(D, 0xa5, N); return;
  }
}

void set4(void *D, const void *S, unsigned N) {
  D = __builtin_assume_aligned(D, 4);
  S = __builtin_assume_aligned(S, 4);
  switch (N) {
  case 0: __builtin_memset(D, 0xa5, 0); return;
  case 1: __builtin_memset(D, 0xa5, 1); return;
  case 4: __builtin_memset(D, 0xa5, 4); return;
  case 5: __builtin_memset(D, 0xa5, 5); return;
  case 8: __builtin_memset(D, 0xa5, 8); return;
  case 9: __builtin_memset(D, 0xa5, 9); return;
  case 16: __builtin_memset(D, 0xa5, 16); return;
  case 17: __builtin_memset(D, 0xa5, 17); return;
  case 31: __builtin_memset(D, 0xa5, 31); return;
  case 32: __builtin_memset(D, 0xa5, 32); return;
  case 33: __builtin_memset(D, 0xa5, 33); return;
  case 56: __builtin_memset(D, 0xa5, 56); return;
  case 60: __builtin_memset(D, 0xa5, 60); return;
  case 61: __builtin_memset(D, 0xa5, 61); return;
  case 128: __builtin_memset(D, 0xa5, 128); return;
  default: __builtin_memset(D, 0xa5, N); return;
  }
}

void copy_mixed(void *D, const void *S, unsigned N) {
  D = __builtin_assume_aligned(D, 4);
  __builtin_memcpy(D, S, 56);
}

// Reduced address-chain initializer from the pool-pressure investigation.
// The caller checks all links and untouched bytes after whole-TU compilation.
void link_nodes(unsigned char *Storage) {
  Storage = __builtin_assume_aligned(Storage, 4);
  for (unsigned I = 0; I != 72; ++I)
    *(void **)(Storage + 56 * I) = Storage + 56 * (I + 1);
  *(void **)(Storage + 56 * 72) = 0;
}
