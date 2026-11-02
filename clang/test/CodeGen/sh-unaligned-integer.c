// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O2 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O2 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O2 -emit-obj -o /dev/null %s
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O2 -emit-obj -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O0 -c -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O0 -c -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O2 -c -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O2 -c -o /dev/null %s

typedef unsigned int u32_align1 __attribute__((aligned(1)));
typedef unsigned int u32_align2 __attribute__((aligned(2)));
typedef unsigned long long u64_align2 __attribute__((aligned(2)));

struct __attribute__((packed)) P16 {
	unsigned char prefix;
	unsigned short value;
};

struct __attribute__((packed)) P32 {
	unsigned char prefix;
	unsigned int value;
};

struct __attribute__((packed)) P64 {
	unsigned char prefix;
	unsigned long long value;
};

struct __attribute__((packed)) Signed16 {
	unsigned char prefix;
	short value;
};

struct __attribute__((packed)) Signed32 {
	unsigned char prefix;
	int value;
};

struct __attribute__((packed)) Nested {
	unsigned char prefix;
	struct P32 inner;
};

struct __attribute__((packed)) PackedArray {
	unsigned char prefix;
	unsigned short value[2];
};

struct __attribute__((packed)) VolatileFields {
	unsigned char prefix;
	volatile unsigned short half;
	volatile unsigned int word;
	volatile unsigned long long wide;
};

struct Natural {
	unsigned short half;
	unsigned int word;
	unsigned long long wide;
};

// CHECK-LABEL: define{{.*}} i32 @load_p16(
// CHECK: load i16, ptr {{.*}}, align 1
unsigned load_p16(const struct P16 *p) {
	return p->value;
}

// CHECK-LABEL: define{{.*}} void @store_p16(
// CHECK: store i16 {{.*}}, ptr {{.*}}, align 1
void store_p16(struct P16 *p, unsigned value) {
	p->value = value;
}

// CHECK-LABEL: define{{.*}} i32 @load_p32(
// CHECK: load i32, ptr {{.*}}, align 1
unsigned load_p32(const struct P32 *p) {
	return p->value;
}

// CHECK-LABEL: define{{.*}} void @store_p32(
// CHECK: store i32 {{.*}}, ptr {{.*}}, align 1
void store_p32(struct P32 *p, unsigned value) {
	p->value = value;
}

// CHECK-LABEL: define{{.*}} i64 @load_p64(
// CHECK: load i64, ptr {{.*}}, align 1
unsigned long long load_p64(const struct P64 *p) {
	return p->value;
}

// CHECK-LABEL: define{{.*}} void @store_p64(
// CHECK: store i64 {{.*}}, ptr {{.*}}, align 1
void store_p64(struct P64 *p, unsigned long long value) {
	p->value = value;
}

// CHECK-LABEL: define{{.*}} i32 @load_signed16(
// CHECK: load i16, ptr {{.*}}, align 1
// CHECK: sext i16 {{.*}} to i32
int load_signed16(const struct Signed16 *p) {
	return p->value;
}

// CHECK-LABEL: define{{.*}} i32 @load_signed32(
// CHECK: load i32, ptr {{.*}}, align 1
int load_signed32(const struct Signed32 *p) {
	return p->value;
}

// CHECK-LABEL: define{{.*}} void @store_signed32(
// CHECK: store i32 {{.*}}, ptr {{.*}}, align 1
void store_signed32(struct Signed32 *p, int value) {
	p->value = value;
}

// CHECK-LABEL: define{{.*}} i32 @load_nested(
// CHECK: load i32, ptr {{.*}}, align 1
unsigned load_nested(const struct Nested *p) {
	return p->inner.value;
}

// CHECK-LABEL: define{{.*}} void @store_nested(
// CHECK: store i32 {{.*}}, ptr {{.*}}, align 1
void store_nested(struct Nested *p, unsigned value) {
	p->inner.value = value;
}

// CHECK-LABEL: define{{.*}} i32 @load_array(
// CHECK: load i16, ptr {{.*}}, align 1
unsigned load_array(const struct PackedArray *p) {
	return p->value[1];
}

// CHECK-LABEL: define{{.*}} void @store_array(
// CHECK: store i16 {{.*}}, ptr {{.*}}, align 1
void store_array(struct PackedArray *p, unsigned value) {
	p->value[1] = value;
}

// CHECK-LABEL: define{{.*}} i32 @load_align1_pointer(
// CHECK: load i32, ptr {{.*}}, align 1
unsigned load_align1_pointer(const u32_align1 *p) {
	return *p;
}

// CHECK-LABEL: define{{.*}} void @store_align1_pointer(
// CHECK: store i32 {{.*}}, ptr {{.*}}, align 1
void store_align1_pointer(u32_align1 *p, unsigned value) {
	*p = value;
}

// CHECK-LABEL: define{{.*}} i32 @load_align2_pointer(
// CHECK: load i32, ptr {{.*}}, align 2
unsigned load_align2_pointer(const u32_align2 *p) {
	return *p;
}

// CHECK-LABEL: define{{.*}} void @store_align2_pointer(
// CHECK: store i32 {{.*}}, ptr {{.*}}, align 2
void store_align2_pointer(u32_align2 *p, unsigned value) {
	*p = value;
}

// CHECK-LABEL: define{{.*}} i64 @load_i64_align2_pointer(
// CHECK: load i64, ptr {{.*}}, align 2
unsigned long long load_i64_align2_pointer(const u64_align2 *p) {
	return *p;
}

// CHECK-LABEL: define{{.*}} void @store_i64_align2_pointer(
// CHECK: store i64 {{.*}}, ptr {{.*}}, align 2
void store_i64_align2_pointer(u64_align2 *p, unsigned long long value) {
	*p = value;
}

// CHECK-LABEL: define{{.*}} i32 @increment_p32(
// CHECK: load i32, ptr {{.*}}, align 1
// CHECK: store i32 {{.*}}, ptr {{.*}}, align 1
unsigned increment_p32(struct P32 *p) {
	return ++p->value;
}

// CHECK-LABEL: define{{.*}} i32 @compound_signed16(
// CHECK: load i16, ptr {{.*}}, align 1
// CHECK: store i16 {{.*}}, ptr {{.*}}, align 1
int compound_signed16(struct Signed16 *p, int value) {
	return p->value += value;
}

// CHECK-LABEL: define{{.*}} i32 @load_volatile_half(
// CHECK: load volatile i16, ptr {{.*}}, align 1
unsigned load_volatile_half(const struct VolatileFields *p) {
	return p->half;
}

// CHECK-LABEL: define{{.*}} void @store_volatile_half(
// CHECK: store volatile i16 {{.*}}, ptr {{.*}}, align 1
void store_volatile_half(struct VolatileFields *p, unsigned value) {
	p->half = value;
}

// CHECK-LABEL: define{{.*}} i32 @load_volatile_word(
// CHECK: load volatile i32, ptr {{.*}}, align 1
unsigned load_volatile_word(const struct VolatileFields *p) {
	return p->word;
}

// CHECK-LABEL: define{{.*}} void @store_volatile_word(
// CHECK: store volatile i32 {{.*}}, ptr {{.*}}, align 1
void store_volatile_word(struct VolatileFields *p, unsigned value) {
	p->word = value;
}

// CHECK-LABEL: define{{.*}} i64 @load_volatile_wide(
// CHECK: load volatile i64, ptr {{.*}}, align 1
unsigned long long load_volatile_wide(const struct VolatileFields *p) {
	return p->wide;
}

// CHECK-LABEL: define{{.*}} void @store_volatile_wide(
// CHECK: store volatile i64 {{.*}}, ptr {{.*}}, align 1
void store_volatile_wide(struct VolatileFields *p, unsigned long long value) {
	p->wide = value;
}

// CHECK-LABEL: define{{.*}} i32 @load_natural_half(
// CHECK: load i16, ptr {{.*}}, align 4
unsigned load_natural_half(const struct Natural *p) {
	return p->half;
}

// CHECK-LABEL: define{{.*}} i32 @load_natural_word(
// CHECK: load i32, ptr {{.*}}, align 4
unsigned load_natural_word(const struct Natural *p) {
	return p->word;
}

// CHECK-LABEL: define{{.*}} i64 @load_natural_wide(
// CHECK: load i64, ptr {{.*}}, align 4
unsigned long long load_natural_wide(const struct Natural *p) {
	return p->wide;
}
