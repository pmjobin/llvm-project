#include "bench.h"
#ifndef SH_MASK_CASE
#define SH_MASK_CASE 1
#endif
BENCH_FN u64 bench_entry(u32 *data, u32 *aux, unsigned n, u32 seed) {
	u32 sum = seed;
	for (unsigned i = 0; i != n; ++i) {
		u32 x = data[i];
#if SH_MASK_CASE == 1
		u32 v = ((x >> 8) & 31) << 2;
#elif SH_MASK_CASE == 2
		u32 v = ((x >> 16) & 63) << 2;
#elif SH_MASK_CASE == 3
		u32 v = *(const u16 *)(data + i) & 31;
#elif SH_MASK_CASE == 4
		u32 v = *(const u8 *)(data + i) & 7;
#elif SH_MASK_CASE == 5
		u32 v = ((x >> 8) & 31) << 2;
		sum ^= x >> 6; // a second genuinely unmasked producer use
#elif SH_MASK_CASE == 6
		u32 v = (*(const u16 *)(data + i) & 31) + ((x >> 8) & 63);
		sum += aux[i] ^ (seed + i);
#elif SH_MASK_CASE == 7
		u32 v = *(const volatile u8 *)(data + i) & 7;
#else
		u32 v = ((x >> 8) & 31) << 2;
		v += ((aux[i] >> 16) & 63) << 2;
#endif
		sum = (sum + v) ^ (aux[i] + seed);
	}
	return (u64)sum;
}
