typedef unsigned int u32;
typedef unsigned long long u64;
extern float ordinary_boundary(float);
extern u32 observed_boundary;
static __attribute__((noinline)) float private_leaf(float a, float b) {
	return a + b;
}
__attribute__((noinline)) u64 bench_entry(u32 *data, u32 *aux, unsigned n, u32 seed) {
	float value = (float)(seed & 31);
	for (unsigned i = 0; i < n; ++i)
		value = private_leaf(value, 1.0f);
	value = ordinary_boundary(value);
	value = private_leaf(value, 0.5f);
	union { float f; u32 u; } result;
	result.f = value;
	return ((u64)observed_boundary << 32) | result.u;
}
