// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O0 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O1 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O2 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -Os -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O0 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O1 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O2 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -Os -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -pic-level 2 -O0 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -pic-level 2 -O1 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -pic-level 2 -O2 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -target-cpu sh2 -pic-level 2 -Os -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -pic-level 2 -O0 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -pic-level 2 -O1 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -pic-level 2 -O2 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -pic-level 2 -Os -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple sh-unknown-none-elf -target-cpu sh2 -O1 -debug-info-kind=standalone -dwarf-version=4 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-none-elf -target-cpu sh2 -O1 -debug-info-kind=standalone -dwarf-version=4 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -target-cpu sh2 -pic-level 2 -O2 -S -mllvm -verify-machineinstrs %s -o - | FileCheck %s

typedef unsigned char u8;

struct LocalRecord {
	unsigned word;
	unsigned short half;
	u8 bytes[66];
};

struct __attribute__((packed)) PackedRecord {
	u8 prefix;
	unsigned word;
	u8 bytes[67];
};

struct NestedRecord {
	struct LocalRecord records[2];
	struct PackedRecord packed;
};

struct VolatileRecord {
	u8 bytes[16];
};

u8 fixed_global_source[128] = {0x35};
u8 fixed_global_destination[128];

int far_memset_min(unsigned index) {
	char frame[4096];
	__builtin_memset(frame, 0x5a, sizeof(frame));
	return frame[index & 4095];
}

int local_array_memset(unsigned index, unsigned value) {
	u8 frame[512];
	__builtin_memset(frame, value, sizeof(frame));
	return frame[index & 511];
}

int local_struct_memset(unsigned index) {
	struct LocalRecord frame;
	__builtin_memset(&frame, 0x26, sizeof(frame));
	return ((u8 *)&frame)[index % sizeof(frame)];
}

int packed_struct_memset(unsigned index) {
	struct PackedRecord frame;
	__builtin_memset(&frame, 0x47, sizeof(frame));
	return ((u8 *)&frame)[index % sizeof(frame)];
}

int nested_partial_memset(unsigned index) {
	struct NestedRecord frame;
	__builtin_memset(&frame.records[1], 0x68, sizeof(frame.records[1]));
	__builtin_memset(frame.packed.bytes + 3, 0x79, 32);
	return ((u8 *)&frame)[index % sizeof(frame)];
}

int zero_length_local_memset(unsigned value) {
	u8 frame[8];
	frame[0] = value;
	__builtin_memset(frame + 1, 0, 0);
	return frame[0];
}

int local_to_local_memcpy(unsigned index) {
	u8 source[256];
	u8 destination[256];
	__builtin_memset(source, 0x2a, sizeof(source));
	__builtin_memcpy(destination, source, sizeof(destination));
	return destination[index & 255];
}

int global_to_local_memcpy(unsigned index) {
	u8 destination[128];
	__builtin_memcpy(destination, fixed_global_source, sizeof(destination));
	return destination[index & 127];
}

int local_to_global_memcpy(unsigned index, unsigned value) {
	u8 source[128];
	__builtin_memset(source, value, sizeof(source));
	__builtin_memcpy(fixed_global_destination, source, sizeof(source));
	return fixed_global_destination[index & 127];
}

int packed_local_memcpy(unsigned index) {
	struct PackedRecord source;
	struct PackedRecord destination;
	__builtin_memset(&source, 0x4b, sizeof(source));
	__builtin_memcpy(&destination, &source, sizeof(destination));
	return ((u8 *)&destination)[index % sizeof(destination)];
}

int partial_and_zero_local_memcpy(unsigned index) {
	u8 source[64];
	u8 destination[64];
	__builtin_memset(source, 0x5c, sizeof(source));
	__builtin_memcpy(destination + 5, source + 7, 31);
	__builtin_memcpy(destination, source, 0);
	return destination[5 + (index % 31)];
}

int far_local_transfers(unsigned index) {
	u8 source[4096];
	u8 destination[4096];
	__builtin_memset(source, 0x3e, sizeof(source));
	__builtin_memcpy(destination, source, sizeof(destination));
	__builtin_memmove(destination + 1024, destination + 1000, 64);
	return destination[index & 4095];
}

int overlapping_local_memmove(unsigned index) {
	u8 frame[512];
	__builtin_memset(frame, 0x6d, sizeof(frame));
	__builtin_memmove(frame + 3, frame + 1, 64);
	__builtin_memmove(frame + 128, frame + 130, 62);
	__builtin_memmove(frame + 256, frame, 32);
	__builtin_memmove(frame, frame + 1, 0);
	return frame[index & 511];
}

int selected_local_memset(unsigned condition, unsigned index) {
	u8 left[64];
	u8 right[64];
	__builtin_memset(condition ? left : right, 0x72, sizeof(left));
	return (condition ? left : right)[index & 63];
}

int volatile_local_copy(unsigned index) {
	volatile struct VolatileRecord source;
	volatile struct VolatileRecord destination;
	for (unsigned i = 0; i != sizeof(source.bytes); ++i)
		source.bytes[i] = i + 1;
	destination = source;
	return destination.bytes[index & 15];
}

int near_bound_local_memset(unsigned index) {
	u8 frame[60000];
	__builtin_memset(frame, 0x7e, sizeof(frame));
	return frame[index % sizeof(frame)];
}

// CHECK-LABEL: far_memset_min:
// CHECK: mov #16,r3
// CHECK: shll8 r3
// CHECK: add #4,r3
// CHECK: sub r3,r15
// CHECK: mov #90,r5
// CHECK: mov #16,r6
// CHECK: shll8 r6
// CHECK: jsr
// CHECK: and {{r[0-9]+}},r0
// CHECK-NEXT: mov.b @(r0,{{r[0-9]+}}),r0
// CHECK-NEXT: extu.b r0,r0
// CHECK: add r3,r15
