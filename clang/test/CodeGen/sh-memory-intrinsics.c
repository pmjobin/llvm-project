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

struct R {
	unsigned char a;
	unsigned short b;
};

struct EquivalentR {
	unsigned short a;
	unsigned char b;
};

struct Large {
	unsigned words[5];
};

struct PaddedLarge {
	unsigned char prefix;
	unsigned value;
	unsigned char tail[12];
};

struct __attribute__((packed)) PackedLarge {
	unsigned char prefix;
	unsigned words[4];
};

struct VolatileBlock {
	unsigned char bytes[16];
};

// CHECK-LABEL: define{{.*}} void @return_reduced4(
// CHECK-SAME: sret(%struct.R) align 2
// CHECK: call void @llvm.memset.p0.i64(ptr align 1 {{.*}}, i8 0, i64 1, i1 false)
struct R return_reduced4(unsigned value) {
	return (struct R){value, value + 1};
}

// CHECK-LABEL: define{{.*}} void @return_equivalent4(
// CHECK-SAME: sret(%struct.EquivalentR) align 2
struct EquivalentR return_equivalent4(unsigned value) {
	return (struct EquivalentR){value, value + 1};
}

// CHECK-LABEL: define{{.*}} void @assign_large(
// CHECK: call void @llvm.memcpy.p0.p0.i32(ptr align 4 {{.*}}, ptr align 4 {{.*}}, i32 20, i1 false)
void assign_large(struct Large *dst, const struct Large *src) {
	*dst = *src;
}

// CHECK-LABEL: define{{.*}} void @assign_padded(
// CHECK: call void @llvm.memcpy.p0.p0.i32(ptr align 4 {{.*}}, ptr align 4 {{.*}}, i32 20, i1 false)
void assign_padded(struct PaddedLarge *dst, const struct PaddedLarge *src) {
	*dst = *src;
}

// CHECK-LABEL: define{{.*}} void @assign_packed(
// CHECK: call void @llvm.memcpy.p0.p0.i32(ptr align 1 {{.*}}, ptr align 1 {{.*}}, i32 17, i1 false)
void assign_packed(struct PackedLarge *dst, const struct PackedLarge *src) {
	*dst = *src;
}

// CHECK-LABEL: define{{.*}} void @builtin_memset_constant(
// CHECK: call void @llvm.memset.p0.i32(ptr align 1 {{.*}}, i8 {{.*}}, i32 32, i1 false)
void builtin_memset_constant(void *dst, int value) {
	__builtin_memset(dst, value, 32);
}

// CHECK-LABEL: define{{.*}} void @builtin_memset_variable(
// CHECK: call void @llvm.memset.p0.i32(ptr align 1 {{.*}}, i8 {{.*}}, i32 {{.*}}, i1 false)
void builtin_memset_variable(void *dst, int value, __SIZE_TYPE__ size) {
	__builtin_memset(dst, value, size);
}

// CHECK-LABEL: define{{.*}} void @builtin_memcpy_constant(
// CHECK: call void @llvm.memcpy.p0.p0.i32(ptr align 1 {{.*}}, ptr align 1 {{.*}}, i32 32, i1 false)
void builtin_memcpy_constant(void *dst, const void *src) {
	__builtin_memcpy(dst, src, 32);
}

// CHECK-LABEL: define{{.*}} void @builtin_memcpy_variable(
// CHECK: call void @llvm.memcpy.p0.p0.i32(ptr align 1 {{.*}}, ptr align 1 {{.*}}, i32 {{.*}}, i1 false)
void builtin_memcpy_variable(void *dst, const void *src, __SIZE_TYPE__ size) {
	__builtin_memcpy(dst, src, size);
}

// CHECK-LABEL: define{{.*}} void @builtin_memmove_overlap(
// CHECK: call void @llvm.memmove.p0.p0.i32(ptr align 1 {{.*}}, ptr align 1 {{.*}}, i32 {{.*}}, i1 false)
void builtin_memmove_overlap(unsigned char *bytes, __SIZE_TYPE__ size) {
	__builtin_memmove(bytes + 1, bytes, size);
}

// CHECK-LABEL: define{{.*}} void @builtin_zero_lengths(
// CHECK: call void @llvm.memset.p0.i32(ptr align 1 {{.*}}, i8 0, i32 0, i1 false)
// CHECK: call void @llvm.memcpy.p0.p0.i32(ptr align 1 {{.*}}, ptr align 1 {{.*}}, i32 0, i1 false)
// CHECK: call void @llvm.memmove.p0.p0.i32(ptr align 1 {{.*}}, ptr align 1 {{.*}}, i32 0, i1 false)
void builtin_zero_lengths(void *dst, const void *src) {
	__builtin_memset(dst, 0, 0);
	__builtin_memcpy(dst, src, 0);
	__builtin_memmove(dst, src, 0);
}

// CHECK-LABEL: define{{.*}} void @copy_volatile_block(
// CHECK: call void @llvm.memcpy.p0.p0.i32(ptr align 1 {{.*}}, ptr align 1 {{.*}}, i32 16, i1 true)
void copy_volatile_block(volatile struct VolatileBlock *dst, const volatile struct VolatileBlock *src) {
	*dst = *src;
}
