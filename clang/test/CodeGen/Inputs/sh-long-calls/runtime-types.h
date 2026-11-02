#ifndef SH_LONG_CALLS_RUNTIME_TYPES_H
#define SH_LONG_CALLS_RUNTIME_TYPES_H

typedef signed char i8;
typedef unsigned char u8;
typedef signed short i16;
typedef unsigned short u16;
typedef signed int i32;
typedef unsigned int u32;
typedef signed long long i64;
typedef unsigned long long u64;

struct R2 {
	u16 value;
};

struct N4 {
	struct {
		u8 bytes[3];
	} inner;
	u8 tail;
};

struct __attribute__((packed)) P3 {
	u16 value;
	u8 tail;
};

struct R4 {
	u8 first;
	u16 second;
};

struct E8 {
	u8 bytes[8];
};

struct __attribute__((packed)) I3 {
	u8 tag;
	u16 value;
};

struct __attribute__((packed)) P8 {
	struct I3 inner;
	u32 word;
	u8 tail;
};

struct __attribute__((packed)) PackedBool {
	u8 prefix;
	_Bool flag;
	u16 word;
	u32 dword;
	u8 suffix;
};

struct __attribute__((packed, aligned(4))) Direct4 {
	u32 value;
};

struct S12 {
	u32 words[3];
};

u32 foreign_helper(u32 value);
u32 impl_take_r2(struct R2 value, u32 salt);
u32 impl_take_n4(struct N4 value, u32 salt);
u32 impl_take_p3(struct P3 value, u32 salt);
struct R4 impl_return_r4(u32 value);
u32 impl_boundary(u32 a, u32 b, u32 c, struct P8 value, u32 tail);
struct Direct4 impl_conditional_direct(i32 choose, struct Direct4 left, struct Direct4 right);
struct S12 impl_conditional_sret(i32 choose, struct S12 left, struct S12 right);
u32 impl_bool_min(struct PackedBool *value);
void impl_bool_store(struct PackedBool *value, i32 a, i32 b);
u32 impl_bool_mixed(struct PackedBool *value, i32 a, i32 b);
u32 impl_cmp_i32(u32 predicate, i32 a, i32 b);
u32 impl_cmp_i64(u32 predicate, i64 a, i64 b);
u32 impl_zext(i32 a, i32 b);
i32 impl_sext(i32 a, i32 b);
u32 impl_scalar_select(i32 choose, u32 left, u32 right);
u32 impl_pointer_select(i32 choose, u32 *left, u32 *right);
u64 impl_i64_select(i32 choose, u64 left, u64 right);
u32 impl_live_call(i32 a, i32 b, u32 value);
u32 impl_unaligned_compare(const struct PackedBool *value, u16 word, u32 dword);
void impl_conditional_memory(struct PackedBool *destination, const struct PackedBool *source, i32 choose);

extern u32 runtime_failures;
void test_aggregate_abi(void);
void test_packed_bool(void);
void test_predicates(void);
void test_values_and_memory(void);

#endif
