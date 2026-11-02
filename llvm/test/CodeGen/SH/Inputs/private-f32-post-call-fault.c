typedef unsigned int u32;
typedef unsigned long long u64;
static __attribute__((noinline)) float private_leaf(float a, float b, float c) {
	return a * b + c;
}
__attribute__((noinline)) u64 bench_entry(u32 *data, u32 *aux, unsigned n, u32 seed) {
	union { float f; u32 u; } result;
	result.f = private_leaf((float)(seed & 31), 0.1f, 1.25f);
	// The fault address depends on the actual returned bits: the call must finish.
	const volatile u32 *address = (const volatile u32 *)(result.u ^ 0x40566667U);
	return *address;
}
