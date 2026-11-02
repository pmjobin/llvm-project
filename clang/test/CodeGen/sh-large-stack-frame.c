// RUN: %clang_cc1 -triple sh-unknown-none-elf -O0 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple sh-unknown-none-elf -O1 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple sh-unknown-none-elf -O2 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple sh-unknown-none-elf -Os -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-none-elf -O0 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-none-elf -O1 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-none-elf -O2 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-none-elf -Os -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -pic-level 2 -O0 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -pic-level 2 -O1 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -pic-level 2 -O2 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple sh-unknown-linux-gnu -pic-level 2 -Os -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -pic-level 2 -O0 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -pic-level 2 -O1 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -pic-level 2 -O2 -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -pic-level 2 -Os -emit-obj -mllvm -verify-machineinstrs %s -o /dev/null
// RUN: %clang_cc1 -triple shle-unknown-linux-gnu -pic-level 2 -O0 -S -mllvm -verify-machineinstrs %s -o - | FileCheck %s

typedef unsigned int u32;

extern u32 fresh_long_external(u32);

#ifndef FIXED_FRAME_BYTES
#define FIXED_FRAME_BYTES 65512
#endif

#define FRESH_STEP value = fresh_long_external(value ^ 0x9e3779b9); if (value & 1) accumulator += value; else accumulator ^= value
#define FRESH_2 FRESH_STEP; FRESH_STEP
#define FRESH_4 FRESH_2; FRESH_2
#define FRESH_8 FRESH_4; FRESH_4
#define FRESH_16 FRESH_8; FRESH_8
#define FRESH_32 FRESH_16; FRESH_16
#define FRESH_64 FRESH_32; FRESH_32
#define FRESH_128 FRESH_64; FRESH_64
#define FRESH_256 FRESH_128; FRESH_128

__attribute__((noinline)) u32 fresh_long_control(u32 value) {
	volatile u32 accumulator = value + 3;
	if (value == 0)
		goto far_target;
	FRESH_256;
far_target:
	return accumulator ^ fresh_long_external(value + 7);
}

__attribute__((noinline)) u32 fixed_frame_near_64k(u32 value) {
	volatile unsigned char frame[FIXED_FRAME_BYTES];
	frame[0] = (unsigned char)value;
	frame[FIXED_FRAME_BYTES / 2] = (unsigned char)(value >> 8);
	frame[FIXED_FRAME_BYTES - 1] = (unsigned char)(value >> 16);
	return frame[0] + frame[FIXED_FRAME_BYTES / 2] + frame[FIXED_FRAME_BYTES - 1];
}

struct __attribute__((packed)) PackedPointer {
	unsigned char tag;
	unsigned *pointer;
	unsigned short tail;
};

static unsigned packed_pointer_anchor;
static _Thread_local struct PackedPointer packed_pointer_tls = {
	1, &packed_pointer_anchor, 2};

unsigned large_frame_packed_tls_pointer(void) {
	volatile unsigned char frame[512];
	frame[0] = packed_pointer_tls.tag;
	frame[511] = (unsigned char)packed_pointer_tls.tail;
	return *packed_pointer_tls.pointer + frame[0] + frame[511];
}

unsigned large_frame_dynamic_index(unsigned index, unsigned value) {
	volatile unsigned char frame[512];
	frame[index & 511] = (unsigned char)value;
	return frame[index & 511];
}

// CHECK-LABEL: fresh_long_control:
// CHECK: sts.l	pr,@-r15
// CHECK: sub	r3,r15
// CHECK: add	r3,r15
// CHECK: lds.l	@r15+,pr
// CHECK: rts
// CHECK-LABEL: fixed_frame_near_64k:
// CHECK: sub	r3,r15
// CHECK: add	r3,r15
// CHECK: rts
