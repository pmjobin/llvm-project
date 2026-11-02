#include "runtime-types.h"

static u32 failures;
static struct PackedBool packed = {0x11, 0, 0x2233, 0x44556677, 0x88};
static struct PackedBool copied = {0xaa, 1, 0xbbcc, 0xddeeff00, 0x99};
static u32 left = 13;
static u32 right = 29;
static i32 i32_values[] = {0, 1, -1, (i32)0x80000000u, 0x7fffffff};
static i64 i64_values[] = {0, 1, -1, (i64)0x8000000000000000ULL, 0x7fffffffffffffffLL};

#define CHECK(expression, bit) do { if (!(expression)) failures |= 1u << (bit); } while (0)

static u32 expected_i32(u32 predicate, i32 a, i32 b) {
	switch (predicate) {
	case 0: return a == b;
	case 1: return a != b;
	case 2: return a < b;
	case 3: return a <= b;
	case 4: return a > b;
	case 5: return a >= b;
	case 6: return (u32)a < (u32)b;
	case 7: return (u32)a <= (u32)b;
	case 8: return (u32)a > (u32)b;
	default: return (u32)a >= (u32)b;
	}
}

static u32 expected_i64(u32 predicate, i64 a, i64 b) {
	switch (predicate) {
	case 0: return a == b;
	case 1: return a != b;
	case 2: return a < b;
	case 3: return a <= b;
	case 4: return a > b;
	case 5: return a >= b;
	case 6: return (u64)a < (u64)b;
	case 7: return (u64)a <= (u64)b;
	case 8: return (u64)a > (u64)b;
	default: return (u64)a >= (u64)b;
	}
}

__attribute__((noinline)) void test_aggregate_abi(void) {
	struct R2 r2 = {0x1234};
	struct N4 n4 = {{{1, 2, 3}}, 4};
	struct P3 p3 = {0x4567, 8};
	struct R4 r4;
	struct P8 p8 = {{9, 0x1234}, 0x10203040, 10};
	struct Direct4 dleft = {0x11223344};
	struct Direct4 dright = {0xaabbccdd};
	struct S12 sleft = {{1, 2, 3}};
	struct S12 sright = {{4, 5, 6}};
	struct S12 sresult;
	CHECK(impl_take_r2(r2, 0x55aa) == (0x1234 ^ 0x55aa), 0);
	CHECK(impl_take_n4(n4, 5) == 13, 1);
	CHECK(impl_take_p3(p3, 9) == 0x4578, 2);
	r4 = impl_return_r4(0x1234);
	CHECK(r4.first == 0x34 && r4.second == 0x1235, 3);
	CHECK(impl_boundary(1, 2, 3, p8, 4) == 0x10204288, 4);
	CHECK(impl_conditional_direct(1, dleft, dright).value == dleft.value, 5);
	CHECK(impl_conditional_direct(0, dleft, dright).value == dright.value, 6);
	sresult = impl_conditional_sret(1, sleft, sright);
	CHECK(sresult.words[0] == 1 && sresult.words[2] == 3, 7);
	sresult = impl_conditional_sret(0, sleft, sright);
	CHECK(sresult.words[0] == 4 && sresult.words[2] == 6, 8);
}

__attribute__((noinline)) void test_packed_bool(void) {
	CHECK(impl_bool_min(&packed) == 0x44558943, 9);
	impl_bool_store(&packed, 1, 2);
	CHECK(((u8 *)&packed)[1] == 1 && packed.prefix == 0x11 && packed.word == 0x2233 && packed.dword == 0x44556677 && packed.suffix == 0x88, 10);
	impl_bool_store(&packed, 2, 2);
	CHECK(((u8 *)&packed)[1] == 0 && packed.prefix == 0x11 && packed.word == 0x2233 && packed.dword == 0x44556677 && packed.suffix == 0x88, 11);
	CHECK(impl_bool_mixed(&packed, 3, 2) == 0x44556677 && ((u8 *)&packed)[1] == 1, 12);
	CHECK(impl_bool_mixed(&packed, 2, 3) == 0x2233 && ((u8 *)&packed)[1] == 0, 13);
}

__attribute__((noinline)) void test_predicates(void) {
	for (u32 predicate = 0; predicate != 10; ++predicate) {
		for (u32 a = 0; a != sizeof(i32_values) / sizeof(i32_values[0]); ++a)
			for (u32 b = 0; b != sizeof(i32_values) / sizeof(i32_values[0]); ++b)
				CHECK(impl_cmp_i32(predicate, i32_values[a], i32_values[b]) == expected_i32(predicate, i32_values[a], i32_values[b]), 14);
		for (u32 a = 0; a != sizeof(i64_values) / sizeof(i64_values[0]); ++a)
			for (u32 b = 0; b != sizeof(i64_values) / sizeof(i64_values[0]); ++b)
				CHECK(impl_cmp_i64(predicate, i64_values[a], i64_values[b]) == expected_i64(predicate, i64_values[a], i64_values[b]), 15);
	}
}

__attribute__((noinline)) void test_values_and_memory(void) {
	CHECK(impl_zext(1, 2) == 1 && impl_zext(2, 1) == 0, 16);
	CHECK(impl_sext(1, 2) == -1 && impl_sext(2, 1) == 0, 17);
	CHECK(impl_scalar_select(1, left, right) == left && impl_scalar_select(0, left, right) == right, 18);
	CHECK(impl_pointer_select(1, &left, &right) == left && impl_pointer_select(0, &left, &right) == right, 19);
	CHECK(impl_i64_select(1, 0x1122334455667788ULL, 0x8877665544332211ULL) == 0x1122334455667788ULL, 20);
	CHECK(impl_i64_select(0, 0x1122334455667788ULL, 0x8877665544332211ULL) == 0x8877665544332211ULL, 21);
	CHECK(impl_live_call(-4, 7, 9) == foreign_helper(9) + 1 && impl_live_call(7, -4, 9) == foreign_helper(9), 22);
	CHECK(impl_unaligned_compare(&packed, 0x2233, 0x50000000) == 2, 23);
	impl_conditional_memory(&packed, &copied, 1);
	CHECK(packed.prefix == copied.prefix && packed.flag == copied.flag && packed.word == copied.word && packed.dword == copied.dword && packed.suffix == copied.suffix, 24);
	impl_conditional_memory(&packed, &copied, 0);
	CHECK(packed.prefix == 0 && packed.flag == 0 && packed.word == 0 && packed.dword == 0 && packed.suffix == 0, 25);
}

int main(void) {
	test_aggregate_abi();
	test_packed_bool();
	test_predicates();
	test_values_and_memory();
	return failures != 0;
}
