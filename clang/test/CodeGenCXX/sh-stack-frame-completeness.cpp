// RUN: %clangxx --target=sh-unknown-none-elf -mcpu=sh2 -O0 -fexceptions -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clangxx --target=sh-unknown-none-elf -mcpu=sh2 -O2 -fexceptions -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clangxx --target=shle-unknown-none-elf -mcpu=sh2 -O0 -fexceptions -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clangxx --target=shle-unknown-none-elf -mcpu=sh2 -O2 -fexceptions -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clangxx --target=sh-unknown-linux-gnu -mcpu=sh2 -O0 -fPIC -fexceptions -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clangxx --target=sh-unknown-linux-gnu -mcpu=sh2 -O2 -fPIC -fexceptions -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clangxx --target=shle-unknown-linux-gnu -mcpu=sh2 -O0 -fPIC -fexceptions -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clangxx --target=shle-unknown-linux-gnu -mcpu=sh2 -O2 -fPIC -fexceptions -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clangxx --target=sh-unknown-none-elf -mcpu=sh3 -O2 -fexceptions -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clangxx --target=shle-unknown-none-elf -mcpu=sh3 -O2 -fexceptions -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clangxx --target=sh-unknown-linux-gnu -mcpu=sh3 -O2 -fPIC -fexceptions -c -mllvm -verify-machineinstrs -o /dev/null %s
// RUN: %clangxx --target=shle-unknown-linux-gnu -mcpu=sh3 -O2 -fPIC -fexceptions -c -mllvm -verify-machineinstrs -o /dev/null %s

extern void consume(void *);
extern void may_throw();

struct Object {
	unsigned value;
	Object(unsigned input) : value(input) {}
	~Object();
};

unsigned object_and_vla(unsigned count, unsigned value) {
	Object object(value);
	volatile unsigned values[count];
	values[0] = value;
	consume((void *)values);
	may_throw();
	return object.value + values[0];
}

unsigned realigned_object(unsigned value) {
	alignas(32) Object object(value);
	consume(&object);
	may_throw();
	return object.value;
}

unsigned large_object_frame(unsigned value) {
	Object object(value);
	volatile unsigned char frame[131072];
	frame[0] = (unsigned char)value;
	frame[131071] = (unsigned char)(value >> 8);
	may_throw();
	return object.value + frame[0] + frame[131071];
}
