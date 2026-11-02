// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O0 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O1 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -Os -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O0 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O1 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -Os -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O0 -fPIC -ftls-model=global-dynamic -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O2 -fPIC -ftls-model=global-dynamic -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O0 -fPIC -ftls-model=global-dynamic -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O2 -fPIC -ftls-model=global-dynamic -c -mllvm -verify-machineinstrs -o /dev/null %s

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

struct __attribute__((packed)) Reduced {
	u32 value;
};

struct __attribute__((packed)) BoolField {
	u8 before;
	_Bool enabled;
	u16 value;
};

struct __attribute__((packed)) BoolFields {
	u8 prefix;
	_Bool flag;
	u16 value;
	u8 suffix;
};

struct __attribute__((packed)) BoolWithWord {
	u8 prefix;
	_Bool flag;
	u32 value;
	u8 suffix;
};

struct __attribute__((packed)) NestedBoolFields {
	u8 guard;
	struct BoolFields fields;
	u8 tail;
};

struct __attribute__((packed)) LargeReduced {
	u32 first;
	u32 second;
	u8 tail;
};

struct __attribute__((packed)) ComparisonPressure {
	u8 padding[80];
	_Bool flag;
	u32 words[24];
};

extern u32 helper(u32);
extern __thread u32 tls_value;

struct Reduced conditional_min(int choose, struct Reduced left, struct Reduced right) {
	return choose ? left : right;
}

u32 bool_field_min(struct BoolField *p) {
	return p->before + p->enabled + p->value;
}

void bool_fields_eq(struct BoolFields *p, u32 a, u32 b) {
	p->flag = a == b;
}

void bool_fields_ne(struct BoolFields *p, u32 a, u32 b) {
	p->flag = a != b;
}

void bool_fields_signed(struct BoolFields *p, int a, int b) {
	p->flag = a < b;
}

void bool_fields_unsigned(struct BoolFields *p, u32 a, u32 b) {
	p->flag = a >= b;
}

void bool_fields_zero(struct BoolFields *p, u32 value) {
	p->flag = value != 0;
}

u32 bool_fields_reload(struct BoolFields *p) {
	_Bool flag = p->flag;
	return p->prefix + flag + p->value + p->suffix;
}

u32 bool_fields_arithmetic(struct BoolFields *p, u32 addend) {
	return addend + p->flag;
}

u32 bool_fields_branch(struct BoolFields *p, u32 when_true, u32 when_false) {
	return p->flag ? when_true : when_false;
}

void bool_fields_conditional(struct BoolFields *p, u32 selector, u32 a, u32 b) {
	p->flag = selector ? a < b : a != b;
}

void bool_word_store(struct BoolWithWord *p, u32 a, u32 b) {
	p->flag = a <= b;
}

u32 bool_word_reload(struct BoolWithWord *p) {
	return p->prefix + p->flag + p->value + p->suffix;
}

void bool_array_store(struct BoolFields *records, u32 index, u32 a, u32 b) {
	records[index].flag = a > b;
}

u32 bool_array_reload(struct BoolFields *records, u32 index) {
	return records[index].flag + records[index].value;
}

void nested_bool_store(struct NestedBoolFields *p, u32 a, u32 b) {
	p->fields.flag = a < b;
}

u32 nested_bool_reload(struct NestedBoolFields *p) {
	return p->guard + p->fields.flag + p->fields.value + p->tail;
}

struct Reduced select_reduced(int choose, struct Reduced left, struct Reduced right) {
	return choose ? left : right;
}

struct LargeReduced select_large_reduced(int choose, struct LargeReduced left, struct LargeReduced right) {
	return choose ? left : right;
}

struct BoolFields *select_record_address(int choose, struct BoolFields *left, struct BoolFields *right) {
	return choose ? left : right;
}

u32 comparison_argument(u32 a, u32 b) {
	return helper(a == b);
}

u32 comparison_live_across_call(u32 a, u32 b, u32 value) {
	_Bool condition = a < b;
	return helper(value) + condition;
}

u32 comparison_tls(u32 a, u32 b) {
	_Bool condition = a != b;
	return helper(tls_value) + condition;
}

u32 comparison_frame_pressure(u32 a, u32 b, u32 value) {
	volatile struct ComparisonPressure local;
	local.flag = a >= b;
	local.words[23] = value;
	return helper(local.words[23]) + local.flag;
}
