// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O0 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O1 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -Os -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O0 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O1 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -Os -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O0 -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O1 -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O2 -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -Os -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O0 -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O1 -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O2 -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -Os -S -emit-llvm -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O1 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O2 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -Os -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O0 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O1 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O2 -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -Os -S -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O0 -fPIC -ftls-model=global-dynamic -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O2 -fPIC -ftls-model=global-dynamic -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O0 -fPIC -ftls-model=global-dynamic -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O2 -fPIC -ftls-model=global-dynamic -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O1 -emit-llvm -o - %s | FileCheck %s --check-prefix=IR

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

struct __attribute__((packed)) PackedOverflow {
	u8 prefix;
	u32 value;
	_Bool flag;
	u8 suffix;
};

struct OverflowPressure {
	u8 padding[80];
	_Bool flag;
	u32 words[24];
};

extern u32 helper(u32);
extern __thread u32 tls_value;

__attribute__((noinline)) u32 comparison_around_rotate(u32 a, u32 b) {
	_Bool before = a < b;
	u32 rotated = (a << 3) | (a >> 29);
	_Bool after = rotated >= b;
	return (before << 1) | after;
}

__attribute__((noinline)) u32 comparison_around_carry(u32 a, u32 b) {
	u32 sum;
	_Bool before = a < b;
	_Bool carry = __builtin_add_overflow(a, b, &sum);
	_Bool after = sum >= a;
	return (before << 2) | (carry << 1) | after;
}

// IR-LABEL: define{{.*}} i32 @rotate_left_variable(
// IR: call i32 @llvm.fshl.i32(i32 [[VALUE:%.*]], i32 [[VALUE]], i32
u32 rotate_left_variable(u32 value, u32 count) {
	return __builtin_rotateleft32(value, count);
}

// IR-LABEL: define{{.*}} i32 @rotate_right_variable(
// IR: call i32 @llvm.fshr.i32(i32 [[VALUE:%.*]], i32 [[VALUE]], i32
u32 rotate_right_variable(u32 value, u32 count) {
	return __builtin_rotateright32(value, count);
}

u32 rotate_left_constant(u32 value) {
	return __builtin_rotateleft32(value, 7);
}

u32 rotate_right_constant(u32 value) {
	return __builtin_rotateright32(value, 17);
}

u32 rotate_left_i8(u32 value, u32 count) {
	return __builtin_rotateleft8((u8)value, (u8)count);
}

u32 rotate_right_i16(u32 value, u32 count) {
	return __builtin_rotateright16((u16)value, (u16)count);
}

u32 unsigned_add_overflow(u32 a, u32 b, _Bool *flag) {
	u32 result;
	*flag = __builtin_add_overflow(a, b, &result);
	return result;
}

int signed_add_overflow(int a, int b, _Bool *flag) {
	int result;
	*flag = __builtin_add_overflow(a, b, &result);
	return result;
}

u32 unsigned_sub_overflow(u32 a, u32 b, _Bool *flag) {
	u32 result;
	*flag = __builtin_sub_overflow(a, b, &result);
	return result;
}

int signed_sub_overflow(int a, int b, _Bool *flag) {
	int result;
	*flag = __builtin_sub_overflow(a, b, &result);
	return result;
}

u32 overflow_branch(u32 a, u32 b) {
	u32 result;
	return __builtin_add_overflow(a, b, &result) ? result : 7;
}

u32 overflow_arithmetic(u32 a, u32 b) {
	u32 result;
	_Bool flag = __builtin_sub_overflow(a, b, &result);
	return result + flag;
}

int overflow_sign_extension(int a, int b) {
	int result;
	_Bool flag = __builtin_add_overflow(a, b, &result);
	return -(int)flag;
}

u32 overflow_select(u32 a, u32 b, u32 when_true, u32 when_false) {
	u32 result;
	_Bool flag = __builtin_sub_overflow(a, b, &result);
	return flag ? when_true : when_false;
}

u32 overflow_bitwise(u32 a, u32 b, u32 mask) {
	u32 result;
	_Bool flag = __builtin_add_overflow(a, b, &result);
	return result ^ ((flag | (flag & mask)) + flag);
}

void packed_overflow_store(struct PackedOverflow *record, u32 a, u32 b) {
	u32 result;
	record->flag = __builtin_add_overflow(a, b, &result);
	record->value = result;
}

u32 packed_overflow_reload(struct PackedOverflow *record) {
	return record->prefix + record->value + record->flag + record->suffix;
}

u32 rotate_live_across_call(u32 value, u32 count) {
	u32 rotated = __builtin_rotateleft32(value, count);
	return helper(value) + rotated;
}

u32 overflow_live_across_call(u32 a, u32 b) {
	u32 result;
	_Bool flag = __builtin_sub_overflow(a, b, &result);
	return helper(result) + flag;
}

u32 rotate_overflow_tls(u32 value, u32 count, u32 addend) {
	u32 sum;
	_Bool flag = __builtin_add_overflow(value, addend, &sum);
	return helper(tls_value) + __builtin_rotateright32(sum, count) + flag;
}

u32 rotate_overflow_frame_pressure(u32 value, u32 count, u32 addend) {
	volatile struct OverflowPressure local;
	u32 sum;
	local.flag = __builtin_add_overflow(value, addend, &sum);
	local.words[23] = __builtin_rotateleft32(sum, count);
	return helper(local.words[23]) + local.flag;
}

u32 narrow_overflow(u8 a, u8 b, u16 c, u16 d) {
	u8 sum;
	u16 difference;
	_Bool carry = __builtin_add_overflow(a, b, &sum);
	_Bool borrow = __builtin_sub_overflow(c, d, &difference);
	return sum + difference + carry + borrow;
}

__attribute__((noinline)) static u32 internal_rotate_overflow(u32 value, u32 count, u32 addend) {
	u32 sum;
	_Bool carry = __builtin_add_overflow(value, addend, &sum);
	return __builtin_rotateleft32(sum, count) + carry;
}

// IR-LABEL: define{{.*}} i32 @call_internal_rotate_overflow(
// IR: call fastcc i32 @internal_rotate_overflow(
u32 call_internal_rotate_overflow(u32 value, u32 count, u32 addend) {
	return internal_rotate_overflow(value, count, addend);
}

// IR-LABEL: define internal fastcc{{.*}} i32 @internal_rotate_overflow(
