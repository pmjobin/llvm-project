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
// RUN: %clang --target=sh-unknown-none-elf -mcpu=sh2 -O0 -fstack-protector-all -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-none-elf -mcpu=sh2 -O0 -fstack-protector-all -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=sh-unknown-linux-gnu -mcpu=sh2 -O0 -fPIC -fstack-protector-all -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clang --target=shle-unknown-linux-gnu -mcpu=sh2 -O0 -fPIC -fstack-protector-all -c -mllvm -verify-machineinstrs -o /dev/null %s

void *published_pointer;

extern void consume_buffer(char *);

unsigned generic_integer_builtins(unsigned value) {
	return __builtin_clz(value) + __builtin_ctz(value) +
	       __builtin_popcount(value) + __builtin_bswap32(value) +
	       __builtin_bitreverse32(value);
}

unsigned unsigned_multiply_overflow(unsigned a, unsigned b, _Bool *overflow) {
	unsigned result;
	*overflow = __builtin_mul_overflow(a, b, &result);
	return result;
}

int signed_multiply_overflow(int a, int b, _Bool *overflow) {
	int result;
	*overflow = __builtin_mul_overflow(a, b, &result);
	return result;
}

void *thread_pointer(void) {
	return __builtin_thread_pointer();
}

unsigned publish_fixed_address(unsigned value) {
	published_pointer = &value;
	return (unsigned long)&value;
}

int protected_buffer(unsigned value) {
	char buffer[32];
	buffer[0] = value;
	consume_buffer(buffer);
	return buffer[0];
}
