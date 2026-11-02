// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O0 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O1 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -Os -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O0 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O1 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O2 -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -Os -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -mcpu=sh2 -O0 -fPIC -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -mcpu=sh2 -O1 -fPIC -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -mcpu=sh2 -O2 -fPIC -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -mcpu=sh2 -Os -fPIC -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh2 -O0 -fPIC -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh2 -O1 -fPIC -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh2 -O2 -fPIC -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh2 -Os -fPIC -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O0 -fstack-protector -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O0 -fstack-protector-strong -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O0 -fstack-protector-all -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -mcpu=sh2 -O2 -fPIC -fstack-protector-all -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh2 -O2 -fPIC -fstack-protector-all -c -mllvm -verify-machineinstrs -o /dev/null %s

typedef _Atomic(unsigned char) atomic_u8;
typedef _Atomic(unsigned short) atomic_u16;

extern void consume(void *);
extern unsigned five(unsigned, unsigned, unsigned, unsigned, unsigned);

unsigned one_vla(unsigned count, unsigned value) {
	unsigned fixed = value;
	volatile unsigned values[count];
	values[0] = value;
	consume((void *)values);
	return fixed + values[0] + five(value, value, value, value, value);
}

unsigned nested_conditional_vlas(unsigned outer_count, unsigned inner_count, unsigned condition) {
	volatile unsigned outer[outer_count];
	outer[0] = outer_count;
	if (condition) {
		volatile unsigned short inner[inner_count];
		inner[0] = (unsigned short)inner_count;
		consume((void *)inner);
	}
	return outer[0];
}

unsigned conditional_vla(unsigned count, unsigned condition) {
	if (condition) {
		volatile unsigned values[count];
		values[0] = count;
		consume((void *)values);
		return values[0];
	}
	return count;
}

unsigned realigned_local(unsigned value) {
	_Alignas(32) volatile unsigned local = value;
	consume((void *)&local);
	return local;
}

unsigned large_frame(unsigned index, unsigned value) {
	_Alignas(32) volatile unsigned char frame[262144];
	frame[0] = (unsigned char)value;
	frame[131072] = (unsigned char)(value >> 8);
	frame[262143] = (unsigned char)(value >> 16);
	return frame[index & 262143];
}

unsigned protected_objects(unsigned value) {
	char buffer[32];
	_Alignas(16) unsigned aligned = value;
	buffer[0] = (char)value;
	consume(buffer);
	consume(&aligned);
	return (unsigned char)buffer[0] + aligned;
}

unsigned atomic_u8_operations(atomic_u8 *object, unsigned value) {
	unsigned char narrow = (unsigned char)value;
	unsigned char expected = narrow;
	unsigned result = __c11_atomic_load(object, __ATOMIC_ACQUIRE);
	__c11_atomic_store(object, narrow, __ATOMIC_RELEASE);
	result += __c11_atomic_exchange(object, narrow, __ATOMIC_ACQ_REL);
	result += __c11_atomic_compare_exchange_strong(object, &expected, narrow,
		__ATOMIC_SEQ_CST, __ATOMIC_ACQUIRE);
	result += __c11_atomic_fetch_add(object, narrow, __ATOMIC_RELAXED);
	result += __c11_atomic_fetch_sub(object, narrow, __ATOMIC_RELAXED);
	result += __c11_atomic_fetch_and(object, narrow, __ATOMIC_RELAXED);
	result += __c11_atomic_fetch_or(object, narrow, __ATOMIC_RELAXED);
	result += __c11_atomic_fetch_xor(object, narrow, __ATOMIC_RELAXED);
	return result;
}

unsigned atomic_u16_operations(atomic_u16 *object, unsigned value) {
	unsigned short narrow = (unsigned short)value;
	unsigned short expected = narrow;
	unsigned result = __c11_atomic_load(object, __ATOMIC_SEQ_CST);
	__c11_atomic_store(object, narrow, __ATOMIC_SEQ_CST);
	result += __c11_atomic_exchange(object, narrow, __ATOMIC_SEQ_CST);
	result += __c11_atomic_compare_exchange_weak(object, &expected, narrow,
		__ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
	result += __c11_atomic_fetch_add(object, narrow, __ATOMIC_RELAXED);
	result += __c11_atomic_fetch_sub(object, narrow, __ATOMIC_RELAXED);
	result += __c11_atomic_fetch_and(object, narrow, __ATOMIC_RELAXED);
	result += __c11_atomic_fetch_or(object, narrow, __ATOMIC_RELAXED);
	result += __c11_atomic_fetch_xor(object, narrow, __ATOMIC_RELAXED);
	return result;
}

void *frame_address(void) {
	return __builtin_frame_address(0);
}

void *return_address(void) {
	return __builtin_return_address(0);
}
