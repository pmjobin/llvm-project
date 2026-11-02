// Native private-call and ordinary public-boundary runtime input.
typedef unsigned int u32;
typedef unsigned long long u64;
#ifndef SH_F32_CASE
#define SH_F32_CASE 0
#endif
static __attribute__((noinline)) float private_leaf(float a, float b, float c) {
	return a * b + c;
}
#if SH_F32_CASE == 1
static __attribute__((noinline)) u32 second_caller(float value) {
	union { float f; u32 u; } result;
	result.f = private_leaf(value, 0.75f, 1.25f);
	return result.u;
}
#endif
__attribute__((noinline)) u64 bench_entry(u32 *data, u32 *aux, unsigned n, u32 seed) {
	float value = (float)(seed & 31);
	for (unsigned i = 0; i < n; ++i) {
#if SH_F32_CASE == 1
		if (i & 1) {
			union { float f; u32 u; } result;
			result.u = second_caller(value);
			value = result.f;
		} else
#endif
			value = private_leaf(value, 0.75f, 1.25f);
	}
	union { float f; u32 u; } result;
	result.f = value;
	return result.u;
}
